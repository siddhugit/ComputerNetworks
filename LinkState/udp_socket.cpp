#include "udp_socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>

extern int globalMyID;
int globalSocketUDP;
struct sockaddr_in globalNodeAddrs[256];
extern struct timeval globalLastHeartbeat[256];

void UdpSocket::init()
{
	for(int i=0;i<256;i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(7777);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);
	}	
}

void UdpSocket::hackyBroadcast(const char* buf, int length)
{
	int i;
	for(i=0;i<256;i++){
		if(i != globalMyID){ //(although with a real broadcast you would also get the packet yourself)
			sendto(globalSocketUDP, buf, length, 0,(struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
		}
	}
}

int UdpSocket::makeServerSocket(const std::string& port,int nodeId)
{
	/*char tempIp[100];
	sprintf(tempIp, "10.1.1.%d", nodeId);
	std::string myIp = tempIp;
	const char* portNum = port.c_str();
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	//int numbytes;
	//struct sockaddr_storage their_addr;
	//char buf[MAXBUFLEN]ls_router.cpp:15:21: error: invalid conversion from ‘char*’ to ‘int’ [-fpermissive]
	//socklen_t addr_len;
	//char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	//if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0) {
	if ((rv = getaddrinfo(myIp.c_str(), portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);
	globalSocketUDP = sockfd;*/
	if((globalSocketUDP=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}
	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);	
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr);
	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}
	return globalSocketUDP;
}

void UdpSocket::sendMessage(int nodeId,const char* message,int msgLen)
{
	/*std::string hostIp = "10.1.1." + std::to_string(nodeId);
	const char* portNum = port.c_str();
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostIp.c_str(), portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}
	if (p != NULL) {
		if ((numbytes = sendto(sockfd, message, msgLen, 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
		}
		freeaddrinfo(servinfo);
		close(sockfd);
	}*/
	sendto(globalSocketUDP, message, msgLen, 0,(struct sockaddr*)&globalNodeAddrs[nodeId], sizeof(globalNodeAddrs[nodeId]));
}
