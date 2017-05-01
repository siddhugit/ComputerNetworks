#include "commands.h"
#include "udp_socket.h"
#include "Dijkstra.h"
#include <cstring>
#include <sstream>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fstream>

extern std::ofstream lout;


std::string HELLO_CMD_MSG = "HELO";
std::string LSP_CMD_MSG = "LSPM";
std::string SEND_CMD_MSG = "SEND";
std::string COST_CMD_MSG = "COST";

#define HEARTBEAT_TIMEOUT   1000000
#define SERVERPORT "7777"

extern int globalMyID;
extern TopologyGraph topologyGraph;
extern RoutingInfo *routingInfo;
extern struct timeval globalLastHeartbeat[256];
extern Topology NetworkTopology;

extern const char* logFileName;

void writeLine(const std::string& message)
{
	int len = message.size();
	FILE *fp = fopen(logFileName, "a");
    	fwrite(message.c_str(), sizeof(char), len, fp);
	fwrite("\n", sizeof(char), 1, fp);
    	fclose(fp);
}

void commands::serialize16(char *dest,uint16_t data)
{
	uint16_t val = htons(data);
	unsigned char *valPtr = (unsigned char*)&val;
	dest[0] = valPtr[0];
	dest[1] = valPtr[1];
}

void commands::serialize32(char *dest,uint32_t data)
{
	uint32_t val = htonl(data);
	unsigned char *valPtr = (unsigned char*)&val;
	dest[0] = valPtr[0];
	dest[1] = valPtr[1];
	dest[2] = valPtr[2];
	dest[3] = valPtr[3];
}


void commands::broadcastHelloMessage()
{
	//uint16_t nodeIdNbo = htons(nodeId);
	//unsigned char *nodeIdNboPtr = (unsigned char*)&nodeIdNbo;
	char message[6];
	strcpy(message,HELLO_CMD_MSG.c_str());
	serialize16(message + 4,globalMyID);
	//message[4] = nodeIdNboPtr[0];
	//message[5] = nodeIdNboPtr[1];
	UdpSocket::hackyBroadcast(message,6);
}

LSP getLSP(const std::string& payload)
{
	LSP result;
	const char *charpayLoadStr = payload.c_str();
	//first 2 bytes originating node
	uint16_t origNodeNbo;
	memcpy(&origNodeNbo,charpayLoadStr,2);
	uint16_t origNode = ntohs(origNodeNbo);
	
	//next 4 bytes ttl
	uint32_t ttlNbo;
	memcpy(&ttlNbo,charpayLoadStr + 2,4);
	uint32_t ttl = ntohl(ttlNbo);
	//next 4 bytes seq_num
	uint32_t seqNumNbo;
	memcpy(&seqNumNbo,charpayLoadStr + 6,4);
	uint32_t seqNum = ntohl(seqNumNbo);

	result.originating_node = origNode;
	result.ttl = ttl;
	result.seq_num = seqNum;

	//next node id and cost pair
	unsigned int offset = 10;
	while(offset < payload.size())
	{
		//node id
		uint16_t nodeIdNbo;
		memcpy(&nodeIdNbo,charpayLoadStr + offset,2);
		uint16_t nodeId = ntohs(nodeIdNbo);
		//cost
		uint32_t costNbo;
		memcpy(&costNbo,charpayLoadStr + offset + 2,4);
		uint32_t cost = ntohl(costNbo);

		result.links.push_back(std::make_pair(nodeId,cost));
		offset += 6;
	}
	return result;
}

void commands::sendLSPMessage(const LSP& lsp,int sourceId)
{
	//uint16_t nodeIdNbo = htons(nodeId);
	//unsigned char *nodeIdNboPtr = (unsigned char*)&nodeIdNbo;
	int messageLen = 14 + (6*(lsp.links.size()));
	char *message = new char[messageLen];
	strcpy(message,LSP_CMD_MSG.c_str());
	//message[4] = nodeIdNboPtr[0];
	//message[5] = nodeIdNboPtr[1];
	serialize16(message + 4,lsp.originating_node);
	serialize32(message + 6,lsp.ttl);
	serialize32(message + 10,lsp.seq_num);	
	int offset = 14;
	for(unsigned int i = 0; i < lsp.links.size(); ++i)
	{
		serialize16(message + offset,lsp.links[i].first);
		serialize32(message + offset + 2,lsp.links[i].second);
		offset += 6;
	}
	Node *node = NetworkTopology.G.nodes[globalMyID];
	std::list<std::pair<Node*,int> > edges = node->neighbors;
	std::list<std::pair<Node*,int> >::iterator it = edges.begin();
	for(;it != edges.end();++it)
	{
		if(it->first->value != sourceId)
		{
			UdpSocket::sendMessage(it->first->value,message,messageLen);
		}
	}
	delete[] message;
}

