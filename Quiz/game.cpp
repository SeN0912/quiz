#include "game.h"

Game::Game(string ip, int port)
{
    this->ip = ip;
    this->port = port;
    srand((unsigned int) time(NULL));
}

void Game::Setup()
{
    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if (servFd == -1) error(1, errno, "socket failed");

    SetReuseAddr(servFd);

    sockaddr_in serverAddr {.sin_family = AF_INET, .sin_port = htons((short)port), inet_addr(ip.c_str()) };
    int res = bind(servFd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (res) error(1, errno, "bind failed");

    res = listen(servFd, 1);
    if (res) error(1, errno, "listen failed");

    fd = epoll_create1(0);

    event.events = EPOLLIN;
    event.data.fd = servFd;
    epoll_ctl(fd, EPOLL_CTL_ADD, servFd, &event);
}

void Game::SetReuseAddr(int sock)
{
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(res) error(1,errno, "setsockopt failed");
}

void Game::Start()
{
    thread CheckingThread(&Game::CheckForNoAnswer, this);
    while(true)
    {
        PerformMessages();
    }
}

void Game::PerformMessages()
{
    epoll_wait(fd, &event, 1, -1);
    if (event.events == EPOLLIN)
    {
        if (event.data.fd == servFd)
        {
            AddUser();
        }
        else
        {
            char buffer[100];
            int res = read(event.data.fd, buffer, 100);
            if (res < 1)
            {
                RemoveUser(event.data.fd);

                GameOver();
            }
            else
            {
                string userMessage(buffer, res);
                userMessage.erase(std::remove(userMessage.begin(), userMessage.end(), '\n'), userMessage.end());

                User* sender = GetUser(event.data.fd);
                if (!sender->IsNameSet())
                {
                    if (userMessage.size() < 3)
                    {
                        SendToOne(event.data.fd, "Nazwa musi mieć więcej niż 3 znaki");
                        return;
                    }

                    if (userMessage.size() > 15)
                    {
                        SendToOne(event.data.fd, "Nazwa musi mieć mniej niż 15 znaków");
                        return;
                    }

                    if (UserNameExist(userMessage))
                    {
                        SendToOne(event.data.fd, "Gracz z taką nazwą już się połączył");
                        return;
                    }

                    sender->SetName(userMessage);
                    SendToOne(event.data.fd, "Witaj, " + userMessage);
                    SendToAll("Gracz: " + sender->Name + " dołączyl do gry");

                    if(gameIsOn)
                    {
                        if (IsTooMuchUsers())
                        {
                            SendToOne(event.data.fd, "Niestety jest za dużo graczy. Poczekaj, aż któryś się rozłączy");
                        }
                        else
                        {
                            sender->IsPlaying = true;
                            SortRanking();
                            ShowRanking(event.data.fd);
                            SendToOne(event.data.fd, GetQuestion());
                            sender->ResetTimestamp();
                        }
                    }
                    else
                    {
                        sender->IsPlaying = true;

                        if (IsEnoughUsers())
                        {
                            StartGame();
                        }
                        else
                        {
                            int remainingUsers = 3 - GetPlayingUsersCount();
                            SendToAll("Brakuje: " + to_string(remainingUsers) + " graczy, aby zacząć grę");
                        }
                    }
                    return;
                }

                if (!sender->IsPlaying)
                {
                    SendToOne(event.data.fd, "Niestety jest za dużo graczy. Poczekaj, aż któryś się rozłączy");
                    return;
                }

                if (userMessage == "quit")
                {
                    SendToAllBut(event.data.fd, "Gracz: " + sender->Name + " opuścił grę");
                    RemoveUser(event.data.fd);

                    GameOver();

                    return;
                }

                if (!gameIsOn)
                {
                    SendToOne(event.data.fd, "Gra się jeszcze nie zaczęła");
                    return;
                }

                if (IsAnswerCorrect(userMessage))
                {
                    int answerTime = CalculateTimestampDiff(sender->SeenAnswerTimestamp);
                    SendToAllBut(event.data.fd, "Użytkownik: " + sender->Name + " wygrał rundę! Jego odpowiedź to: " + userMessage +". Zajęło mu to: " + to_string(answerTime) + " sekund.");
                    SendToOne(event.data.fd, "Brawo! Wygrywasz tę runde! Zajęło Ci to: " + to_string(answerTime) + " sekund");
                    sender->AddPoint();
                    PlayNextRound(true);
                }
                else
                    SendToOne(event.data.fd, "Niestety, źle. " + GetQuestion());
            }
        }
    }
}

void Game::AllowAnotherUserToPlay()
{
    for(auto &v : users)
        if (!v.IsPlaying && v.IsNameSet())
        {
            v.IsPlaying = true;
            SendToOne(v.Fd, "Zwolniło się miejsce. Wkraczasz do gry");
            SortRanking();
            ShowRanking(v.Fd);
            SendToOne(v.Fd, GetQuestion());
            return;
        }
}

bool Game::IsAnswerCorrect(string userMessage)
{
    return find(answers.begin(), answers.end(), userMessage) != answers.end();
}

void Game::SendToOne(int fd, string message)
{
    message = message + "\n";
    int res = write(fd, message.c_str(), message.length());
    if (res != message.length())
    {
        SendToAllBut(fd, "usunieto");
        RemoveUser(fd);
    }
}

string Game::GetQuestion()
{
    return "Zgadnij: " + randomTopic + " na literę: " + randomLetter;
}

void Game::SendToAll(string message)
{
    for (auto v : users)
        if (v.IsPlaying)
            SendToOne(v.Fd, message);
}

void Game::SendToAllBut(int fd, string message)
{
    for (auto v : users)
        if (v.Fd != fd)
            SendToOne(v.Fd, message);
}

double Game::CalculateTimestampDiff(time_t timestamp)
{
    time_t now = time(0);
    return difftime(now, timestamp);
}

int Game::GetPlayingUsersCount()
{
    int playingUsers = 0;

    for(auto v:users)
        if(v.IsPlaying)
            playingUsers++;

    return playingUsers;
}

bool Game::IsEnoughUsers()
{
    return GetPlayingUsersCount() >= 3;
}

bool Game::IsTooMuchUsers()
{
    return GetPlayingUsersCount() >= 4;
}

void Game::PlayNextRound(bool isAnswered)
{
    SendToAll("\n\n\n");
    if (!isAnswered)
    {
        SendToAll("Nikt nie odpowiedział na pytanie. Losuję kolejne");
    }

    randomTopic = GetRandomTopic();
    if (randomTopic == "")
        throw("No topics");

    vector<string> allAnswers = GetFileContent(dataFolder + randomTopic + ".txt");
    vector<string> possibleChars = GetPossibleChars(allAnswers);
    randomLetter = possibleChars[rand() % possibleChars.size()];

    answers = GetCorrectAnswers(allAnswers, randomLetter);
    if (answers.size() == 0)
        throw("No answers");

    SendToAll("Zaraz rozpocznie się kolejna runda! Oto ranking: ");
    ShowRanking();
    SendToAll(GetQuestion());

    ResetUsersTimestamp();

    showedQuestionTimestamp = time(0);
    SendToAll("\n\n\n");
}

string Game::GetRandomTopic()
{
    vector<string> topics = GetFileContent(dataFolder + "topics.txt");

    if (topics.size() > 0)
        return topics[rand()%topics.size()];

    return "";
}

vector<string> Game::GetFileContent(string filename)
{
    ifstream myfile(filename);

    if(!myfile)
        throw("Error opening output file");

    string line;
    vector<string> lines;

    while (std::getline(myfile, line))
        if (line != "")
            lines.push_back(line);

    return lines;
}

bool Game::IsLatin(char ch)
{
    return ((int)ch < 0);
}

vector<string> Game::GetPossibleChars(vector<string> allAnswers)
{
    vector<string> chars;

    for(auto v : allAnswers)
    {
        char chr = v.at(0);
        string firstChar = v.substr(0,1);

        if (IsLatin(chr))
        {
            firstChar = v.substr(0,2);
        }

        if(find(chars.begin(), chars.end(), firstChar) == chars.end())
            chars.push_back(firstChar);
    }

    return chars;
}

void Game::SortRanking()
{
    sort(users.begin(), users.end());
}

void Game::ShowRanking()
{
    SortRanking();

    for(auto v : users)
        if (v.IsPlaying)
            ShowRanking(v.Fd);
}

void Game::ShowRanking(int fd)
{
    int counter = 1;

    SendToOne(fd, "========== RANKING ==========");
    for (auto v : users)
        if (v.IsPlaying)
            SendToOne(fd, "   " + to_string(counter++) + ". " + v.Name + ": " + to_string(v.Points));

    SendToOne(fd, "=============================");
}

vector<string> Game::GetCorrectAnswers(vector<string> allAnswers, string firstChar)
{
    vector<string> correctAnswers;

    for (auto v : allAnswers)
    {
        char chr = v.at(0);
        string answerFirstChar = v.substr(0,1);

        if (IsLatin(chr))
        {
            answerFirstChar = v.substr(0,2);

        }

        if (answerFirstChar == firstChar)
            correctAnswers.push_back(v);
    }

    return correctAnswers;
}

vector<User> Game::GetWinners()
{
    vector<User> winners;
    if (users.size() == 0)
        return winners;

    sort(users.begin(), users.end());

    int maxPoints = users[0].Points;

    if (users.size() >= 2 && users[1].Points == maxPoints)
    {
        for(auto v:users)
            if (v.Points == maxPoints)
                winners.push_back(v);
    }
    else
        winners.push_back(users[0]);

    return winners;
}

void Game::CheckForNoAnswer()
{
    while(true)
    {
        if (gameIsOn)
        {
            if (CalculateTimestampDiff(showedQuestionTimestamp) > displayNextQuestionAfter)
            {
                PlayNextRound(false);
            }
        }
        sleep(3);
    }
}

void Game::ShowWinners()
{
    vector<User> winners = GetWinners();

    if (winners.size() == 0)
        return;

    if (winners.size() == 1)
        SendToAll("Wygrywa: " + winners[0].Name + " z liczba punktow: " + to_string(winners[0].Points));
    else
    {
        SendToAll("Zwyciężcow jest kilka. Oto oni: ");
        for(auto v : winners)
            SendToAll(to_string(v.Points) + ". " + v.Name);
    }
}

void Game::GameOver()
{
    AllowAnotherUserToPlay();

    if (!IsEnoughUsers() && gameIsOn)
    {
        gameIsOn = false;
        SendToAll("Niestety jest za mało graczy. Gra sie konczy. Oto aktualny ranking:");
        ShowRanking();
        ShowWinners();
    }
}

void Game::StartGame()
{
    ResetUsersPoints();
    gameIsOn = true;
    PlayNextRound(true);
}

void Game::ResetUsersPoints()
{
    for(auto &v:users)
        v.ResetPoints();
}

void Game::ResetUsersTimestamp()
{
    for(auto &v: users)
        if (v.IsPlaying)
            v.ResetTimestamp();
}

User* Game::GetUser(int fd)
{
    auto it = FindUser(fd);
    return it == end(users) ? nullptr : &*it;
}

vector<User>::iterator Game::FindUser(int fd)
{
    return find_if(begin(users), end(users), [fd](User const& u)
    {
        return u.Fd == fd;
    });
}

void Game::RemoveUser(int fd)
{
    auto it = FindUser(fd);
    if(it != end(users))
        users.erase(it);


    close(fd);
}

void Game::RemoveAllUsers()
{
    for(auto v : users)
        RemoveUser(v.Fd);
}

bool Game::UserNameExist(string name)
{
    for (auto v:users)
        if(v.Name == name)
            return true;
    return false;
}

void Game::AddUser()
{
    // prepare placeholders for client address
    sockaddr_in clientAddr {0};
    socklen_t clientAddrSize = sizeof(clientAddr);

    // accept new connection
    auto clientFd = accept(servFd, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientFd == -1)
        error(1, errno, "accept failed");

    // add client to all clients set
    User newUser(clientFd);
    users.push_back(newUser);

    event.data.fd = clientFd;
    epoll_ctl(fd, EPOLL_CTL_ADD, clientFd, &event);

    SendToOne(event.data.fd, "Wpisz swoj nick: ");
}
