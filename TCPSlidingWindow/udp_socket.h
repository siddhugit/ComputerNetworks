#ifndef UDP_SOCKET_H__
#define UDP_SOCKET_H__

#include <string>
#include <vector>

struct addrinfo; //forward declaration

class UdpSocket
{
	public:
		//static void init();
		//static void hackyBroadcast(const char* buf, int length);
		static int getDestSocket(const std::string& hostName,const std::string& port,addrinfo **destAddr);
		static int makeServerSocket(const std::string& port);
		static void sendMessage(int sockFd,const char* message,int msgLen,addrinfo *destAddr);
};

#endif //UDP_SOCKET_H__
