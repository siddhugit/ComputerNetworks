#ifndef TOPOLOGY_SOCKET_H__
#define TOPOLOGY_SOCKET_H__

#include <vector>
#include <list>
#include <cstddef>
#include <stdint.h>
#include <string>


struct LSP
{
	uint16_t originating_node;
	uint32_t ttl;
	uint32_t seq_num;
	std::vector<std::pair<uint16_t,uint32_t> > links;
};

struct Node
{
	int value;
	std::list<std::pair<Node*,int> > neighbors;
	Node(int val):value(val){}
};

struct TopologyGraph
{
	std::vector<Node*> nodes;
	TopologyGraph():nodes(256,(Node*)0)
	{
	}
};

struct RoutingInfo
{
	int next_hop;
	int distance;
	uint32_t seq_num;
	int initialCost;
};

LSP getCurrLSP();

typedef std::list<std::pair<Node*,int> > edgeList;
typedef edgeList::iterator edgeListIterator;

struct Topology
{
	TopologyGraph G;
	RoutingInfo routingInfo[256];
	Topology();
	void init(const std::string& initialCostFile);
	void initRoutingInfo();
	void addEdge(int i,int j,int cost);
	void addEdge(int i,int j);
	void removeEdge(int i,int j);
	void removeSingleEdge(int i,int j);
	void setNextHop(int dest,int nextHop);
	void setCost(int dest,int cost);
	int getNextHop(int dest);
	int getCost(int dest);
	bool findEdge(int i,int j,edgeListIterator& it);
	bool updateEdgeCost(int i,int j,int newCost);
};
#endif //TOPOLOGY_SOCKET_H__
