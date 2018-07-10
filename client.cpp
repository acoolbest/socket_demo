#include "client.h"
//g++ client.cpp -lpthread -g -Wall -std=c++11
#define CLIENT_COUNT 1

static socket_help* gp_socket_help[CLIENT_COUNT]={NULL};

int main()
{
	for(uint16_t i=0;i<CLIENT_COUNT;i++)
	{
		gp_socket_help[i] = new socket_help("103.46.128.41", 57093, i);//103.46.128.41:57093	192.168.0.67:8088
		if(!gp_socket_help[i]->init_socket())
			printf("[%d] ok\n",i);
		else
			printf("[%d] err\n",i);
	}
	while(1) sleep(100);
	return 0;
}
