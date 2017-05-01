#ifndef commands_H__
#define commands_H__

#include <string>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include "topology.h"

void writeLine(const std::string& message);

class commands
{
	static void serialize16(char *dest,uint16_t data);
	static void serialize32(char *dest,uint32_t data);
	public:
		static void broadcastHelloMessage();
		static void sendLSPMessage(const LSP& lsp);
		static void sendLSPMessage(const LSP& lsp,int sourceId);
		static void* sendHeartBeat(void *args);
		static void* sendLSPs(void *args);
		static void* checkLinks(void *args);
		static void* messageHandler(void *args);
};

struct Packet
{
	int sourceId;
	std::string neighborIp;
	std::string command;
	std::string payload;
};

#endif //commands_H__
