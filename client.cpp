#include "client.h"
//g++ client.cpp -lpthread -g -Wall -std=c++11

static socket_help* gp_socket_help[CLIENT_COUNT]={NULL};

vector<string> get_ip(string domain)
{
	domain_to_ip dti(domain);
	vector<string> vec = dti.get_ip();
	return vec;
}
//39.106.26.66:9503
//208l8w1838.51mypc.cn
//103.46.128.41:57093 48438	192.168.0.67:8088
int main(int argc, char * argv[])
{
	if(argc < 2)
	{
		printf("usage:\n\topen 10 tcp connections: ./a.out 10\n");
		return -1;
	}
	#ifdef ZHZQ
	vector<string> vec {"103.46.128.41"};
	#else
	vector<string> vec {"39.106.26.66"};
	#endif
	for(uint16_t i=0;i<atoi(argv[1]);i++)
	{
		#ifdef ZHZQ
		gp_socket_help[i] = new socket_help(vec, 57093, i);
		//gp_socket_help[i] = new socket_help(get_ip("208l8w1838.51mypc.cn"), 48438, i);
		#else
		gp_socket_help[i] = new socket_help(vec, 9503, i);
		#endif
		if(!gp_socket_help[i]->init_socket())
		{
			printf("[%d] ok\n",i);
			write_log("[%d] ok\n",i);
		}
		else
		{
			printf("[%d] err\n",i);
			write_log("[%d] err\n",i);
		}
		//sleep(1);
	}
	while(1) sleep(100);
	return 0;
}
