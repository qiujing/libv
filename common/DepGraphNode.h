#pragma once
#include <vector>
#include <set>
#include "../beaengine-win32/headers/BeaEngine.h"
using namespace std;
class DepGraphNode
{
public:
	DepGraphNode(void);
	~DepGraphNode(void);

public:
	int ID;
	int instruction_local_id;
	set<int> successors;
	set<int> predecessors;
};

typedef DepGraphNode* PDepGraphNode;