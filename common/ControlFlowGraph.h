#pragma once
#include "BasicBlock.h"
#include "instruction.h"
#include "../vflib2/include/argraph.h"
#include "../vflib2/include/argedit.h"
#include "../vflib2/include/vf2_sub_state.h"
using namespace std;
#include <Windows.h>

typedef struct
{
    int size;
    int index;
} bb_sort;

typedef struct vfnode
{
    bool isBB;
    int data; // instruction_id for instruction; bb_id for BB
} VFNODE;

class ControlFlowGraph
{
public:
    ControlFlowGraph(void);
    ~ControlFlowGraph(void);

    void build(bool for_lib=false);
    void bb_graph();

    void buildDepGraph(bool _lib_mode);
    void dep_graph();

    void build_bb_sorts();
    void serialize();
	
	void buildVLibGraph();
	void buildVLibGraph2();
	void dot_vfgraph(ControlFlowGraph* library_cfg);
#ifdef CEDG
	int buildCEDG(ARGEdit* target_ed, ControlFlowGraph* library_cfg);
	void dot_contract_graph(IN CBitSet* contracted_set);
	void buildVLibGraphForFullIdentification();
	void dot_vfgraphForFull();
	void get_edges_count();
#endif

	//void patch_exchange_instruction();
public:
    BasicBlock* basic_blocks;
    int bb_len;//basic block number
    vector<Instruction> instructions;
#if 0
    bitset<555422> instruction_db_type;// instruction types
#endif   
    ARGEdit vlibARGEdit;
    bb_sort *bb_sorts;

	CRITICAL_SECTION cs;

    bitset<10240> bblen_set;

    // only for libray function
    set<BasicBlock*> head_blocks;
	set<BasicBlock*> tail_blocks;

	size_t MOV_COUNT; // number of MOVs
	size_t CTI_COUNT; // number of CTIs
	size_t ARITHMETIC_COUNT;
	size_t LOGI_COUNT;
	size_t ETC_COUNT;
	size_t STRING_COUNT;

#ifdef CEDG
	size_t edge_count;
	size_t edge_CEDG_count;
	size_t vertices_CEDG_count;
#endif

private:
    int head2bb(int id);
    int add2ins(int addr);
#ifdef CEDG
	void _contract(ARGEdit* ed, PCBitSet contracted_set);
#endif
private:
    bool _dep_built;
    bool _vlib_built;
    bool _build_bb_sorts;
#ifdef CEDG
	vector<VFNODE*> vf_graph;
#endif
};

#define NO_HT 1
#define NO_H2T 2