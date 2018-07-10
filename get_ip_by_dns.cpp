//208l8w1838.51mypc.cn
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define SRV_PORT 53
typedef unsigned short U16;
const char srv_ip[] = "208.67.222.222";
typedef struct _DNS_HDR
{  
  U16 id;
  U16 tag;
  U16 numq;
  U16 numa;
  U16 numa1;
  U16 numa2;
}DNS_HDR;
typedef struct _DNS_QER
{
   U16 type;
   U16 classes;
}DNS_QER;
int main(string domain)
{
	char buf[1024] = {0};

	DNS_HDR  *dnshdr = (DNS_HDR *)buf;
    dnshdr->id = 1;
    dnshdr->tag = htons(0x0100);
    dnshdr->numq = htons(1);
    dnshdr->numa = 0;
   
    strcpy(buf + sizeof(DNS_HDR) + 1, domain.c_str());
    char * p = buf + sizeof(DNS_HDR) + 1; 
	int i = 0;
    while (p < (buf + sizeof(DNS_HDR) + 1 + domain.length()))
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
       
    DNS_QER  *dnsqer = (DNS_QER *)(buf + sizeof(DNS_HDR) + 2 + domain.length());
    dnsqer->classes = htons(1);
    dnsqer->type = htons(1);
	
	
	char dns_server[] = "114.114.114.114";
	uint16_t dns_port = 53;
    struct sockaddr_in servaddr;
	
	bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_aton(dns_server, &servaddr.sin_addr);
    servaddr.sin_port = htons(dns_port);
	
	int servfd = -1;
    if ((servfd  =  socket(AF_INET, SOCK_DGRAM, 0 ))  <   0 )
    {
         printf( "dns create socket error!\n " );
         return "";
    }
    
    int len = sendto(servfd, buf, sizeof(DNS_HDR) + sizeof(DNS_QER) + domain.length() + 2, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(len < 0)
	{
		printf("dns send error\n");
        return "";
	}
    socklen_t addrlen = sizeof(struct sockaddr_in);
    len = recvfrom(servfd, buf, BUF_SIZE, 0, (struct sockaddr *)&servaddr, &addrlen);
    if (len < 0)
    {
          printf("dns recv error\n");
          return "";
    }
    if (dnshdr->numa == 0)
    {
          printf("dns ack error\n");
          return "";
    }
    p = buf + len -4;
	
	string s = std::to_string(*p) + "." + std::to_string(*(p + 1)) + "." + std::to_string(*(p + 2)) + "." + std::to_string(*(p + 3));
    //printf("%s ==> %u.%u.%u.%u\n", argv[1], (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
 
    close(servfd);
    return 0;
}