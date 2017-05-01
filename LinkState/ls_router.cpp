#include "udp_socket.h"
#include "commands.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <sstream>

#define MYPORT "7777"
#define MAXBUFLEN 1024

int globalMyID;
struct timeval globalLastHeartbeat[256];

const char* logFileName;

extern Topology NetworkTopology;
extern int globalSocketUDP;

void* processReceieved(void * args)
{
	return 0;
}

int main(int argc,char **argv)
{
	globalMyID = atoi(argv[1]);
	std::string costFile = argv[2];
	logFileName = argv[3];
	UdpSocket::init();
	UdpSocket::makeServerSocket(MYPORT,globalMyID);
	NetworkTopology.init(costFile);
	int numbytes;
	
	pthread_t heartBeatThread, checkLinksThread, sendLSPThread;
	pthread_create(&heartBeatThread, 0, commands::sendHeartBeat, (void*)0);
    	pthread_create(&checkLinksThread, 0, commands::checkLinks, (void*)0);
    	pthread_create(&sendLSPThread, 0, commands::sendLSPs, (void*)0);

	while(1)
	{
		struct sockaddr_in their_addr;
		socklen_t addr_len = sizeof(their_addr);
		char buf[MAXBUFLEN];
		if ((numbytes = recvfrom(globalSocketUDP, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			writeLine("test line5");
			perror("recvfrom");
			exit(1);
		}
		char neighborAddr[100];
		inet_ntop(AF_INET, &their_addr.sin_addr, neighborAddr, 100);
		std::string neighborIP(neighborAddr);
		std::string::size_type pos=neighborIP.find_last_of('.');
	    	std::string neighborNodeIdStr = neighborIP.substr(pos+1);
		int neighborNodeId = atoi(neighborNodeIdStr.c_str());
		gettimeofday(&globalLastHeartbeat[neighborNodeId], 0);
		char *message = new char[numbytes];
		memcpy(message,buf,numbytes);
		Packet *packet = new Packet;
		packet->command = std::string(message,message + 4);
		packet->payload = std::string(message + 4,message + numbytes);
		packet->sourceId = neighborNodeId;
		packet->neighborIp = neighborIP;
		
		pthread_t messageHandlerThread;		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&messageHandlerThread, &attr, commands::messageHandler, (void*)packet);
	}
	close(globalSocketUDP);
	return 0;
}
