#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>

static const int BUFF_SIZE = 1024;

const char* HELLO_MSG = "HELO\n";
const char* USR_MSG = "USERNAME ";
const char* RECV_MSG = "RECV\n";
const char* BYE_MSG = "BYE\n";
const char* RESPONSE_MSG = "300 - DATA: ";
const char* PREFIX_MSG = "Received: ";

int sendAll(int sockFd,const char *data,int bytesToSend)
{
	int bytesLeft = bytesToSend;
	int bytesSent = 0,sent;
	while(bytesLeft > 0)
	{
		if((sent = send(sockFd,data + bytesSent,bytesLeft,0)) == -1)
		{
			return -1;
		}
		else if(sent == 0)
		{
			break;
		}
		else
		{
			bytesLeft -= sent;
			bytesSent += sent;
		}
	}
	return bytesSent;
}

int connectToServer(const char* hostName,const char* portNum)
{
	struct addrinfo hints;
	struct addrinfo *serverAddr,*servIter;
	int sockFd = -1;
	int result;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if((result = getaddrinfo(hostName,portNum,&hints,&serverAddr)) != 0)
	{
		fprintf(stderr, "getaddrinfo error : %s\n", gai_strerror(result));
		return -1;
	}
	for(servIter = serverAddr; servIter != NULL; servIter = servIter->ai_next)
	{
		if((sockFd = socket(servIter->ai_family,servIter->ai_socktype,servIter->ai_protocol)) == -1)
		{
			perror("socket() error");
			continue;
		}
		if((result = connect(sockFd,servIter->ai_addr,servIter->ai_addrlen)) == -1)
		{
			close(sockFd);
			perror("connect() error");
			continue;
		}
		break;
	}
	freeaddrinfo(serverAddr);
	if(servIter == NULL)
	{
		return -1;
	}
	return sockFd;
}

char* getUserNameMsg(const char* userName)
{
	char *result;
	int usrNmLen = strlen(userName);
	int usrMsgLen = strlen(USR_MSG);
	int len = usrMsgLen + usrNmLen;
	len += 2; //for newline and null termination
	result = malloc(len);
	strcpy(result,USR_MSG);
	strcpy(result + usrMsgLen,userName);
	strcpy(result + usrMsgLen + usrNmLen,"\n");
	return result;
}

void sendMessage(int sockFd,const char* message,int doPrint)
{
	int received;
	char buff[BUFF_SIZE];
	if(sendAll(sockFd,message,strlen(message)) == -1)
	{
		perror("send() error");
		exit(1);
	}
	if((received = recv(sockFd,buff,BUFF_SIZE,0)) == -1)
	{
		perror("recv() error");
		exit(1);
	}
	if(doPrint)
	{
		buff[received] = '\0';
		printf("%s%s",PREFIX_MSG,buff + strlen(RESPONSE_MSG));
	}
}

void sendMessages(int sockFd,const char* userName)
{
	char *usrMsg;
	int index;
	sendMessage(sockFd,HELLO_MSG,0);
	usrMsg = getUserNameMsg(userName);
	sendMessage(sockFd,usrMsg,0);
	free(usrMsg);
	for(index = 0; index < 10; ++index)
	{
		sendMessage(sockFd,RECV_MSG,1);
	}
	sendMessage(sockFd,BYE_MSG,0);
}

int main(int argc,char **argv)
{
	int sockFd;
	char *userName;
	if(argc != 4)
	{
		printf("Usage : mp0client <hostname> <port> <username>\n");
		exit(1);
	}
	userName = argv[3];
	sockFd = connectToServer(argv[1],argv[2]);
	if(sockFd == -1)
	{
		printf("Failed to connect to server\n");
		exit(1);
	}
	sendMessages(sockFd,userName);
	close(sockFd);
	return 0;
}