void commands::sendLSPMessage(const LSP& lsp)
{
	//uint16_t nodeIdNbo = htons(nodeId);
	//unsigned char *nodeIdNboPtr = (unsigned char*)&nodeIdNbo;
	int messageLen = 14 + (6*(lsp.links.size()));
	char *message = new char[messageLen];
	strcpy(message,LSP_CMD_MSG.c_str());
	//message[4] = nodeIdNboPtr[0];
	//message[5] = nodeIdNboPtr[1];
	serialize16(message + 4,lsp.originating_node);
	serialize32(message + 6,lsp.ttl);
	serialize32(message + 10,lsp.seq_num);	
	int offset = 14;
	for(unsigned int i = 0; i < lsp.links.size(); ++i)
	{
		serialize16(message + offset,lsp.links[i].first);
		serialize32(message + offset + 2,lsp.links[i].second);
		offset += 6;
	}
	Node *node = NetworkTopology.G.nodes[globalMyID];
	std::list<std::pair<Node*,int> > edges = node->neighbors;
	std::list<std::pair<Node*,int> >::iterator it = edges.begin();
	for(;it != edges.end();++it)
	{
		UdpSocket::sendMessage(it->first->value,message,messageLen);
	}
	delete[] message;
}
void* commands::sendHeartBeat(void *args)
{
	//lout<<"in sendHeartBeat\n";
	//send hearbeat message every 300 ms
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000; //300 ms
	while(1)
	{
		//increment local sequence number
		++(NetworkTopology.routingInfo[globalMyID].seq_num);
		broadcastHelloMessage();
		nanosleep(&sleepFor, 0);
	}
	return 0;
}

void* commands::sendLSPs(void *args)
{
	//lout<<"in sendLSPs\n";
	//wait for random amount of time in each time period
	srand(time(NULL));
	struct timespec rndWait;
	rndWait.tv_sec = 0;
	rndWait.tv_nsec = (rand() % 500) * 1000000; //random time betwee 0 and 500 ms
	nanosleep(&rndWait, 0);

	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 500 * 1000 * 1000; //500 ms
	while(1)
	{
		//send LSP
		LSP lsp = getCurrLSP();
		sendLSPMessage(lsp);
		nanosleep(&sleepFor, 0);
	}
	return 0;
}

//To Do : implement heartbeat timeout
bool isHeartBeatTimeOut(int nodeId)
{
	//int nodeId = link.first->value;
	struct timeval currTime;
	gettimeofday(&currTime, 0);
	int diffInUSecs = (currTime.tv_usec - globalLastHeartbeat[nodeId].tv_usec) + ((currTime.tv_sec - globalLastHeartbeat[nodeId].tv_sec)*1000000);
	if(diffInUSecs > HEARTBEAT_TIMEOUT)
	{
		return true;
	}
	return false;
}

void checkTimeOut()
{
	Node *node = NetworkTopology.G.nodes[globalMyID];
	std::list<std::pair<Node*,int> >::iterator it = node->neighbors.begin();
	while(it != node->neighbors.end())
	{
		int neighborId = it->first->value;
		bool isTimedOut = isHeartBeatTimeOut(neighborId);
		if (isTimedOut)
		{
			it = node->neighbors.erase(it); 
			NetworkTopology.removeSingleEdge(neighborId,globalMyID);
		}
		else
		{
			++it;
		}
	}
}
void* commands::checkLinks(void *args)
{
	//lout<<"in checkLinks\n";
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 500 * 1000 * 1000; //500 ms
	while(1)
	{
		checkTimeOut();
		nanosleep(&sleepFor, 0);
	}
	return 0;
}

