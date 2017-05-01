#include "topology.h"
#include <stdint.h>
#include <limits>
#include <fstream>
#include <sstream>
#include <iostream>

extern int globalMyID;

Topology NetworkTopology;

extern std::ofstream lout;

void writeLine(const std::string& message);

LSP getCurrLSP()
{
	LSP result;
	result.originating_node = globalMyID;
	result.ttl = 0;
	result.seq_num = NetworkTopology.routingInfo[globalMyID].seq_num;
	Node *node = NetworkTopology.G.nodes[globalMyID];
	std::list<std::pair<Node*,int> > edges = node->neighbors;
	std::list<std::pair<Node*,int> >::iterator it = edges.begin();
	for(;it != edges.end();++it)
	{
		result.links.push_back(std::make_pair(it->first->value,it->second));
	}
	return result;
}

Topology::Topology()
{
	//init();
}

void Topology::initRoutingInfo()
{
	for(int i = 0; i < 256;++i)
	{
		routingInfo[i].seq_num = 0;
		routingInfo[i].distance = INT32_MAX;
		routingInfo[i].next_hop = -1;
		routingInfo[i].initialCost = INT32_MAX;
	}
	routingInfo[globalMyID].distance = 0;
	routingInfo[globalMyID].next_hop = globalMyID;
	routingInfo[globalMyID].initialCost = 0;
}
void Topology::init(const std::string& initialCostFile)
{
	G.nodes[globalMyID] = new Node(globalMyID);
	for(int i = 0; i < 256;++i)
	{
		routingInfo[i].seq_num = 0;
		routingInfo[i].distance = INT32_MAX;
		routingInfo[i].next_hop = -1;
		routingInfo[i].initialCost = INT32_MAX;
	}
	routingInfo[globalMyID].distance = 0;
	routingInfo[globalMyID].next_hop = globalMyID;
	routingInfo[globalMyID].initialCost = 0;

	std::ifstream infile(initialCostFile.c_str());
	std::string line;
	while (std::getline(infile, line))
	{
	    std::istringstream iss(line);
	    int nieghborId, cost;
	    iss >> nieghborId >> cost;
	    routingInfo[nieghborId].initialCost = cost;
	}
}

void Topology::addEdge(int i,int j)
{
	int cost = routingInfo[j].initialCost;
	if(cost == INT32_MAX)
	{
		cost = 1;
	}
	addEdge(i,j,cost);
}

void Topology::addEdge(int i,int j,int cost)
{
	/*edgeListIterator iter1,iter2;
	if(findEdge(i,j,iter1))
	{
		findEdge(j,i,iter2);
		iter1->second = cost;
		iter2->second = cost;
	}*/
	Node *node1 = G.nodes[i];
	Node *node2 = G.nodes[j];
	bool found = false;
	if(node1 && node2)
	{
		std::list<std::pair<Node*,int> >::iterator it = node1->neighbors.begin();
		for(;it != node1->neighbors.end();++it)
		{
			if(it->first == node2)
			{
				it->second = cost;
				found = true;
				break;
			}
		}
		if(found)
		{
			it = node2->neighbors.begin();
			for(;it != node2->neighbors.end();++it)
			{
				if(it->first == node1)
				{
					it->second = cost;
					return;
				}
			}
		}
	}
	if(!found)
	{
		if(!G.nodes[i])
		{
			G.nodes[i] = new Node(i);
		}
		if(!G.nodes[j])
		{
			G.nodes[j] = new Node(j);
		}
		Node *node1 = G.nodes[i];
		Node *node2 = G.nodes[j];
		//lout<<"node1 = "<<node1->value<<" , cost = "<<cost<<"\n";
		//lout<<"node2 = "<<node2->value<<" , cost = "<<cost<<"\n";
		node1->neighbors.push_back(std::make_pair(node2,cost));	
		node2->neighbors.push_back(std::make_pair(node1,cost));
	}
}

void Topology::removeSingleEdge(int i,int j)
{
	Node *node1 = G.nodes[i];
	Node *node2 = G.nodes[j];
	if(node1 && node2)
	{
		std::list<std::pair<Node*,int> >::iterator it = node1->neighbors.begin();
		for(;it != node1->neighbors.end();++it)
		{
			if(it->first == node2)
			{
				node1->neighbors.erase(it);
				break;
			}
		}
	}
}

void Topology::removeEdge(int i,int j)
{
	/*edgeListIterator iter1,iter2;
	if(findEdge(i,j,iter1))
	{
		//findEdge(j,i,iter2);
		Node *node1 = G.nodes[i];
		Node *node2 = G.nodes[j];
		node1->neighbors.erase(iter1);
		if(findEdge(j,i,iter2))
		{
			node2->neighbors.erase(iter2);
		}
	}*/
	Node *node1 = G.nodes[i];
	Node *node2 = G.nodes[j];
	if(node1 && node2)
	{
		std::list<std::pair<Node*,int> >::iterator it = node1->neighbors.begin();
		for(;it != node1->neighbors.end();++it)
		{
			if(it->first == node2)
			{
				node1->neighbors.erase(it);
				break;
			}
		}
		it = node2->neighbors.begin();
		for(;it != node2->neighbors.end();++it)
		{
			if(it->first == node1)
			{
				node2->neighbors.erase(it);
				break;
			}
		}
	}
}

bool Topology::updateEdgeCost(int i,int j,int newCost)
{
	Node *node1 = G.nodes[i];
	Node *node2 = G.nodes[j];
	if(node1 && node2)
	{
		std::list<std::pair<Node*,int> >::iterator it = node1->neighbors.begin();
		for(;it != node1->neighbors.end();++it)
		{
			if(it->first == node2)
			{
				it->second = newCost;
				return true;
			}
		}
	}
	return false;
}

bool Topology::findEdge(int i,int j,edgeListIterator& foundIter)
{
	Node *node1 = G.nodes[i];
	Node *node2 = G.nodes[j];
	if(node1 && node2)
	{
		//std::list<std::pair<Node*,int> > edges = node1->neighbors;
		std::list<std::pair<Node*,int> >::iterator it = node1->neighbors.begin();
		for(;it != node1->neighbors.end();++it)
		{
			if(it->first == node2)
			{
				foundIter = it;
				return true;
			}
		}
	}
	return false;
}

void Topology::setNextHop(int dest,int nextHop)
{
	routingInfo[dest].next_hop = nextHop;
}
void Topology::setCost(int dest,int cost)
{
	routingInfo[dest].distance = cost;
}
int Topology::getNextHop(int dest)
{
	return routingInfo[dest].next_hop;
}
int Topology::getCost(int dest)
{
	return routingInfo[dest].distance;
}



