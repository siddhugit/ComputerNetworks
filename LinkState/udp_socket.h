#ifndef UDP_SOCKET_H__
#define UDP_SOCKET_H__

#include <string>
#include <vector>

class UdpSocket
{
	public:
		static void init();
		static void hackyBroadcast(const char* buf, int length);
		static int makeServerSocket(const std::string& port,int nodeId);
		static void sendMessage(int nodeId,const char* message,int msgLen);
};

#endif //UDP_SOCKET_H__
