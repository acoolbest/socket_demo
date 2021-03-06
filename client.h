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
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

using namespace std;

//#define ZHZQ

//#define RANDOM_DATA

#define TERMINAL_LEN 8
#define RFID_SIZE 8
#define RFID_LEN 8
#define CLIENT_COUNT 1000

#define MAX_PATH 256
#define MAXLEN_LOGRECORD 1024

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

void get_cur_time(char* cur_time, bool btype)
{
	time_t lTime;
	struct tm *ptmTime;

	time(&lTime);
	ptmTime = localtime(&lTime);
	if (btype)
	{
		strftime(cur_time, 32, "%Y/%m/%d %H:%M:%S", ptmTime);
	}
	else
	{
		strftime(cur_time, 32, "%Y%m%d%H%M%S", ptmTime);
	}

}
string get_cur_work_folder()
{
	char sz_file_path[MAX_PATH] = {0};
	if(getcwd(sz_file_path, MAX_PATH))
		return string(sz_file_path);
	else
		return "";
}

bool check_folder(const char* p_path_name)
{
	char  dir_name[MAX_PATH] = {0};
	strcpy(dir_name, p_path_name);
	int i = 0, len = strlen(dir_name);

	if (dir_name[len - 1] != '/')
	{
		strcat(dir_name, "/");
	}

	len = strlen(dir_name);

	for (i = 1; i < len; i++)
	{
		if (dir_name[i] == '/')
		{
			dir_name[i] = 0;
			if (access(dir_name, F_OK) != 0)
			{
				if (mkdir(dir_name, 0777) == -1)
				{
					return false;
				}
			}
			dir_name[i] = '/';
		}
	}

	return (access(p_path_name, F_OK) == 0) ? true : false;
}

bool append_to_file(const char* file_name, const char* content, size_t file_length)
{
	FILE* fp = NULL;

	try
	{
		fp = fopen(file_name, "a+b");
		if (fp == NULL)
			return false;
		
		fwrite(content, 1, file_length, fp);
		fclose(fp);
	}
	catch (...)
	{
		fclose(fp);
		return false;
	}

	return true;
}

void get_cur_date(char* cur_date)
{
	time_t lTime;
	struct tm *ptmTime;

	time(&lTime);

	ptmTime = localtime(&lTime);

	strftime(cur_date, 16, "%Y%m%d", ptmTime);
}

void write_log(const char *fm, ...)
{
	int iSize = 0;

	char buff[MAXLEN_LOGRECORD] = { 0 };

	int i = 0;
	char cur_time[32] = { 0 };
	get_cur_time(cur_time, true);
	i = sprintf(buff, "%s gettid:%ld\t", cur_time, syscall(SYS_gettid));
	int m_maxlen = MAXLEN_LOGRECORD - (i + 3);
	va_list args;
	va_start(args, fm);
	iSize = vsprintf(buff + i, fm, args);
	va_end(args);

	if (iSize > m_maxlen || iSize < 0)
	{
		memset(buff, 0, MAXLEN_LOGRECORD);
		iSize = sprintf(buff, "%s\t", cur_time);
		iSize += sprintf(buff + iSize, "-- * --\n");
	}
	else
	{
		iSize += i;
		if (iSize < MAXLEN_LOGRECORD - 2)
		{
			buff[iSize] = 13;
			buff[iSize+1] = 10;
			iSize+=2;
		}
	}

	char curDate[20] = { 0 };
	char fileName[300] = { 0 };
	get_cur_date(curDate);
	int  j = 0;
	j = sprintf(fileName, "%s/%s", get_cur_work_folder().c_str(), "logs");
	check_folder(fileName);

	j += sprintf(fileName + j, "/%s_%s%s", "tcp_client", curDate, ".log");

	append_to_file(fileName, buff, iSize);
	//cout << fileName << endl;
	return;
}

