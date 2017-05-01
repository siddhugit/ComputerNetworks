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
#include <arpa/inet.h>
#include <iostream>


void UdpSocket::sendMessage(int sockFd,const char* message,int msgLen,addrinfo *destAddr)
{
	sendto(sockFd, message, msgLen, 0,destAddr->ai_addr, destAddr->ai_addrlen);
}

int UdpSocket::getDestSocket(const std::string& hostName,const std::string& port,addrinfo **destAddr)
{
	const char* portNum = port.c_str();
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	memset(&servinfo,0,sizeof servinfo);
	
	//if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0) {
	if ((rv = getaddrinfo(hostName.c_str(), portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("Sender: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Sender: failed to create socket\n");
		return -1;
	}
	*destAddr = p;
	//freeaddrinfo(servinfo);
	return sockfd;
}

int UdpSocket::makeServerSocket(const std::string& port)
{
	const char* portNum = port.c_str();
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; 
	
	if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
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
	return sockfd;
}

/*void UdpSocket::init()
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
*/
