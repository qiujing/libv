#pragma once
/********************************************************************
	created:	2012/04/19
	created:	19:4:2012   10:50
	filename: 	d:\kuaipan\study\lib\lib\BasicBlock.h
	file path:	d:\kuaipan\study\lib\lib
	file base:	BasicBlock
	file ext:	h
	author:		qj

	purpose:	basic block
*********************************************************************/
#ifndef BB_HEADER
#define BB_HEADER
#include "DepGraphNode.h"
#include "bitset.h"
class BasicBlock
{
public:
    BasicBlock(void);
    ~BasicBlock(void);
    void buildDepGraph(bool _lib_mode);
    int size();
    void serialize();
    void dump_serial_result();
    bool isHead();
    bool isTail();
    bool isBody();
	DepGraphNode* getEntryNode();
	bool is_contained_all_instructions(BasicBlock* bb2);
	bool hasDependence(int v1, int v2);
public:
    int start; // start instruction index
    int end; // end instruction index
    int ID;
    set<BasicBlock*> succeccors;
    set<BasicBlock*> predecessors;
    PCBitSet depGraph;
    void* cfg;
    bool hasEntry;
    bool hasExit;
    int Entry;
    int Exit;
    vector<DepGraphNode> node;
	vector<PDepGraphNode> vertexSeq;// vertex sequence
#ifdef CEDG
	PCBitSet OAM; // ordered adjacent matrix
	bool equal(BasicBlock* bb2);
	bool has_uncertain_vertexSeq;
#endif
private:
    bool _serialized;
};

#endif
