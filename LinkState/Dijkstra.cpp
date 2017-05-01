#include "Dijkstra.h"
//#include "topology.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <queue>
#include <map>
#include <functional>
#include <limits>

void writeLine(const std::string& message);

std::vector<int> Dijkstra::dist(256,INFINITI);
std::vector<int> Dijkstra::parent(256,-1);
const int Dijkstra::INFINITI;
extern Topology NetworkTopology;

bool Dijkstra::dijkstraCmp(Node *node1,Node *node2)
{
	std::greater<int> cmp;
	return cmp(dist[node1->value],dist[node2->value]);
}
void Dijkstra::initDistance(int sourceId)
{
	for(int i = 0;i < 256;++i)
	{
		dist[i] = INFINITI;
		parent[i] = -1;
	}
	dist[sourceId] = 0;
	parent[sourceId] = sourceId;
}
void Dijkstra::apply(TopologyGraph G,int sourceId)
{
	initDistance(sourceId);
	NetworkTopology.initRoutingInfo();
	std::vector<int> next_hop(256,-1);
	//lout<<"destination"<<"  "<<"cost"<<"  "<<"next hop\n";
	std::map<Node*,bool> computed;
	next_hop[sourceId] = sourceId;
	std::vector<Node*> Q;
	for(int i = 0;i < 256;++i)
	{
		if(G.nodes[i])
		{
			Q.push_back(G.nodes[i]);
		}
	}
	std::make_heap(Q.begin(), Q.end(),Dijkstra::dijkstraCmp);
	while(!Q.empty())
	{
		std::pop_heap(Q.begin(), Q.end(),Dijkstra::dijkstraCmp);
		Node *node = Q.back();
		Q.pop_back();
		computed[node] = true;
		//lout<<"  "<<node->value<<"            "<<dist[node->value]<<"        "<<next_hop[node->value]<<"\n";
		NetworkTopology.setNextHop(node->value,next_hop[node->value]);
		NetworkTopology.setCost(node->value,dist[node->value]);
		if(node)
		{
			std::list<std::pair<Node*,int> > edges = node->neighbors;
			std::list<std::pair<Node*,int> >::iterator it = edges.begin();
			for(;it != edges.end();++it)
			{
				Node *u = it->first;
				if(computed.find(u) == computed.end())
				{
					if((dist[u->value] > dist[node->value] + it->second) || 
						((dist[u->value] == dist[node->value] + it->second) && node->value < parent[u->value]))
					{
						dist[u->value] = dist[node->value] + it->second;
						parent[u->value] = node->value;
						if(node->value != sourceId)
						{
							next_hop[u->value] = next_hop[node->value];
						}
						else
						{
							next_hop[u->value] = u->value;
						}
					}
				}
			}
			std::make_heap(Q.begin(), Q.end(),Dijkstra::dijkstraCmp);
		}
	}
}
