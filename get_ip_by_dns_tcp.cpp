//g++ get_ip_by_dns_tcp.cpp -o tcpdns -std=c++11
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

typedef unsigned short U16;
const char srv_ip[] = "8.8.8.8";
#define RET_OK    0
#define RET_ERROR -1
/*typedef struct _DNS_HDR
{  
    U16 id;
    U16 tag;
    U16 numq;
    U16 numa;
    U16 numa1;
    U16 numa2;
}DNS_HDR;*/
typedef struct
{
unsigned short id;       // identification number
unsigned char rd :1;     // recursion desired
unsigned char tc :1;     // truncated message
unsigned char aa :1;     // authoritive answer
unsigned char opcode :4; // purpose of message
unsigned char qr :1;     // query/response flag
unsigned char rcode :4;  // response code
unsigned char cd :1;     // checking disabled
unsigned char ad :1;     // authenticated data
unsigned char z :1;      // its z! reserved
unsigned char ra :1;     // recursion available
unsigned short q_count;  // number of question entries
unsigned short ans_count; // number of answer entries
unsigned short auth_count; // number of authority entries
unsigned short add_count; // number of resource entries
}DNS_HDR;
/*typedef struct _DNS_QER
{
    U16 type;
    U16 classes;
}DNS_QER;*/
typedef struct
{
unsigned short type;
unsigned short classes;
}DNS_QES;


class tcp_client
{
public:
	tcp_client()
	{
		socket_fd = -1;
	}
	~tcp_client()
	{
		if(socket_fd != -1) close(socket_fd);
	}
	int tcp_client_init()
	{
		if( (socket_fd = socket(AF_INET,SOCK_STREAM,0)) < 0 ) {
                printf("create socket error: %s(errno:%d)\n)",strerror(errno),errno);
               return -1;
        }
 
        memset(&server_addr,0,sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
 
        if( inet_pton(AF_INET,server_ip,&server_addr.sin_addr) <=0 ) {
                printf("inet_pton error for %s\n",server_ip);
                return -1;
        }
 
        if( connect(socket_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0) {
                printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
                return -1;
        }
		 return 0;
	}
	int tcp_client_send(uint8_t *message, uint16_t len)
	{
		if( send(socket_fd, message, len, 0) < 0 ) {
                printf("send message error\n");
                return -1;
        }
		return 0;
	}
	int tcp_client_recv(uint8_t *message, uint16_t len)
	{
		return recv(socket_fd, message, len, 0);
	}
	
private:
	int socket_fd;
	char message[4096];
	struct sockaddr_in server_addr;
	const char server_ip[64] = "8.8.8.8";
	const uint16_t server_port = 53;
};

int main(int argc, char **argv)
{
    unsigned char buff[1024];
    unsigned char *buf = buff + 2;
    unsigned char *p;
    int len, i;
    DNS_HDR  *dnshdr = (DNS_HDR *)buf;
    DNS_QES  *dnsqes = NULL;
	
	tcp_client tc;
	
    if (RET_ERROR == tc.tcp_client_init())
    {
        printf("Conn Error!\n");
        return -1;
    }
    else
    {
        printf("Conn OK!\n");
    }
   
    memset(buff, 0, 1024);
    dnshdr->id = htons(0x2000);//(U16)1;
    dnshdr->qr = 0;
    dnshdr->opcode = 0;
    dnshdr->aa = 0;
    dnshdr->tc = 0;
    dnshdr->rd = 1;
    dnshdr->ra = 1;
    dnshdr->z  = 0;
    dnshdr->ad = 0;
    dnshdr->cd = 0;
    dnshdr->rcode = 0;
    dnshdr->q_count = htons(1);
    dnshdr->ans_count = 0;
    dnshdr->auth_count = 0;
    dnshdr->add_count = 0;
    strcpy((char *)buf + sizeof(DNS_HDR) + 1, argv[1]);
    p = buf + sizeof(DNS_HDR) + 1; i = 0;
    while (p < (buf + sizeof(DNS_HDR) + 1 + strlen(argv[1])))
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
                                   
    dnsqes = (DNS_QES *)(buf + sizeof(DNS_HDR) + 2 + strlen(argv[1]));
    dnsqes->classes = htons(1);
    dnsqes->type = htons(1);
    buff[0] = 0; buff[1] = sizeof(DNS_HDR) + sizeof(DNS_QES) + strlen(argv[1]) + 2;
    if (RET_ERROR == tc.tcp_client_send(buff, sizeof(DNS_HDR) + sizeof(DNS_QES) + strlen(argv[1]) + 4))
    {
        printf("Send Error!\n");
        return -1;
    }
    else
    {
        printf("Send OK!\n");
    }
   
    len = tc.tcp_client_recv(buff, 1024);
    if (len < 0)
    {
        printf("Recv Error!\n");
        return -1;
    }
    else
    {
        printf("Recv OK!\n");
    }
   
    if (dnshdr->rcode !=0 || dnshdr->ans_count == 0)
    {
        printf("Ack Error\n");
        return -1;
    }
    p = buff + 2 + sizeof(DNS_HDR) + sizeof(DNS_QES) + strlen(argv[1]) + 2;
    printf("Ans Count = %d\n", ntohs(dnshdr->ans_count));
    for (i = 0; i < ntohs(dnshdr->ans_count); i++)
    {
        p = p + 12;
        printf("%s ==> %u.%u.%u.%u\n", argv[1], (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
        p = p + 4;
    }
    return 0;
}
