#include <iostream>
#include "url_util.h"
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include "tcp_socket.h"


static std::string sendGet(int sockFd,const std::string& host,const std::string& resource,int& statusCode)
{
	std::string request = "GET /";
	request += resource;
	request += " HTTP/1.0\r\n";
	request += "Host: ";
	request += host;
	request += "\r\n\r\n";
	TcpSocket::sendAll(sockFd,request);
	std::vector<char> output;
	std::string headers;
	TcpSocket::recvAllBytes(sockFd,statusCode,headers);
	return headers;
}


std::string getRedirectedUrl(const std::string& headers)
{
	std::stringstream responseStream(headers);
	std::string statusLine;
	std::getline(responseStream,statusLine);
	std::string header;
	while(std::getline(responseStream,header))
	{
		if(header == "\r")
		{
			break;
		}
		std::string::size_type pos = header.find(": ");
		std::string key = header.substr(0,pos);
		std::string value = header.substr(std::min(pos + 2,header.size()));
		if(UrlUtil::strCmpIgnCase(key,"Location"))
		{
			return value;
		}
	}
	return "";
}

static void sendRequest(const std::string& url)
{
	std::vector<std::string> result = UrlUtil::parseUrl(url);
	if(result.size() == 4)
	{
		std::string host = result[1];
		std::string port = result[2];
		std::string resource = result[3];
		int sockFd = TcpSocket::connectToServer(host,port);
		if(sockFd == -1)
		{
			printf("Failed to connect to server\n");
			exit(1);
		}
		int statusCode;
		std::string headers = sendGet(sockFd,host,resource,statusCode);
		close(sockFd);
		if(statusCode == 301 || statusCode == 302)
		{
			std::string redirectedUrl = getRedirectedUrl(headers);
			std::cout<<"redirectedUrl = "<<redirectedUrl<<"\n";
			sendRequest(redirectedUrl);
		}
	}
}

int main(int argc,char **argv)
{
	if(argc < 2)
	{
		printf("Usage : http_client <url>\n");
		exit(1);
	}
        sendRequest(argv[1]);
	return 0;
}
