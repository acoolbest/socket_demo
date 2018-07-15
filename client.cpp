#include "client.h"
//g++ client.cpp -lpthread -g -Wall -std=c++11
#define CLIENT_COUNT 1
//39.106.26.66:9503
//#define ZHZQ
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
	#ifdef ZHZQ
	vector<string> vec {"103.46.128.41"};
	#else
	vector<string> vec {"39.106.26.66"};
	#endif
	for(uint16_t i=0;i<CLIENT_COUNT;i++)
	{
		#ifdef ZHZQ
		gp_socket_help[i] = new socket_help(vec, 48438, i);
		//gp_socket_help[i] = new socket_help(get_ip("208l8w1838.51mypc.cn"), 48438, i);
		#else
		gp_socket_help[i] = new socket_help(vec, 9503, i);
		#endif
		if(!gp_socket_help[i]->init_socket())
			printf("[%d] ok\n",i);
		else
			printf("[%d] err\n",i);
	}
	while(1) sleep(100);
	return 0;
}
