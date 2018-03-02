#ifndef GAME_H
#define GAME_H

#include <string>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h>
#include <signal.h>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <thread>
#include <vector>
#include "user.h"

using namespace std;

class Game
{
    public:
        Game(string ip, int port);
        void Setup();
        void Start();
    protected:

    private:
        string ip;
        int port;
        int servFd;
        int fd;
        bool gameIsOn = false;
        epoll_event event;
        int displayNextQuestionAfter = 15;

        void SetReuseAddr(int sock);
        void PerformMessages();
        void AddUser();
        bool IsEnoughUsers();
        bool IsTooMuchUsers();
        void StartGame();
        void RemoveUser(int fd);
        void GameOver();
        void SendToOne(int fd, string message);
        void SendToAll(string message);
        void SendToAllBut(int fd, string message);
        void PlayNextRound(bool isAnswered);
        bool IsAnswerCorrect(string answer);
        double CalculateTimestampDiff(time_t timestamp);
        time_t showedQuestionTimestamp;
        void ShowRanking(int fd);
        void ShowRanking();
        void ShowWinners();
        void RemoveAllUsers();
        void ResetUsersPoints();
        void ResetUsersTimestamp();
        int GetPlayingUsersCount();
        void AllowAnotherUserToPlay();
        void SortRanking();
        void CheckForNoAnswer();
        bool UserNameExist(string name);

        string GetRandomTopic();
        vector<string> GetFileContent(string filename);
        char GetRandomLetter();
        vector<string> GetCorrectAnswers(vector<string> allAnswers, string firstChar);
        vector<User> GetWinners();
        vector<string> GetPossibleChars(vector<string> allAnswers);

        vector<string> answers;
        vector<User> users;

        User *GetUser(int fd);
        string randomLetter;
        string randomTopic;
        string dataFolder = "data/";
        string GetQuestion();
        bool IsLatin(char ch);

        vector<User>::iterator FindUser( int fd);
};

#endif // GAME_H
