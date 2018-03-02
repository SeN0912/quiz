CC = g++
CFLAGS  = -Wall -std=c++11 -pthread

all:  main.o game.o user.o 
	$(CC) $(CFLAGS) main.o game.o user.o -o server

main.o:  main.cpp main.h 
	$(CC) $(CFLAGS) -c main.cpp

game.o:  game.cpp game.h
	$(CC) $(CFLAGS) -c game.cpp

user.o:  user.cpp user.h 
	$(CC) $(CFLAGS) -c user.cpp

clean: 
	$(RM) *.o server
