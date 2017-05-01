#include "tcp_socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>

static const int BUFF_SIZE = 4096;
static const int BACKLOG = 100;

int TcpSocket::makeServerSocket(const std::string& port)
{
	const char* portNum = port.c_str();
    	struct addrinfo hints;
    	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo *serverAddr,*servIter;
	int result;
	if((result = getaddrinfo(NULL,portNum,&hints,&serverAddr)) != 0)
	{
		fprintf(stderr, "getaddrinfo error : %s\n", gai_strerror(result));
		return -1;
	}
	int sockFd = -1;
	for(servIter = serverAddr; servIter != NULL; servIter = servIter->ai_next)
	{
		if((sockFd = socket(servIter->ai_family,servIter->ai_socktype,servIter->ai_protocol)) == -1)
		{
			perror("socket() error");
			continue;
		}
		int yes = 1;
		if(setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) == -1)
        	{
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockFd, servIter->ai_addr, servIter->ai_addrlen) == -1)
        	{
			close(sockFd);
			perror("server: bind");
			continue;
		}
		break;
	}
	if (!servIter)
    	{
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	freeaddrinfo(serverAddr);
	if (listen(sockFd, BACKLOG) == -1)
    	{
		perror("listen");
		exit(1);
    	}
	return sockFd;
}

int TcpSocket::connectToServer(const std::string& host,const std::string& port)
{
	const char* portNum = port.c_str();
	const char* hostName = host.c_str();
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

int TcpSocket::recvAllBytes(int sockFd,int &statusCode,std::string& headers)
{
	int totalBytes = 0;
	char buffer[BUFF_SIZE];
	bool headerReceived = false;
	std::vector<char> output;
	std::ofstream out("output", std::ios::out | std::ios::binary);
	while(1)
	{
		int bytesRead;
		if((bytesRead = recv(sockFd,buffer,BUFF_SIZE, 0)) < 0)
		{
		    perror("recv() error");
		    exit(1);
		}
		else if(bytesRead == 0)
		{
			break;
		}
		totalBytes += bytesRead;
		if(totalBytes > 3)
		{
			if(headerReceived)
			{
				out.write(buffer, bytesRead);
			}
			else
			{
				output.insert(output.end(),buffer,buffer + bytesRead);
				std::string response(output.begin(),output.end());
				std::string::size_type pos =  response.find("\r\n\r\n");
				if(pos != std::string::npos)
				{
					headerReceived = true;
					headers = response.substr(0,pos + 4);
					std::stringstream responseStream(response);
					//status line
					std::string statusLine;
					std::getline(responseStream,statusLine);
					std::stringstream slss(statusLine);
					std::string status;
					slss >> status;
					slss >> status;
					statusCode = atoi(status.c_str());
					if((statusCode == 302 || statusCode == 301))
					{
						out.close();
						return totalBytes;
					}
					if(output.size() > pos + 4)
					{
						out.write(&output[pos + 4], output.size() - pos - 4);
					}
				}
			}
		}
	}
	out.close();
	return totalBytes;
}

int TcpSocket::recvAll(int sockFd,std::string& output,bool isServer)
{
	int totalBytes = 0;
	char buffer[BUFF_SIZE];
	std::string prev,curr;
	while(1)
	{
		int bytesRead;
		if((bytesRead = recv(sockFd,buffer,BUFF_SIZE - 1, 0)) < 0)
		{
		    perror("recv() error");
		    exit(1);
		}
		else if(bytesRead == 0)
		{
			break;
		}
		buffer[bytesRead] = '\0';
		output += buffer;
		totalBytes += bytesRead;
		if(isServer && totalBytes > 3)
		{
			std::string::size_type pos =  output.find("\r\n\r\n");
			if(pos != std::string::npos)
			{
				output = output.substr(0,pos);
				return totalBytes;
			}
		}
	}
	return totalBytes;
}

int TcpSocket::sendAll(int sockFd,const char* data,int size)
{
	int bytesToSend = size;
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

int TcpSocket::sendAll(int sockFd,const std::string& text)
{
	int bytesToSend = text.size();
	const char *data = text.c_str();
	return sendAll(sockFd,data,bytesToSend);
}

bool TcpSocket::sendFile(int sockFd,const std::string& fileName,const std::string& headers)
{
	std::ifstream inFile(fileName.c_str(),std::ios::in|std::ios::binary|std::ios::ate);
	int headerSize = headers.size();
	bool result = false;
	char memblock[BUFF_SIZE];
	int total  = 0;
	//send headers
	sendAll(sockFd,headers.c_str(),headerSize);
	if (inFile.is_open())
	{
	    int fileSize = inFile.tellg();
	    inFile.seekg (0, std::ios::beg);
	    while(!inFile.eof())
	    {
		inFile.read(memblock, BUFF_SIZE);
		int bytesReadFromFile = inFile.gcount();
	    	int bytesSent = sendAll(sockFd,memblock,bytesReadFromFile);
		total += bytesSent;
	    }
	    inFile.close();
	    result = (fileSize == total);
	}
	return result;
}

