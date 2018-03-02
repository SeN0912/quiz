#ifndef USER_H
#define USER_H

#include <string>
#include <time.h>
#include <iterator>

using namespace std;

class User
{
public:
    User(int fd);
    void AddPoint();
    void SetName(string name);
    bool IsNameSet();
    void ResetTimestamp();
    void ResetPoints();

    int Fd;
    bool IsPlaying = false;
    int Points = 0;
    string Name;
    time_t SeenAnswerTimestamp;

    bool operator < (const User& str) const
    {
        return (Points > str.Points);
    }

protected:

private:
};

#endif // USER_H
