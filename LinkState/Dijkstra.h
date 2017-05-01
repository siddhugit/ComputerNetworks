#ifndef DIJKSTRA_H__
#define DIJKSTRA_H__

#include "topology.h"
#include <vector>
#include <stdint.h>
#include <limits>

class Dijkstra
{
	static const int INFINITI = INT32_MAX;
	static std::vector<int> dist;
	static std::vector<int> parent;
	static void initDistance(int sourceId);
	static bool dijkstraCmp(Node *node1,Node *node2);
public:
	//static bool dijkstraCmp(Node *node1,Node *node2);
	static void apply(TopologyGraph G,int sourceId);
};

#endif //DIJKSTRA
