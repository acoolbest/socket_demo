#include "client.h"
//g++ client.cpp -lpthread -g -Wall -std=c++11
#define CLIENT_COUNT 1

static socket_help* gp_socket_help[CLIENT_COUNT]={NULL};

vector<string> get_ip(string domain)
{
	domain_to_ip dti(domain);
	vector<string> vec = dti.get_ip();
	return vec;
}
//208l8w1838.51mypc.cn
//103.46.128.41:57093 48438	192.168.0.67:8088
int main()
{
	for(uint16_t i=0;i<CLIENT_COUNT;i++)
	{
		gp_socket_help[i] = new socket_help(get_ip("208l8w1838.51mypc.cn"), 57093, i);
		if(!gp_socket_help[i]->init_socket())
			printf("[%d] ok\n",i);
		else
			printf("[%d] err\n",i);
	}
	while(1) sleep(100);
	return 0;
}
