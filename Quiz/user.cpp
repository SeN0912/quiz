#include "user.h"

User::User(int fd)
{
    this->Fd = fd;
}

void User::AddPoint()
{
    this->Points++;
}

void User::SetName(string name)
{
    this->Name = name;
}

bool User::IsNameSet()
{
    return !this->Name.empty();
}

void User::ResetTimestamp()
{
    this->SeenAnswerTimestamp = time(0);
}

void User::ResetPoints()
{
    this->Points = 0;
}
