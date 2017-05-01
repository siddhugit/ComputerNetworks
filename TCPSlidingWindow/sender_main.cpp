#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>
#include "udp_socket.h"

//test code
/*int drop_rate = 20;
std::set<unsigned long long int> TestAckSet;
bool doSendAck(unsigned long long int baseFrameNum)
{
	if(baseFrameNum % drop_rate == 19)
	{
		if(TestAckSet.find(baseFrameNum) == TestAckSet.end())
		{
			TestAckSet.insert(baseFrameNum);
			return false;
		}
		return true;
	}
	return true;
}*/

#define PAYLOAD_SIZE 1464
#define HEADER_SIZE 8
#define WRAP_WINDOW 262144 
#define SEND_WINDOW 64

struct RttInfo
{
	static const float alpha = 0.5;
	timeval sent[WRAP_WINDOW];
	timeval recv[WRAP_WINDOW];
	timeval srtt;
	RttInfo()
	{
		srtt.tv_sec = 0;
		srtt.tv_usec = 150000;
	}
	void SentFrame(int index)
	{
		gettimeofday(&sent[index], 0);
	}
	void ReceivedAck(int index)
	{
		gettimeofday(&recv[index], 0);
		update(index);
	}
	void update(int index)
	{
		timeval temp;
		temp.tv_sec = 2*(recv[index].tv_sec - sent[index].tv_sec);
		temp.tv_usec = 2*(recv[index].tv_usec - sent[index].tv_usec);
		//std::cout<<"temp.tv_sec = "<<temp.tv_sec<<"\n";
		//std::cout<<"temp.tv_usec = "<<temp.tv_usec<<"\n";
		//temp.tv_sec = ((1.0 - alpha)*(float)srtt.tv_sec) + alpha*(float)(recv[index].tv_sec - sent[index].tv_sec);
		//temp.tv_usec = ((1.0 - alpha)*(float)srtt.tv_usec) + alpha*(float)(recv[index].tv_usec - sent[index].tv_usec);
		srtt = temp;
	}
};

const float RttInfo::alpha;

RttInfo SRTT;

std::vector<std::pair<bool,char*> > FileChunks(WRAP_WINDOW,std::make_pair(false,(char*)0));
std::vector<int> BytesRead(WRAP_WINDOW,0);
std::vector<bool> AckReceieved(WRAP_WINDOW,false);

int WindowSize = 16;
const int Threshold = SEND_WINDOW/2;

void initFileChunk()
{
	for(int i = 0; i < WRAP_WINDOW; ++i)
	{
		FileChunks.push_back(std::make_pair(false,new char[PAYLOAD_SIZE]));
	}
}

void additiveIncr()
{
	if(WindowSize < Threshold)
	{
		WindowSize *= 2;
		WindowSize = std::min(WindowSize,Threshold);
	}
	else
	{
		++WindowSize;
		WindowSize = std::min(WindowSize,SEND_WINDOW);
	}
}