void print_com_data_to_log(const char * str, uint8_t * data, uint16_t len)
{
	char buffer[2048] = { 0 };
	for (int i = 0; i < len; i++)
	{
		sprintf(buffer + i * 3, " %02X", data[i]);
	}
	write_log("%s%s", str, buffer);
}


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
		write_log("close dns socket\n");
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
				 fprintf(stderr, "dns create socket error!\n ");
				 write_log("dns create socket error!\n ");
				 continue;
			}

			timeval tv = {3, 0};
			if(setsockopt(servfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval))!=0) {
				continue;
			}

			int len = sendto(servfd, buf, sizeof(DNS_HDR) + sizeof(DNS_QER) + str_domain.length() + 2, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			if(len < 0)
			{
				fprintf(stderr, "dns send error\n");
				write_log("dns send error\n");
				continue;
			}
			socklen_t addrlen = sizeof(struct sockaddr_in);
			len = recvfrom(servfd, buf, 1024, 0, (struct sockaddr *)&servaddr, &addrlen);
			if (len < 0)
			{
				  fprintf(stderr, "dns recv error\n");
				  write_log("dns recv error\n");
				  continue;
			}
			if (dnshdr->ans_count == 0)
			{
				  fprintf(stderr, "dns ack error\n");
				  write_log("dns ack error\n");
				  continue;
			}
			else
			{
				break;
			}
		}
		if (dnshdr->ans_count == 0)
		{
			  fprintf(stderr, "dns ack error\n");
			  write_log("dns ack error\n");
			  return vec;
		}
		uint8_t *p = (uint8_t *)buf + sizeof(DNS_HDR) + sizeof(DNS_QER) + str_domain.length() + 2;
		printf("Ans Count = %d\n", ntohs(dnshdr->ans_count));
		write_log("Ans Count = %d\n", ntohs(dnshdr->ans_count));
		for (int i = 0; i < ntohs(dnshdr->ans_count); i++)
		{
			p = p + 12;
			string ip = std::to_string(*p) + "." + std::to_string(*(p + 1)) + "." + std::to_string(*(p + 2)) + "." + std::to_string(*(p + 3));
			vec.push_back(ip);
			printf("%s ==> %u.%u.%u.%u\n", str_domain.c_str(), (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
			write_log("%s ==> %u.%u.%u.%u\n", str_domain.c_str(), (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
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
		srv_ip = "";
		vec_ip = server_ip;
		port = server_port;
		cli_index = client_index;
		memset(&servaddr,0,sizeof(servaddr));
		
		#ifdef RANDOM_DATA
		terminal_id = random_string::gene(TERMINAL_LEN);
		for(int i=0;i<RFID_SIZE;i++)
			rfid_id.push_back(random_string::gene(RFID_LEN));
		#else
		terminal_id = "terminal";
		for(int i=0;i<RFID_SIZE;i++)
			rfid_id.push_back("rfid_id"+std::to_string(i));
		#endif
		
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
		char str_print[1024] = "";
		if(send(sockfd, data, len, 0) < 0)
		{
			if(errno==EINTR)
			{
				fprintf(stderr, "[%d] send error : errno==EINTR strerror : %s, continue\n", cli_index, strerror(errno));
				write_log("[%d] send error : errno==EINTR strerror : %s, continue\n", cli_index, strerror(errno));
				return 0;
			}
			else if(errno==EAGAIN)
			{
				sleep(1);
				fprintf(stderr, "[%d] send error : errno==EAGAIN strerror : %s, continue\n", cli_index, strerror(errno));
				write_log("[%d] send error : errno==EAGAIN strerror : %s, continue\n", cli_index, strerror(errno));
				return 0;
			}
			else
			{
				fprintf(stderr, "[%d] send error : errno : %d, strerror : %s \n", cli_index, errno, strerror(errno));
				write_log("[%d] send error : errno : %d, strerror : %s \n", cli_index, errno, strerror(errno));
				return 1;
			}
		}
		else
		{
			sprintf(str_print, "[%d] send data: ", cli_index);
			print_com_data_to_log(str_print, data, len);
			#if 0
			printf("[%d] send data: ", cli_index);
			for(uint16_t i=0;i<len;i++)
			{

				printf("%02x ", data[i]);
			}
			printf("\n");
			#endif
		}
		return 0;
	}

	static void read_thread(socket_help * sh)
	{
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
		
		char str_print[1024] = "";

		while(1)
		{
			memset(buf,0,sizeof(buf));
			if((recv_len = recv(sh->sockfd, buf, sizeof(buf), 0)) == -1)
			{
				if(errno==EAGAIN)
				{
					;//fprintf(stderr, "[%d] recv timeout\n", sh->cli_index);
				}
				else
				{
					fprintf(stderr, "[%d] recv error, server crash\n", sh->cli_index);
					write_log("[%d] recv error, server crash\n", sh->cli_index);
					break;
				}
				
			}
			if(recv_len>0)
			{
				sprintf(str_print, "[%d] recv data: ", sh->cli_index);
				print_com_data_to_log(str_print, buf, recv_len);
				#if 0
				for(int i=0;i<recv_len;i++)
				{
					write_log("%02x ", buf[i]);
				}
				write_log("\n");
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
				
				write_log("[%d] 0x%04x, 0x%04x, 0x%04x, 0x%04x, %s, 0x%04x, %s, 0x%02x\n", sh->cli_index, function_code, data_len, msg_id, terminal_len, terminal_id, rfid_len, rfid_id, addr);
				
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
				
				uint8_t result_code = 0;
				if(terminal_len == sh->terminal_id.length()
					&& !memcmp(terminal_id, sh->terminal_id.c_str(), terminal_len)
					&& addr < sh->rfid_id.size()
					&& rfid_len == sh->rfid_id[addr].length()
					&& !memcmp(rfid_id, sh->rfid_id[addr].c_str(), rfid_len))
				{
					result_code = 0x00;
					buf[1] = 0x01;
					buf[recv_len] = 0x00;
					buf[3] = recv_len+1;
					
					write_log("[%d] read_thread msg_id[%04x], rfid_len[%04x], rfid_id[index %d][%s] addr[index %d][%02x], result_code[0x00]\n", 
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
					result_code = 0x01;
					buf[1] = 0x01;
					buf[recv_len] = 0x01;
					buf[3] = recv_len+1;
					
					write_log("[%d] read_thread function_code[%04x],msg_id[%04x], terminal_len[%04x], terminal_id[%s], rfid_len[%04x], rfid_id[index %d][%s] addr[index %d][%02x], result_code[0x01]\n", 
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
				uint8_t send_buf[256] = {0};
				p16 = (uint16_t *)send_buf;
				*p16++ = htons(0x0101);
				*(++p16)++ = htons(msg_id);

				*p16++ = htons(sh->terminal_id.length());
				p=(uint8_t *)p16;
				memcpy(p, sh->terminal_id.c_str(), sh->terminal_id.length());
				p += sh->terminal_id.length();
				p16=(uint16_t *)p;
				*p16++ = htons(sh->rfid_id[addr].length());
				p=(uint8_t *)p16;
				memcpy(p, sh->rfid_id[addr].c_str(), sh->rfid_id[addr].length());
				p += sh->rfid_id[addr].length();
				*p++ = addr;
				*p++ = result_code;
				uint16_t send_len = p-send_buf;
				send_buf[3] = send_len-4;
				
				if(sh->send_data(send_buf, send_len)) break;//reconnect
			}
		}
		printf("[%d] read_thread exit\n", sh->cli_index);
		write_log("[%d] read_thread exit\n", sh->cli_index);
	}

	static void send_heartbeat(socket_help * sh, bool while_flag)
	{
		static uint8_t u8_flag = 0;
		uint8_t rc522_index_start = '0';
		uint8_t rc522_state_ok = '1';
		uint8_t rc522_state_err = '0';
		
		uint8_t send_buf[256] = {0};
		uint8_t *p = NULL;
		uint16_t *p16 = NULL;
		uint16_t send_len = 0;
		
		do{
			p16 = (uint16_t *)send_buf;
			*p16++ = htons(0x0103);
			*(++p16)++ = htons(sh->send_msg_id);
			sh->send_msg_id++;
			*p16++ = htons(sh->terminal_id.length());
			p=(uint8_t *)p16;
			memcpy(p, sh->terminal_id.c_str(), sh->terminal_id.length());
			p += sh->terminal_id.length();
			*p++ = sh->rfid_id.size();
			
			u8_flag^=1;
			for(uint8_t i=0;i<sh->rfid_id.size();i++)
			{
				*p++ = i+ rc522_index_start;
				if(u8_flag) *p++ = rc522_state_ok;
				else{
					if(i%2==0) *p++ = rc522_state_ok;
					else *p++ = rc522_state_err;
				}
			}
			
			send_len = p-send_buf;
			send_buf[3] = send_len-4;
			if(sh->send_data(send_buf, send_len)) break;//reconnect
			write_log("[%d] write_thread heartbeat\n", sh->cli_index);
			
			if(while_flag) sleep(60);
		}while(while_flag);
	}

	static void send_report_device(socket_help * sh, bool while_flag, uint8_t rfid_index)
	{
		uint8_t send_buf[256] = {0};
		uint8_t *p = NULL;
		uint16_t *p16 = NULL;
		uint16_t send_len = 0;

		uint8_t random_addr = 0;
		
		do{
			random_addr = while_flag ? (time(NULL)%(sh->rfid_id.size())) : rfid_index;
			if(random_addr >= sh->rfid_id.size())
			{
				fprintf(stderr, "[%d] rfid_id[index %d] overflow!\n", sh->cli_index, random_addr);
				write_log("[%d] rfid_id[index %d] overflow!\n", sh->cli_index, random_addr);
				break;
			}
			write_log("[%d] rfid_id[index %d][%s]\n", sh->cli_index, random_addr, sh->rfid_id[random_addr].c_str());

			p16 = (uint16_t *)send_buf;
			*p16++ = htons(0x0102);
			*(++p16)++ = htons(sh->send_msg_id);
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
			
			if(sh->send_data(send_buf, send_len)) break;//reconnect

			write_log("[%d] write_thread report device info\n", sh->cli_index);
			
			if(while_flag) sleep(10);
		}while(while_flag);
	}

	int get_line_number_to_int()
	{
		string input_str = "";
		int input_num = 0;
		std::cin >> input_str;
		try{
			input_num = std::stoi(input_str, NULL);
		}
		catch(std::invalid_argument &ia)
		{
			write_log("[%d] Invalid_argument : [%s]\n", cli_index, ia.what());
			input_num = -1;
			fprintf(stderr, "[%d] error input str [%s]\n", cli_index, input_str.c_str());
		}
		return input_num;
	}
	
	static void write_thread(socket_help * sh)
	{
		std::thread (send_heartbeat, sh, true).detach();

		if(sh->stress_test > 1) 
		{
			write_log("[%d] terminal_id[%s] stress test\n", sh->cli_index, sh->terminal_id.c_str());
			std::thread t1 (send_report_device, sh, true, 0);
			t1.join();
		}
		else
		{
			write_log("[%d] terminal_id[%s] debug test\n", sh->cli_index, sh->terminal_id.c_str());

			int input_num = 0;

			while(1)
			{
				std::cout << "pls input a num, 2 is report device info, 3 is heartbeat\n";
				input_num = sh->get_line_number_to_int();

				if(input_num == 2)
				{
					while(1)
					{
						std::cout << "pls input a num, range from 0 to " << sh->rfid_id.size()-1 << ", stand for the rfid_index\n";
						std::cout << "if the input num overflow the range, it will quit this module\n";
						input_num = sh->get_line_number_to_int();
						if(input_num < 0) continue;

						if(static_cast<uint8_t>(input_num) >= sh->rfid_id.size())
						{
							fprintf(stderr, "[%d] input_num [%d], overflow! quit this module\n", sh->cli_index, input_num);
							input_num = 0;
							break;
						}

						std::thread (send_report_device, sh, false, input_num).detach();
					}
				}
				else if(input_num == 3) std::thread (send_heartbeat, sh, false).detach();
				else fprintf(stderr, "[%d] write_thread error input_num [%d]\n", sh->cli_index, input_num);
			}
		}

		printf("[%d] write_thread exit\n", sh->cli_index);
		write_log("[%d] write_thread exit\n", sh->cli_index);
	}

	int init_socket(int argc)
	{
		stress_test = argc;
		signal(SIGPIPE,SIG_IGN);
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(port);
		#if 1
		for(auto ip:vec_ip){
			if(inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0)
			{
				fprintf(stderr, "[%d] inet_pton error\n", cli_index);
				write_log("[%d] inet_pton error\n", cli_index);
				continue;
			}
			
			if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{
				fprintf(stderr, "[%d] create socket error\n", cli_index);
				write_log("[%d] create socket error\n", cli_index);
				continue;
			}
			struct timeval timeout = {3,0};
			setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
			
			if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
			{
				fprintf(stderr, "[%d] connect error: %s(errno: %d)\n", cli_index, strerror(errno), errno);
				write_log("[%d] connect error: %s(errno: %d)\n", cli_index, strerror(errno), errno);
				continue;
			}
			else
			{
				srv_ip = ip;
				printf("get server ip[%s]\n", srv_ip.c_str());
				write_log("get server ip[%s]\n", srv_ip.c_str());
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
	int stress_test;
		
};
#endif
