#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "tcp_socket.h"
#include "url_util.h"

static std::string getStatusLine(int statusCode,const std::string& msg)
{
	std::string result = "HTTP/1.0 ";
	std::ostringstream ss;
	ss << statusCode;
	result += ss.str();
	result += " ";
	result += msg;
	result += "\r\n";
	return result;
}
static std::string getHeaders()
{
	return "";
}

static bool fileExists(const std::string& fileName)
{
	struct stat st; 
	int ret = stat(fileName.c_str(),&st); 
	return (ret == 0);
}

static void send_not_found(int clientFd)
{
	std::string statusline = getStatusLine(404,"Not Found");
	std::string headers = statusline + getHeaders();
	headers += "\r\n";
	std::string html = "<html><head><title>404 Error - Document Not Found</title></head><body><h1>404 - Whoops, File Not Found!</h1></body></html>";
	std::string response = headers + html;
	TcpSocket::sendAll(clientFd,response);
}

static void send_bad_request(int clientFd)
{
	std::string statusline = getStatusLine(400,"Bad Request");
	std::string headers = statusline + getHeaders();
	headers += "\r\n";
	std::string html = "<html><head><title>400 Error - Bad Request</title></head><body><h1>400 - Bad Request</h1></body></html>";
	std::string response = headers + html;
	TcpSocket::sendAll(clientFd,response);
}

static void send_file(const std::string& fileName,int clientFd)
{
	std::string statusline = getStatusLine(200,"OK");
	std::string headers = statusline + getHeaders();
	headers += "\r\n";
	TcpSocket::sendFile(clientFd,fileName,headers);
}

static void processRequest(const std::string& request,int clientFd)
{
	std::stringstream requestStream(request);
	std::string method,resource,scheme;
	requestStream >> method;
	requestStream >> resource;
	requestStream >> scheme;
	if(UrlUtil::strCmpIgnCase(method,"GET") == false)
	{
		send_bad_request(clientFd);
		return;
	}
	if(scheme.find("http") == std::string::npos && scheme.find("HTTP") == std::string::npos)
	{
		send_bad_request(clientFd);
		return;
	}
	if(resource.size() < 1)
	{
		send_bad_request(clientFd);
		return;
	}
	std::string fileName = resource.substr(1);
	if(!fileExists(fileName))
	{
		send_not_found(clientFd);
		return;
	}
	send_file(fileName,clientFd);
}

static void *handleRequest(void *data)
 {
    long clientFd = (long)data;
    std::string request;
    TcpSocket::recvAll(clientFd,request,true);
    processRequest(request,clientFd);
    close(clientFd);
    pthread_exit(NULL);
 }
int main(int argc, char **argv)
{
	if(argc < 2)
	{
		printf("Usage : http_server <port>\n");
		exit(1);
	}
	const char *portNum = argv[1];
	int sockFd = TcpSocket::makeServerSocket(portNum);
	while(1)
	{  	//accept() loop
		sockaddr_storage clientAddr;
		socklen_t clientAddrSize = sizeof(clientAddr);
		int clientFd = accept(sockFd, (struct sockaddr *)&clientAddr, &clientAddrSize);
		if (clientFd == -1)
		{
			perror("accept");
			continue;
		}
		pthread_t thread;
		long lClientFd = clientFd;
		pthread_create(&thread, NULL, handleRequest, (void *)lClientFd);
	}
	close(sockFd);
    	return 0;
}
