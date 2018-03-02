#include "main.h"

uint16_t readPort(char * txt){
	char * ptr;
	auto port = strtol(txt, &ptr, 10);
	if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
	return port;
}

int main(int argc, char ** argv)
{
    // get and validate port number
	if(argc != 3) error(1, 0, "Need 2 arg (ip, port)");
	auto port = readPort(argv[2]);

    Game game(argv[1], port);
    game.Setup();
    game.Start();

    return 0;
}