void* commands::messageHandler(void *args)
{
	//lout<<"in messageHandler\n";
	Packet *packet = (Packet*)args;
	int sourceId = packet->sourceId;
	std::string command = packet->command;
	std::string payload = packet->payload;
	std::string neighborIp = packet->neighborIp;
	if(neighborIp.find("10.1.1.") != std::string::npos)
	{
		NetworkTopology.addEdge(globalMyID, sourceId);
	}
	//lout<<"in messageHandler:"<<sourceId<<" "<<payload<<" "<<command<<" "<<"\n";
	if(command == "LSPM")
	{
		LSP lsp = getLSP(payload);
		if(lsp.seq_num > NetworkTopology.routingInfo[lsp.originating_node].seq_num)
		{
			std::vector<std::pair<uint16_t,uint32_t> > newNeighbors = lsp.links;
			Node *sourceNode = NetworkTopology.G.nodes[lsp.originating_node];
			if(sourceNode)
			{
				std::list<std::pair<Node*,int> > currNeighbors = sourceNode->neighbors;
				std::list<std::pair<Node*,int> >::iterator it = currNeighbors.begin();
				for(;it != currNeighbors.end();++it)
				{
					bool found = false;
					for(unsigned int i = 0;i < newNeighbors.size();++i)
					{
						if(it->first->value == newNeighbors[i].first)
						{
							found = true;
							break;
						}
					}
					if(!found)
					{
						NetworkTopology.removeEdge(lsp.originating_node,it->first->value);
					}
				}
			}
			for(unsigned int i = 0;i < newNeighbors.size();++i)
			{
				NetworkTopology.addEdge(lsp.originating_node,newNeighbors[i].first,newNeighbors[i].second);
			}			
			sendLSPMessage(lsp,sourceId);
			NetworkTopology.routingInfo[lsp.originating_node].seq_num = lsp.seq_num;
		}
	}
	else if(command == "send")
	{
		Dijkstra::apply(NetworkTopology.G,globalMyID);
		const char *charpayLoadStr = payload.c_str();
		//node id
		uint16_t destIdNbo;
		memcpy(&destIdNbo,charpayLoadStr,2);
		uint16_t destId = ntohs(destIdNbo);
		//data
		std::string data(charpayLoadStr+2,charpayLoadStr+payload.size());
		if(destId == globalMyID)
		{
			//receiveLog
			std::stringstream ss;
			ss<<"receive packet message "<<data;
			writeLine(ss.str());
		}
		else if(NetworkTopology.getNextHop(destId) == -1)
		{
			//unreachable log
			std::stringstream ss;
			ss<<"unreachable dest "<<destId;
			writeLine(ss.str()); 
		}
		else
		{
			std::string dataToForward = command + payload;
			//writeLine(dataToForward);
			int nextHopNeighbor = NetworkTopology.getNextHop(destId);		
			if(neighborIp.find("10.0.0.10") != std::string::npos)
			{
				//sending log
				std::stringstream ss;
				ss<<"sending packet dest "<<destId<<" nexthop "<<nextHopNeighbor<<" message "<<data;
				writeLine(ss.str()); 
			}
			else
			{
				//forward log
				std::stringstream ss;
				ss<<"forward packet dest "<<destId<<" nexthop "<<nextHopNeighbor<<" message "<<data;
				writeLine(ss.str()); 
			}
			UdpSocket::sendMessage(nextHopNeighbor,dataToForward.c_str(),dataToForward.size());
		}
	}
	else if(command == "cost")
	{
		const char *charpayLoadStr = payload.c_str();
		//node id
		uint16_t nodeIdNbo;
		memcpy(&nodeIdNbo,charpayLoadStr,2);
		uint16_t neighborId = ntohs(nodeIdNbo);
		//cost
		uint32_t costNbo;
		memcpy(&costNbo,charpayLoadStr + 2,4);
		uint32_t cost = ntohl(costNbo);
		
		NetworkTopology.routingInfo[neighborId].initialCost = cost;
		//edgeListIterator iter1,iter2;
		if(NetworkTopology.updateEdgeCost(globalMyID,neighborId,cost))
		{
			NetworkTopology.updateEdgeCost(neighborId,globalMyID,cost); 
		}
	}
	return 0;
}