void muliplicativeDecr()
{
	WindowSize /= 2;
	WindowSize = std::max(WindowSize,1);
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

void createFrame(char *dest,char* payload,int frameNum,int payLoadSz,bool lastChunk)
{
	memset(dest,0,payLoadSz+4);
	uint32_t seqNum = frameNum;
	serialize32(dest,frameNum);
	uint32_t endMark = lastChunk ? 1 : 0;
	serialize32(dest + 4,endMark);
	memcpy(dest + 8,payload,payLoadSz);
	//payload[payLoadSz] = '\0';
	//std::cout<<"payload = "<<payload<<"\n";
}
//int globalcounter = 0;

bool sendBytes(FILE *fileFp,unsigned long long baseFrameNum,int sockfd,addrinfo *destAddr,
				unsigned long long int& lastFrameNum,unsigned long long int& eofFrameNum,unsigned long long int& remainingBytes)
{
	bool lastRead = false;
	int i = 0;
	for(; i < WindowSize && (lastRead == false);++i)
	{
		lastFrameNum = baseFrameNum + i;
		if(lastFrameNum > eofFrameNum)
		{
			return true;
		}
		int offset = (lastFrameNum) % WRAP_WINDOW;
		if(!FileChunks[offset].first)
		{
			int bytesToRead = std::min((unsigned long long int)PAYLOAD_SIZE,remainingBytes);
			if(!FileChunks[offset].second)
			{
				FileChunks[offset].second = new char[PAYLOAD_SIZE];
			}
			int bytesRead = fread(FileChunks[offset].second,1,bytesToRead,fileFp);
			FileChunks[offset].first = true;
			BytesRead[offset] = bytesRead;
			remainingBytes -= bytesRead;
			lastRead = (remainingBytes == 0 || (bytesRead != bytesToRead));
			if(lastRead == true)
			{
				eofFrameNum = lastFrameNum;
				lastRead = true;
				//std::cout<<"here 1 eofFrameNum = "<<eofFrameNum<<"##\n";
			}
		}
		char frame[PAYLOAD_SIZE + HEADER_SIZE];
		createFrame(frame,FileChunks[offset].second,offset,BytesRead[offset],lastRead);
		SRTT.SentFrame(offset);
		/*if(globalcounter == 2 && i == 2)
		{
			++globalcounter;
			std::cout<<"Not sending = "<<offset<<"##\n";
			//continue;
		}
		if(globalcounter == 0 && i == 7)
		{
			++globalcounter;
			std::cout<<"Not sending = "<<offset<<"##\n";
			continue;
		}
		if(globalcounter == 1 && i == 9)
		{
			++globalcounter;
			std::cout<<"Not sending = "<<offset<<"##\n";
			//continue;
		}*/
		//if(doSendAck(lastFrameNum)) //test drops
		//{
			UdpSocket::sendMessage(sockfd,frame,BytesRead[offset] + 8,destAddr);
		//}
		//++chunkSent;
	}
	return lastRead;
}

void resetWindowParams(int offset)
{
	FileChunks[offset].first = false;
	AckReceieved[offset] = false;
	BytesRead[offset] = false;
}

unsigned long long int processAck(unsigned char *buff,unsigned long long baseFrameNum,unsigned long long int eofFrameNum,bool &gotAllAck)
{
	uint32_t frameNumNbo;
	memcpy(&frameNumNbo,buff + 3,4);
	uint32_t offset = ntohl(frameNumNbo);
	int expectedAck = baseFrameNum % WRAP_WINDOW;
	if(offset == expectedAck)
	{
		AckReceieved[offset] = true;
		while(AckReceieved[offset])
		{
			if(eofFrameNum == baseFrameNum)
			{
				//std::cout<<"here 1 gotAllAck = "<<gotAllAck<<"##\n";
				gotAllAck = true;
				break;
			}
			resetWindowParams(offset);
			++offset;
			++baseFrameNum;
			offset %= WRAP_WINDOW;
		}
		additiveIncr();
		SRTT.ReceivedAck(offset);
		//std::cout<<"baseFrameNum = "<<baseFrameNum<<"##\n";
	}
	else if(offset != expectedAck)
	{
		AckReceieved[offset] = true;
	}
	else//ignore dup ack
	{
		//AckReceieved[offset] = false;
	}
	//std::cout<<"here 2 baseFrameNum = "<<baseFrameNum<<"##\n";
	return baseFrameNum;
	//int frameNum = baseFrameNum + offset;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) 
{
	//initFileChunk();
	addrinfo *destAddr;
	char portbuff[16];
	sprintf(portbuff,"%d",hostUDPport);
	std::string port = portbuff;
	int destsockfd = UdpSocket::getDestSocket(hostname,port,&destAddr);
	//std::cout<<"destsockfd = "<<destsockfd<<"##\n";
	
	FILE *fileFp = fopen ( filename , "rb" ); 
	if (fileFp==NULL) 
	{
		std::cerr<<"Error Opening file\n"; 
		exit (1);
	}
	bool done = false;
	unsigned long long int baseFrameNum = 0;
	unsigned long long int remainingBytes = bytesToTransfer;
	unsigned long long int eofFrameNum = std::numeric_limits<unsigned long long int>::max();
	while(!done)
	{
		unsigned long long int lastFrameNum;
		bool fileSent = sendBytes(fileFp,baseFrameNum,destsockfd,destAddr,lastFrameNum,eofFrameNum,remainingBytes);
		int chunkSent = lastFrameNum + 1 - baseFrameNum;
		//std::cout<<"chunkSent = "<<chunkSent<<"##\n";
		//struct timeval guess_rtt = SRTT.srtt;
		struct timeval guess_rtt;
		guess_rtt.tv_sec = 0;
		guess_rtt.tv_usec = 48000; //48 ms
		setsockopt(destsockfd, SOL_SOCKET,SO_RCVTIMEO,&guess_rtt,sizeof(guess_rtt));
		for(int i = 0; i < chunkSent;++i)
		{
			struct sockaddr_in theirAddr;
			socklen_t theirAddrLen = sizeof(theirAddr);
			unsigned char recvBuf[128];
			int bytesRecvd = recvfrom(destsockfd, recvBuf, 128 , 0, (struct sockaddr*)&theirAddr, &theirAddrLen);
			//std::cout<<"bytesRecvd = "<<bytesRecvd<<"##\n";
			if(bytesRecvd < 0)//to do : time outs/packet drop
			{
				muliplicativeDecr();
			}
			else
			{
				bool gotAllAck = false;
				baseFrameNum = processAck(recvBuf,baseFrameNum,eofFrameNum,gotAllAck);
				if(gotAllAck)
				{
					done = true;
					break;
				}
			}
		}
	}
	fclose(fileFp);
	close(destsockfd);
	std::cout<<"Send complete\n";
}

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	unsigned long long int numBytes;
	
	if(argc != 5)
	{
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int)atoi(argv[2]);
	numBytes = atoll(argv[4]);
	
	reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
} 
