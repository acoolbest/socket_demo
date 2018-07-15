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
#include <random>
#include <vector>

using namespace std;

struct DNS_HDR
{  
  uint16_t id;
  uint16_t tag;
  uint16_t q_count;
  uint16_t ans_count;
  uint16_t auth_count;
  uint16_t add_count;
};
struct DNS_QER
{
   uint16_t type;
   uint16_t classes;
};

class domain_to_ip{
	public:
	domain_to_ip(string domain){
		str_domain = domain;
		servfd = -1;
		memset(buf,0,sizeof(buf));
		dnshdr = (DNS_HDR *)buf;
		dnshdr->id = htons(0x2000);
		dnshdr->tag = htons(0x0100);
		dnshdr->q_count = htons(1);
		dnshdr->ans_count = 0;
		strcpy(buf + sizeof(DNS_HDR) + 1, str_domain.c_str());
		char * p = buf + sizeof(DNS_HDR) + 1; 
		int i = 0;
		while (p < (buf + sizeof(DNS_HDR) + 1 + str_domain.length()))
		{
			if ( *p == '.')
			{
				*(p - i - 1) = i;
				i = 0;
			}
			else
			{
				i++;
			}
			p++;
		}
		*(p - i - 1) = i;
		DNS_QER  *dnsqer = (DNS_QER *)(buf + sizeof(DNS_HDR) + 2 + str_domain.length());
		dnsqer->classes = htons(1);
		dnsqer->type = htons(1);
	}
	~domain_to_ip(){
		if(servfd != -1){
			close(servfd);
			servfd = -1;
		}
		printf("close dns socket\n");
	}
	vector<string> get_ip(){
		vector<string> vec;// {"192.168.1.103"};
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		inet_aton(dns_server.c_str(), &servaddr.sin_addr);
		servaddr.sin_port = htons(dns_port);

		for(int i=0;i<3;i++)
		{
			if(servfd != -1){
				close(servfd);
				servfd = -1;
				sleep(3);
			}	
			if ((servfd  =  socket(AF_INET, SOCK_DGRAM, 0 ))  <   0 )
			{
				 printf( "dns create socket error!\n " );
				 continue;
			}
			
			int len = sendto(servfd, buf, sizeof(DNS_HDR) + sizeof(DNS_QER) + str_domain.length() + 2, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			if(len < 0)
			{
				printf("dns send error\n");
				continue;
			}
			socklen_t addrlen = sizeof(struct sockaddr_in);
			len = recvfrom(servfd, buf, 1024, 0, (struct sockaddr *)&servaddr, &addrlen);
			if (len < 0)
			{
				  printf("dns recv error\n");
				  continue;
			}
			if (dnshdr->ans_count == 0)
			{
				  printf("dns ack error\n");
				  continue;
			}
			else
			{
				break;
			}
		}
		if (dnshdr->ans_count == 0)
		{
			  printf("dns ack error\n");
			  return vec;
		}
		uint8_t *p = (uint8_t *)buf + sizeof(DNS_HDR) + sizeof(DNS_QER) + str_domain.length() + 2;
		printf("Ans Count = %d\n", ntohs(dnshdr->ans_count));
		for (int i = 0; i < ntohs(dnshdr->ans_count); i++)
		{
			p = p + 12;
			string ip = std::to_string(*p) + "." + std::to_string(*(p + 1)) + "." + std::to_string(*(p + 2)) + "." + std::to_string(*(p + 3));
			vec.push_back(ip);
			printf("%s ==> %u.%u.%u.%u\n", str_domain.c_str(), (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
			p = p + 4;
		}
		return vec;
	}
	private:
		char buf[1024];
		DNS_HDR  *dnshdr;
		DNS_QER  *dnsqer;
		int servfd;
		//const string dns_server = "101.198.198.198";
		const string dns_server = "8.8.8.8";
		const uint16_t dns_port = 53;
		string str_domain;
};

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
	socket_help(vector<string> server_ip, uint16_t server_port, uint16_t client_index)
	{
		sockfd = -1;
		srv_ip = "";//"103.46.128.41";//"39.106.26.66";
		vec_ip = server_ip;
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
	
	int send_data(uint8_t * data, uint16_t len)
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
		else
		{
			printf("[%d] send data: ", cli_index);
			for(uint16_t i=0;i<len;i++)
			{
				printf("%02x ", data[i]);
			}
			printf("\n");
		}
		return 0;
	}

	static void read_thread(void *arg)
	{
		socket_help * sh = (socket_help *)arg;
		uint8_t buf[256] = {0};
		int recv_len;
		
		uint16_t function_code = 0;
		uint16_t data_len = 0;
		uint16_t msg_id = 0;
		uint16_t terminal_len = 0;
		uint8_t terminal_id[32] = {0};
		uint16_t rfid_len = 0;
		uint8_t rfid_id[32] = {0};
		uint8_t addr = 0;

		while(1)
		{
			memset(buf,0,sizeof(buf));
			if((recv_len = recv(sh->sockfd, buf, sizeof(buf), 0)) == -1)
			{
				if(errno==EAGAIN)
				{
					;//printf("[%d] recv timeout\n", sh->cli_index);
				}
				else
				{
					printf("[%d] recv error, server crash\n", sh->cli_index);
					break;
				}
				
			}
			if(recv_len>0)
			{
				printf("[%d] recv data: ", sh->cli_index);
				for(int i=0;i<recv_len;i++)
				{
					printf("%02x ", buf[i]);
				}
				printf("\n");
				#if 0
				00 01 00 1E 00 00 00 0A 38 70 
				71 7A 41 6E 79 64 51 68 00 08 
				66 63 31 4E 75 56 4C 4B 00 01 
				#endif
				uint16_t * p16 = (uint16_t *)buf;
				uint8_t *p = NULL;
				function_code = htons(*p16++);
				if(function_code != 0x0101) continue;
				data_len = htons(*p16++);
				msg_id = htons(*p16++);
				
				terminal_len = htons(*p16++);
				p=(uint8_t *)p16;
				memcpy(terminal_id, p, terminal_len);
				terminal_id[terminal_len] = '\0';
				p+=terminal_len;
				p16=(uint16_t *)p;
				
				rfid_len = htons(*p16++);
				p=(uint8_t *)p16;
				memcpy(rfid_id, p, rfid_len);
				rfid_id[rfid_len] = '\0';
				p+=rfid_len;
				addr = *p++;
				
				printf("0x%04x, 0x%04x, 0x%04x, 0x%04x, %s, 0x%04x, %s, 0x%02x\n", function_code, data_len, msg_id, terminal_len, terminal_id, rfid_len, rfid_id, addr);
				
				function_code = ((uint16_t)buf[0]) << 8 | buf[1];
				if(function_code != 0x0101) continue;
				msg_id = ((uint16_t)buf[4]) << 8 | buf[5];
				terminal_len = ((uint16_t)buf[6]) << 8 | buf[7];
				//terminal_id[32] = {0};
				memcpy(terminal_id, &buf[8], terminal_len);
				terminal_id[terminal_len] = '\0';
				rfid_len = ((uint16_t)buf[8+terminal_len]) << 8 | buf[8+terminal_len+1];
				rfid_id[32] = {0};
				memcpy(rfid_id, &buf[8+terminal_len+1+1], rfid_len);
				rfid_id[rfid_len] = '\0';
				addr = buf[8+terminal_len+1+1+rfid_len];
				
				if(terminal_len == sh->terminal_id.length()
					&& !memcmp(terminal_id, sh->terminal_id.c_str(), terminal_len)
					&& addr < sh->rfid_id.size()
					&& rfid_len == sh->rfid_id[addr].length()
					&& !memcmp(rfid_id, sh->rfid_id[addr].c_str(), rfid_len))
				{
					buf[1] = 0x01;
					buf[recv_len] = 0x00;
					buf[3] = recv_len+1;
					printf("[%d] read_thread msg_id[%04x], rfid_len[%04x], rfid_id[index %d][%s] addr[index %d][%02x], result_code[0x00]\n", 
						sh->cli_index,
						msg_id,
						rfid_len,
						8+terminal_len+1+1,
						rfid_id,
						8+terminal_len+1+1+rfid_len,
						addr);
				}
				else 
				{
					buf[1] = 0x01;
					buf[recv_len] = 0x01;
					buf[3] = recv_len+1;

					printf("[%d] read_thread function_code[%04x],msg_id[%04x], terminal_len[%04x], terminal_id[%s], rfid_len[%04x], rfid_id[index %d][%s] addr[index %d][%02x], result_code[0x01]\n", 
						sh->cli_index,
						function_code,
						msg_id,
						terminal_len,
						terminal_id,
						rfid_len,
						8+terminal_len+1+1,
						rfid_id,
						8+terminal_len+1+1+rfid_len,
						addr);
				}
				
				if(sh->send_data(buf, recv_len+1+4)) break;//重连
			}
		}
		printf("[%d] read_thread exit\n", sh->cli_index);
	}
	
	static void write_thread(void *arg)
	{
		socket_help * sh = (socket_help *)arg;
		#if 0
		char report_device_info[256] = "report_device_info";
		char heartbeat_info[256] = "heartbeat_info";
		uint16_t report_len = strlen(report_device_info);
		uint16_t heartbeat_len = strlen(heartbeat_info);
		#endif
		
		uint8_t send_buf[256] = {0};
		uint8_t *p = NULL;
		uint16_t *p16 = NULL;
		uint16_t send_len = 0;

		uint8_t rc522_index_start = '0';
		uint8_t rc522_state_ok = '1';
		uint8_t rc522_state_err = '0';
		printf("[%d] terminal_id[%s]\n", sh->cli_index, sh->terminal_id.c_str());
		
		int input_num;
		while(1)
		{
			std::cout << "pls input a num, 2 is report device info, 3 is heartbeat\n";
			std::cin >> input_num;
			
			if(input_num == 2)
			{
				int random_addr = time(NULL)%(sh->rfid_id.size());
				printf("[%d] rfid_id[index %d][%s]\n", sh->cli_index, random_addr, sh->rfid_id[random_addr].c_str());
				#if 0
				01 02 00 1D 00 00 00 0A 47 52 
				7A 70 44 50 57 70 4E 6B 00 08 
				68 44 51 35 6A 74 6A 30 00 
				#endif
				p16 = (uint16_t *)send_buf;
				*p16++ = htons(0x0102);
				*(++p16)++ = sh->send_msg_id;
				sh->send_msg_id++;
				*p16++ = htons(sh->terminal_id.length());
				p=(uint8_t *)p16;
				memcpy(p, sh->terminal_id.c_str(), sh->terminal_id.length());
				p += sh->terminal_id.length();
				p16=(uint16_t *)p;
				*p16++ = htons(sh->rfid_id[random_addr].length());
				p=(uint8_t *)p16;
				memcpy(p, sh->rfid_id[random_addr].c_str(), sh->rfid_id[random_addr].length());
				p += sh->rfid_id[random_addr].length();
				*p++ = random_addr;
				send_len = p-send_buf;
				send_buf[3] = send_len-4;
				
				if(sh->send_data(send_buf, send_len)) break;//重连
				#if 0
				p = send_buf;
				*p++ = 0x01;
				*p++ = 0x02;
				*(p += 2)++ = (uint8_t)(sh->send_msg_id >> 8);
				*p++ = (uint8_t)sh->send_msg_id;
				sh->send_msg_id++;
				*p++ = 0x00;
				*p++ = sh->terminal_id.length();
				memcpy(p, sh->terminal_id.c_str(), sh->terminal_id.length());
				p += sh->terminal_id.length();
				*p++ = 0x00;
				*p++ = sh->rfid_id[random_addr].length();
				memcpy(p, sh->rfid_id[random_addr].c_str(), sh->rfid_id[random_addr].length());
				p += sh->rfid_id[random_addr].length();
				*p++ = random_addr;
				
				send_len = p-send_buf;
				send_buf[3] = send_len-4;

				if(sh->send_data(send_buf, send_len)) break;//重连
				#endif
				printf("[%d] write_thread report device info\n", sh->cli_index);
				//sleep(3);
			}
			else if(input_num == 3)
			{
				#if 0
				01 03 00 1D 00 01 00 0A 47 52 
				7A 70 44 50 57 70 4E 6B 05 30 
				31 31 30 32 31 33 30 34 31
				#endif
				p = send_buf;
				*p++ = 0x01;
				*p++ = 0x03;
				*(p += 2)++ = (uint8_t)(sh->send_msg_id >> 8);
				*p++ = (uint8_t)sh->send_msg_id;
				sh->send_msg_id++;
				*p++ = 0x00;
				*p++ = sh->terminal_id.length();
				memcpy(p, sh->terminal_id.c_str(), sh->terminal_id.length());
				p += sh->terminal_id.length();
				
				*p++ = sh->rfid_id.size();
				for(uint16_t i=0;i<sh->rfid_id.size();i++)
				{
					*p++ = i+ rc522_index_start;
					if(i%2==0) *p++ = rc522_state_ok;
					else *p++ = rc522_state_err;
				}
				
				send_len = p-send_buf;
				send_buf[3] = send_len-4;
				if(sh->send_data(send_buf, send_len)) break;//重连
				printf("[%d] write_thread heartbeat\n", sh->cli_index);
				//sleep(3);
			}
			else
			{
				printf("[%d] write_thread error input num %d\n", sh->cli_index, input_num);
			}
		}
		printf("[%d] write_thread exit\n", sh->cli_index);
	}

	int init_socket()
	{
		signal(SIGPIPE,SIG_IGN);
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(port);
		#if 1
		for(auto ip:vec_ip){
			if(inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0)
			{
				printf("[%d] inet_pton error\n", cli_index);
				continue;
			}
			
			if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{
				printf("[%d] create socket error\n", cli_index);
				continue;
			}
			struct timeval timeout = {3,0};
			setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
			
			if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
			{
				printf("[%d] connect error: %s(errno: %d)\n", cli_index, strerror(errno), errno);
				continue;
			}
			else
			{
				srv_ip = ip;
				printf("get server ip[%s]\n", srv_ip.c_str());
			}
		}
		#endif
		if(!srv_ip.length())
		{
			return 1;
		}
		std::thread (read_thread,this).detach();
		std::thread (write_thread,this).detach();

		return 0;
	}
	private:
	int sockfd;
	string srv_ip;
	vector<string> vec_ip;
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
