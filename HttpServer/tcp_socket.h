#ifndef TCP_SOCKET_H__
#define TCP_SOCKET_H__

#include <string>
#include <vector>

class TcpSocket
{
	static int sendAll(int sockFd,const char* data,int size);
	public:
		static int makeServerSocket(const std::string& port);
		static int connectToServer(const std::string& host,const std::string& port);
		static int recvAll(int sockFd,std::string& output,bool isServer = false);
		static int recvAllBytes(int sockFd,int &statusCode,std::string& headers);
		static int sendAll(int sockFd,const std::string& text);
		static bool sendFile(int sockFd,const std::string& fileName,const std::string& headers);
};

#endif //TCP_SOCKET_H__
