#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <mutex>
#include <signal.h>

using namespace std;

class random_string{
    public:
    static string gene(){
        return gene(10);
    }
    static string gene(int length){
        string s;
        if(length < 1){
            return gene();
        } else {
            string keys {"1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};
            length++;
            while( --length ){
                unsigned seed1 = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                unsigned int seed2 = seed1 % length;
                std::minstd_rand0 g1 (seed1);  // minstd_rand0 is a standard linear_congruential_engine
                g1.discard(seed2+1);
                s += keys.at(g1() % keys.length());
            }
        }
        return s;
    }
};

class socket_help{
	public:
		socket_help(string server_ip, uint16_t server_port, uint16_t client_index)
		{
			sockfd = -1;
			ip = server_ip;
			port = server_port;
			cli_index = client_index;
			memset(&servaddr,0,sizeof(servaddr));
			terminal_id = random_string::gene(10);
			for(int i=0;i<5;i++)
				rfid_id.push_back(random_string::gene(8));
			send_msg_id = 0;
			recv_msg_id = 0;
		}
		
		~socket_help()
		{
			if(sockfd >= 0)
			{
				shutdown(sockfd, SHUT_RDWR);
				close(sockfd);
			}
		}
		
		int send_data(char * data, uint16_t len)
		{	
			std::unique_lock<std::mutex> lok(send_mutex);
			if(send(sockfd, data, len, 0) < 0)
			{
				if(errno==EINTR)
				{
					printf("[%d] send error : errno==EINTR strerror : %s, continue\n", cli_index, strerror(errno));
					return 0;
				}
				else if(errno==EAGAIN)
				{
					sleep(1);
					printf("[%d] send error : errno==EAGAIN strerror : %s, continue\n", cli_index, strerror(errno));
					return 0;
				}
				else
				{
					printf("[%d] send error : errno : %d, strerror : %s \n", cli_index, errno, strerror(errno)); 
					return 1;
				}
			}
			return 0;
		}

		static void read_thread(void *arg)
		{
			socket_help * sh = (socket_help *)arg;
			int sockfd = sh->sockfd;
			uint16_t cli_index = sh->cli_index;

			char buf[256] = {0};
			int recv_len;
			while(1)
			{
				memset(buf,0,sizeof(buf));
				if((recv_len = recv(sockfd, buf, sizeof(buf), 0)) == -1)
				{
					if(errno==EAGAIN)
					{
						printf("[%d] recv timeout\n", cli_index);
					}
					else
					{
						printf("[%d] recv error, server crash\n", cli_index);
						break;
					}
					
				}
				if(recv_len>0)
				{
					printf("[%d] recv data: ", cli_index);
					for(int i=0;i<recv_len;i++)
					{
						printf("%02x ", buf[i]);
					}
					printf("\n");
				}
				//if(unlock) send;
			}
			printf("[%d] read_thread exit\n", cli_index);
		}
		
		static void write_thread(void *arg)
		{
			socket_help * sh = (socket_help *)arg;
			char report_device_info[256] = "report_device_info";
			char heartbeat_info[256] = "heartbeat_info";
			uint16_t report_len = strlen(report_device_info);
			uint16_t heartbeat_len = strlen(heartbeat_info);
			while(1)
			{
				if(sh->send_data(report_device_info, report_len)) break;//重连
				sleep(3);
				if(sh->send_data(heartbeat_info, heartbeat_len)) break;//重连
				sleep(3);
			}
			printf("[%d] write_thread exit\n", sh->cli_index);
		}

		int init_socket()
		{
			signal(SIGPIPE,SIG_IGN);
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(port);
			
			if(inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0)
			{
				printf("[%d] inet_pton error\n", cli_index);
				return 1;
			}
			
			if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{
				printf("[%d] create socket error\n", cli_index);
				return 2;
			}
			struct timeval timeout = {3,0};
			setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
			
			if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
			{
				printf("[%d] connect error: %s(errno: %d)\n", cli_index, strerror(errno), errno);
				return 3;
			}
			
			std::thread (read_thread,this).detach();
			std::thread (write_thread,this).detach();

			return 0;
		}
	private:
		int sockfd;
		string ip;
		uint16_t port;
		uint16_t cli_index;
		struct sockaddr_in servaddr;
		std::mutex send_mutex;
		string terminal_id;
		vector<string> rfid_id;
		uint16_t send_msg_id;
		uint16_t recv_msg_id;
		
};
#endif
