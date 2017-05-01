#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <vector>
#include "udp_socket.h"

#define RECBUFFSIZE 2048
#define WRAP_WINDOW 262144
#define PAYLOAD_SIZE 1464

std::vector<std::pair<bool,char*> > FileChunks(WRAP_WINDOW,std::make_pair(false,(char*)0));
std::vector<int> BytesRead(WRAP_WINDOW,0);
std::vector<bool> AckSent(WRAP_WINDOW,false);

void initFileChunk()
{
	for(int i = 0; i < WRAP_WINDOW; ++i)
	{
		FileChunks.push_back(std::make_pair(false,new char[PAYLOAD_SIZE]));
	}
}

void resetWindowParams(int offset,unsigned long long int baseFrameNum)
{
	FileChunks[offset].first = false;
	AckSent[offset] = false;
	BytesRead[offset] = false;
}

void serialize32(char *dest,uint32_t data)
{
	uint32_t val = htonl(data);
	unsigned char *valPtr = (unsigned char*)&val;
	dest[0] = valPtr[0];
	dest[1] = valPtr[1];
	dest[2] = valPtr[2];
	dest[3] = valPtr[3];
}

void createFrame(char *dest,int frameNum)
{
	memset(dest,0,7);
	uint32_t seqNum = frameNum;
	serialize32(dest + 3,frameNum);
	memcpy(dest,"ACK",3);
}

void sendAck(int frameNum,int sockfd,sockaddr_in *theirAddr)
{
	char frame[7];
	createFrame(frame,frameNum);
	//std::cout<<"sending frameNum = "<<frameNum<<"##\n";
	sendto(sockfd, frame, 7, 0,(sockaddr*)theirAddr, sizeof(sockaddr_in));//send ack
	//std::cout<<"sent frameNum = "<<frameNum<<"##\n";
}

bool isGreater(int frameNum,int expectedFrameNum)
{
	if(frameNum > expectedFrameNum)
	{
		return true;
	}
	else if(frameNum < expectedFrameNum && abs(frameNum - expectedFrameNum) > WRAP_WINDOW/2)
	{
		return true;
	}
	return false;
}

unsigned long long int processData(char *payload,int payloadSize,int frameNum,
		unsigned long long int baseFrameNum,int sockfd,sockaddr_in *theirAddr,FILE *outfile,int lastFrameNum,bool &done)
{
	int expectedFrameNum = baseFrameNum % WRAP_WINDOW;
	//std::cout<<"frameNum = "<<frameNum<<"##\n";
	//std::cout<<"offset = "<<offset<<"##\n";
	//std::cout<<"baseFrameNum = "<<baseFrameNum<<"##\n";
	sendAck(frameNum,sockfd,theirAddr);
	//if(frameNum != expectedFrameNum) //out of order
	if(isGreater(frameNum,expectedFrameNum))
	{
		FileChunks[frameNum].first = true;
		if(!FileChunks[frameNum].second)
		{
			FileChunks[frameNum].second = new char[PAYLOAD_SIZE];
		}
		memcpy(FileChunks[frameNum].second,payload,payloadSize);
		BytesRead[frameNum] = payloadSize;
		AckSent[frameNum] = true;
		//char frame[7];
		//createFrame(frame,frameNum);
		//sendto(sockfd, frame, 7, 0,(sockaddr*)theirAddr, sizeof(sockaddr_in));//send ack
		//UdpSocket::sendMessage(sockfd,frame,BytesRead[offset] + 4,(sockaddr*)theirAddr);
	}
	else if(frameNum == expectedFrameNum)
	{
		FileChunks[frameNum].first = true;
		if(!FileChunks[frameNum].second)
		{
			FileChunks[frameNum].second = new char[PAYLOAD_SIZE];
		}
		memcpy(FileChunks[frameNum].second,payload,payloadSize);
		BytesRead[frameNum] = payloadSize;
		AckSent[frameNum] = true;
		while(AckSent[frameNum])
		{
			if(frameNum == lastFrameNum)
			{
				done = true;
			}
			int written = fwrite(FileChunks[frameNum].second,1,BytesRead[frameNum],outfile);
			//char frame[7];
			//createFrame(frame,frameNum);
			//sendto(sockfd, frame, 7, 0,(sockaddr*)theirAddr, sizeof(sockaddr_in));//send ack
			resetWindowParams(frameNum,baseFrameNum);
			++frameNum;
			++baseFrameNum;
			frameNum %= WRAP_WINDOW;
		}
	}
	return baseFrameNum;
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) 
{
	//initFileChunk();
	FILE *fileFp = fopen ( destinationFile , "wb" ); 
	if (fileFp==NULL) 
	{
		std::cerr<<"Error Opening file\n"; 
		exit (1);
	}
	char portbuff[16];
	sprintf(portbuff,"%d",myUDPport);
	std::string port = portbuff;
	int sockfd = UdpSocket::makeServerSocket(port);
	unsigned long long int baseFrameNum = 0;
	int lastFrameNum = -1;
	while(1)
	{
		unsigned char recvBuf[RECBUFFSIZE];
		struct sockaddr_in theirAddr;
		socklen_t theirAddrLen = sizeof(theirAddr);
		int bytesRecvd = recvfrom(sockfd, recvBuf, RECBUFFSIZE , 0, (struct sockaddr*)&theirAddr, &theirAddrLen);
		if(bytesRecvd < 1)
		{
			//break;
		}
		else
		{
			uint32_t frameNumNbo;
			memcpy(&frameNumNbo,recvBuf,4);
			uint32_t frameNum = ntohl(frameNumNbo);
			//std::cout<<"frameNum = "<<frameNum<<"##\n";
			uint32_t endMarkNbo;
			memcpy(&endMarkNbo,recvBuf + 4,4);
			uint32_t endMark = ntohl(endMarkNbo);
			char *payload = (char*)recvBuf + 8;
			int payloadSize = bytesRecvd - 8;
			if(endMark == 1)
			{
				lastFrameNum = frameNum;
			}
			bool done = false;
			baseFrameNum = processData(payload,payloadSize,frameNum,baseFrameNum,sockfd,&theirAddr,fileFp,lastFrameNum,done);
			if(done)
			{
				break;
			}
		}
	}
	fclose(fileFp);
	shutdown(sockfd, 2);
	close(sockfd);
	std::cout<<"Receive complete\n";
}

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	
	if(argc != 3)
	{
		fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
		exit(1);
	}
	
	udpPort = (unsigned short int)atoi(argv[1]);
	
	reliablyReceive(udpPort, argv[2]);
}
