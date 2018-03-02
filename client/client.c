#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h>
#include <thread>
#include <sys/epoll.h>
#include <string>
#include <algorithm>


using namespace std;

ssize_t readData(int fd, char * buffer, ssize_t buffsize){
    auto ret = read(fd, buffer, buffsize);
    if(ret==-1) error(1,errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd, char * buffer, ssize_t count){
    auto ret = write(fd, buffer, count);
    if(ret==-1) error(1, errno, "write failed on descriptor %d", fd);
    if(ret!=count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

void reading(int sock) {
    while(true) {
        ssize_t bufsize1 = 255, received1;
        char buffer1[bufsize1];
        received1 = readData(sock, buffer1, bufsize1);
        writeData(1, buffer1, received1);
    }
}

int main(int argc, char ** argv){
    if(argc!=3) error(1,0,"Need 2 args");

    // Resolve arguments to IPv4 address with a port number
    addrinfo *resolved, hints={.ai_flags=0, .ai_family=AF_INET, .ai_socktype=SOCK_STREAM};
    int res = getaddrinfo(argv[1], argv[2], &hints, &resolved);
    if(res || !resolved) error(1, errno, "getaddrinfo");

    // create socket
    int sock = socket(resolved->ai_family, resolved->ai_socktype, 0);
    if(sock == -1) error(1, errno, "socket failed");

    // attept to connect
    res = connect(sock, resolved->ai_addr, resolved->ai_addrlen);
    if(res) error(1, errno, "connect failed");

    // free memory
    freeaddrinfo(resolved);


    ssize_t bufsize = 255;
    char buffer1[bufsize];

    int fd = epoll_create1(0);

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sock;

    epoll_ctl(fd, EPOLL_CTL_ADD, sock, &event);
    event.data.fd = 0;
    epoll_ctl(fd, EPOLL_CTL_ADD, 0, &event);

    while(true) {
        epoll_wait(fd, &event, 1, -1);

        if (event.events == EPOLLIN && event.data.fd == sock) {
            int rodeBytes = readData(sock, buffer1, bufsize);
            writeData(1, buffer1, rodeBytes);
        }

        if (event.events == EPOLLIN && event.data.fd == 0) {

            int rodeDataBytes = readData(0, buffer1, bufsize);
            writeData(sock, buffer1, rodeDataBytes);
            
            string userMessage(buffer1, rodeDataBytes);
            userMessage.erase(remove(userMessage.begin(), userMessage.end(), '\n'), userMessage.end());
            if (userMessage == "quit") 
                break;
                
        }
    }

    close(fd);

    close(sock);

    return 0;
}
