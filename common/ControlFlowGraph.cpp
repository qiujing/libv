#include "../beaengine-win32/headers/BeaEngine.h"
#include "utility.h"
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <stack>
using namespace std;
#include "bitset.h"
#include "ControlFlowGraph.h"
#include "lock.h"
#include <conio.h>

ControlFlowGraph::ControlFlowGraph(void)
{
    _dep_built = false;
    _vlib_built = false;
    _build_bb_sorts = false;
    bb_sorts = NULL;
    bb_len = 0;
    InitializeCriticalSection(&cs);
    basic_blocks = NULL;

    MOV_COUNT = CTI_COUNT = ARITHMETIC_COUNT = LOGI_COUNT = ETC_COUNT = STRING_COUNT = 0;

#ifdef CEDG
	edge_count = 0;
	edge_CEDG_count = 0;
	vertices_CEDG_count = 0;
#endif
}

ControlFlowGraph::~ControlFlowGraph(void)
{
    if (bb_sorts)
    {
        delete [] bb_sorts;
    }

    DeleteCriticalSection(&cs);
    if (basic_blocks != NULL)
    {
        delete [] basic_blocks;
    }
#ifdef CEDG
    for each (VFNODE * vf in vf_graph)
    {
        delete vf;
    }
#endif
}

static int compare (const void *a, const void *b)
{
    return( *(int *)a - * (int *)b );
}

int compare2 (const void *a, const void *b)
{
    return ((bb_sort *)a)->size - ((bb_sort *)b)->size;
}

//
// add the head of a block
//
void add_head(int* headers, int* len, int h)
{
    for (int i = 0; i < *len; i++)
    {
        if (headers[i] == h)
        {
            return;
        }
    }
    headers[*len] = h;
    (*len)++;
}

/*
build the CFG
*/
void ControlFlowGraph::build(bool for_lib)
{
    int i = 1;
    int j;
    if (basic_blocks) // already built?
    {
        return;
    }

    // step 1 find the headers
    int *headers = new int[instructions.size()];
    headers[0] = 0;
    // all jump target
    for (vector<Instruction>::iterator ite = instructions.begin(); ite != instructions.end(); ite++)
    {
        Instruction & ins = *ite;
        if ((ins.IsCTI()) && (ins.diasm.Instruction.BranchType != 0))
        {
            int target;
            switch (ins.diasm.Instruction.BranchType)
            {
            case CallType:
                break;
            case RetType:
                if (ins.ID + 1 < instructions.size())
                {
                    add_head(headers, &i, ins.ID + 1);
                }
                break;
            default: //jxx?
                if (ins.ID + 1 < instructions.size())
                {
                    add_head(headers, &i, ins.ID + 1);
                }
                // jxx target
                target = add2ins(ins.diasm.Instruction.AddrValue);
                if (target != -1)
                {
                    add_head(headers, &i, target);
                    ins.JMP_ID = target;
                }
                break;
            }
        }
    }
    bb_len = i;
    qsort(headers, bb_len, sizeof(int), compare);
    basic_blocks = new BasicBlock[bb_len];

#define BIT_SET(i, j) bitset_set(g_connective, (i)*bb_len+(j))
#define BIT_ISSET(i, j) bitset_isset(g_connective, (i)*bb_len+(j))
    // step 2 construct basic blocks
    for (j = 0; j < bb_len; j++)
    {
        basic_blocks[j].start = headers[j];
        basic_blocks[j].end = (j + 1 < i) ? (headers[j + 1] - 1) : (instructions.size() - 1);
        basic_blocks[j].ID = j;
        basic_blocks[j].cfg = this;
    }
    delete [] headers;
    // step 3 compute relations
    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
        Instruction ins = instructions[bb.end];
        int next_id = bb.ID + 1;
        if ((ins.diasm.Instruction.Category & CONTROL_TRANSFER) && (ins.diasm.Instruction.BranchType != 0))
        {
            int bb_target;
#define NEXT(); \
    if (next_id<bb_len) {\
        bb.succeccors.insert(&basic_blocks[next_id]); \
        basic_blocks[next_id].predecessors.insert(&bb); \
    }
#define BRANCH(); \
    if (ins.JMP_ID>=0) {\
        bb_target = head2bb(ins.JMP_ID); \
        if (bb_target!=-1) {\
            bb.succeccors.insert(&basic_blocks[bb_target]); \
            basic_blocks[bb_target].predecessors.insert(&bb); \
        } \
        else \
        { \
            NEXT(); \
        } \
    } \
    else \
    { \
        NEXT(); \
    }
            switch (ins.diasm.Instruction.BranchType)
            {
            case JmpType:
                BRANCH();
                break;
            case RetType:
                break;
            case CallType:
                NEXT();
                break;
            default://jxx
                BRANCH();
                NEXT();
                break;
            }
        }
        else
        {
            NEXT();
        }
#if 1
        if (!for_lib)
        {
            int l = basic_blocks[i].size();
            bblen_set.set(l > 10240 ? 10239 : l);
        }
#endif
    }
#if 1
    if (for_lib)
    {
        for (i = 0; i < bb_len; i++)
        {
            if (basic_blocks[i].isBody())
            {
                int l = basic_blocks[i].size();
                bblen_set.set(l > 10240 ? 10239 : l);
            }
        }
    }
#endif
}

// search the block that contains a head
int ControlFlowGraph::head2bb(int insID)
{
    int low = 0;
    int high = bb_len - 1;
    while (low <= high)
    {
        int mid = (low + high) / 2;
        int v = basic_blocks[mid].start;
        if (v < insID)
            low = mid + 1;
        else if (v > insID)
            high = mid - 1;
        else
            return mid;
    }
    return -1;
}

// search the instruction at the addr
int ControlFlowGraph::add2ins( int addr )
{
    int low = 0;
    int high = instructions.size() - 1;
    while (low <= high)
    {
        int mid = low + ((high - low) / 2);
        int v = instructions[mid].diasm.EIP;
        if (v < addr)
        {
            low = mid + 1;
        }
        else if (v > addr)
        {
            high = mid - 1;
        }
        else
        {
            return instructions[mid].ID;
        }
    }
    return -1;
}

// control flow graph in dot
void ControlFlowGraph::bb_graph()
{
    stringstream dot;
    int i, j;
    dot << "digraph G {node [shape=box];" << endl;

    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
		dot << "n" << bb.ID << "[label=\""; //<< bb.ID << "-";
        for (j = bb.start; j <= bb.end; j++)
        {
            dot << 
				//instructions[j].ID << "-" << 
				instructions[j].diasm.CompleteInstr << "\\l";
        }
        dot << "\"];" << endl;
    }

    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
        for (set<BasicBlock*>::iterator ite2 = bb.succeccors.begin(); ite2 != bb.succeccors.end(); ite2++)
        {
            BasicBlock * bb2 = *ite2;
            dot << "n" << bb.ID << "-> n" << bb2->ID << ";" << endl;
        }
    }
    dot << "}" << endl;
    dot_graph(dot);
}

//
// build EDG
//
void ControlFlowGraph::buildDepGraph(bool _lib_mode)
{
    int i;
    CLock lock(cs);

    if (_dep_built)// already built?
    {
        return;
    }

    for (i = 0; i < bb_len; i++)
    {
        basic_blocks[i].buildDepGraph(_lib_mode);
    }

    _dep_built = true;
}

//
// emit the global EDG in dot
//
void ControlFlowGraph::dep_graph()
{
    //#ifndef _DEBUG
    //    return;
    //#endif
    stringstream dot;
    int i;
    dot << "digraph G {" << endl;

    // 1. vertex: instructions
    for each (Instruction ins in instructions)
    {
        dot << "n" << ins.ID << "[label=\"" << ins.ID;
        switch (ins.db_id)
        {
        case -1:
            dot << ":Entry\",style=filled];";
            break;
        case -2:
            dot << ":Exit\",style=filled];";
            break;
        default:
            dot << ":" << ins.diasm.CompleteInstr << " " << hex << ins.db_id << dec << "\"];";
            break;
        }
        dot << endl;
    }

    // 2. local EDG
    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
        int L = bb.size();
        if (L == 1) continue;
        dot << "subgraph cluster_" << bb.ID << " { color=blue;label=\"" << bb.ID << "\";" << endl;
        for (int j = 0; j < L; j++)
        {
            for each(int dn in bb.node[j].successors)
            {
                if (dn >= L)
                {
                    continue;
                }
                dot << "n" << bb.start + bb.node[j].instruction_local_id << "-> n" << bb.start + dn << "[color=red];" << endl;
            }
        }

        if (bb.hasEntry)
        {
            for each(int dn in bb.node[L].successors)
            {
                dot << "n" << bb.Entry << "-> n" << bb.start + dn << "[color=red,style = dotted];" << endl;
            }
        }

        if (bb.hasExit)
        {
            for each (int dn in bb.node[L + 1].predecessors)
            {
                dot << "n" << bb.start + dn << "-> n" << bb.Exit << "[color=red,style = dotted];" << endl;
            }
        }
        dot << "}" << endl;
    }

    // 3. edges among local EDGs
    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
        for each(BasicBlock * suc in bb.succeccors)
        {
            if (bb.hasExit)
            {
                dot << "n" << bb.Exit;
            }
            else
            {
                dot << "n" << bb.end ;
            }

            dot << "->";

            if (suc->hasEntry)
            {
                dot << "n" << suc->Entry;
            }
            else
            {
                dot << "n" << suc->start ;
            }
            dot << "[color=green4];" << endl;
        }
    }
    // 4. end
    dot << "}" << endl;
    //cout << dot.str();
    dot_graph(dot);
}

// build graph for VFLib
void ControlFlowGraph::buildVLibGraph()
{
    int i;
    CLock lock(cs);

    ARGEdit *ed = &vlibARGEdit;
    if (_vlib_built)
    {
        return;
    }
    // node
    for (i = 0; i < instructions.size(); i++)
    {
        ed->InsertNode((void*)instructions[i].db_id);
    }

    // 2. edges in local EDGs
    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
        int L = bb.size();
        if (L == 1) continue;
        for (int j = 0; j < L; j++)
        {
            for each(int dn in bb.node[j].successors)
            {
                if (dn >= L)
                {
                    continue;
                }
                ed->InsertEdge(bb.start + j, bb.start + dn, NULL);
            }
        }

        if (bb.hasEntry)
        {
            for each(int dn in bb.node[L].successors)
            {
                ed->InsertEdge(bb.Entry, bb.start + dn, NULL);
            }
        }
        if (bb.hasExit)
        {
            for each(int dn in bb.node[L + 1].predecessors)
            {
                ed->InsertEdge(bb.start + dn, bb.Exit, NULL);
            }
        }
    }

    // 3. edges among local EDGs
    for (i = 0; i < bb_len; i++)
    {
        BasicBlock & bb = basic_blocks[i];
        for each(BasicBlock * suc in bb.succeccors)
        {
            if (&bb == suc)
            {
                continue;
            }
            int start = bb.hasExit ? bb.Exit : bb.end;
            int end = suc->hasEntry ? suc->Entry : suc->start;
            ed->InsertEdge(start, end, NULL);
        }
    }
    _vlib_built = true;
}

//
// sort local EDGs
//
void ControlFlowGraph::build_bb_sorts()
{
    CLock lock(cs);
    if (_build_bb_sorts)
    {
        return;
    }
    int L = bb_len;
    bb_sorts = new bb_sort[L];

    for (int i = 0; i < L; i++)
    {
        bb_sorts[i].size = basic_blocks[i].size();
        bb_sorts[i].index = i;
    }
    qsort(bb_sorts, L, sizeof(bb_sort), compare2);
    _build_bb_sorts = true;
}

//
// serialize
//
void ControlFlowGraph::serialize()
{
    CLock lock(cs);
    for (int i = 0; i < bb_len; i++)
    {
        basic_blocks[i].serialize();
    }
}

#ifdef CEDG
//************************************
// Method:    buildVLibGraphForFullIdentification
// FullName:  ControlFlowGraph::buildVLibGraphForFullIdentification
// Access:    public
// Returns:   int
// Qualifier:
// Parameter: ARGEdit * ed
//************************************
void ControlFlowGraph::buildVLibGraphForFullIdentification()
{
    CLock lock(cs);

    if (_vlib_built)
        return;

    ARGEdit *ed = &vlibARGEdit;
	CBitSet C(bb_len);
	for (int i = 0; i < bb_len; i++)
	{
		BasicBlock & bb = basic_blocks[i];
		if (!bb.has_uncertain_vertexSeq)
		{
			C.set(bb.ID);
		}
	}
	
	_contract(ed, &C);

    _vlib_built = true;
}


void ControlFlowGraph::dot_vfgraphForFull()
{
	CBitSet C(bb_len);
	for (int i = 0; i < bb_len; i++)
	{
		BasicBlock & bb = basic_blocks[i];
		if (!bb.has_uncertain_vertexSeq)
		{
			C.set(bb.ID);
		}
	}

	dot_contract_graph(&C);
}

void ControlFlowGraph::dot_contract_graph(IN CBitSet* contracted_set)
{
    //#ifndef _DEBUG
    //    return;
    //#endif
    stringstream dot;
    dot << "digraph G {node [shape=box];" << endl;
    int id_count = 0;

    int * id_vf = new int[bb_len];
    int *start_id = new int[bb_len];

    for (int i = 0; i < bb_len; i++)
    {
        BasicBlock *bb = &basic_blocks[i];

        if (!contracted_set->isset(bb->ID))
        {
            int L = bb->size();
            int start = id_count;
            // instructions to vf graph
            for (int j = bb->start; j <= bb->end; j++)
            {
                dot << "n" << id_count << "[label=\"" << id_count;
                dot << ":" << instructions[j].diasm.CompleteInstr << "\"];";
                dot << endl;
                id_count++;
            }
            if (bb->hasEntry)
            {
                dot << "n" << id_count << "[label=\"" << id_count;
                dot << ":Entry\",style=filled];";
                dot << endl;
                id_count++;
            }
            if (bb->hasExit)
            {
                dot << "n" << id_count << "[label=\"" << id_count;
                dot << ":Exit\",style=filled];";
                dot << endl;
                id_count++;
            }
            // edges of this block
            if (L > 1)
            {
                for (int j = 0; j < L; j++)
                {
                    for each (int dn in bb->node[j].successors)
                    {
                        if (dn >= L)
                        {
                            continue;
                        }
                        int s = start + j;
                        int ee = start + dn;
                        dot << "n" << s << " -> n" << ee << ";" << endl;
                    }
                }
                if (bb->hasEntry)
                {
                    for each(int dn in bb->node[L].successors)
                    {
                        dot << "n" << start + L << "-> n" << start + dn << ";" << endl;
                    }
                }
                if (bb->hasExit)
                {
                    for each (int dn in bb->node[L+1].predecessors)
                    {
						dot << "n" << start + dn << " -> n" << start + L + (bb->hasEntry ? 1 : 0) << ";" << endl;
                    }
                }
            }
            start_id[bb->ID] = start;
        }
        else // not found?
        {
            dot << "n" << id_count << "[label=\"";
            for each (PDepGraphNode pdn in bb->vertexSeq)
            {
                dot << hex << pdn->ID << ",";
            }
            dot << dec;
            dot << "\",style=filled];";
            dot << endl;
            id_vf[bb->ID] = id_count;
            id_count++;
        }
    }

    // step 2: add edges
    for (int i = 0; i < bb_len; i++)
    {
        BasicBlock *bb = &basic_blocks[i];
        int from, to;

        if (bb->succeccors.size() == 0)
        {
            continue;
        }

        // from
        if (!contracted_set->isset(bb->ID)) // not collapsed
        {
            if (bb->hasExit)
            {
                from = start_id[bb->ID] + bb->size() + (bb->hasEntry ? 1 : 0); // L + 1
            }
            else
            {
                from = start_id[bb->ID] + bb->end - bb->start; // the last one
            }
        }
        else // collapsed
        {
            from = id_vf[bb->ID];
        }
        // to
        for each (BasicBlock * suc in bb->succeccors)
        {
            if (suc == bb) continue;
            if (!contracted_set->isset(suc->ID)) // next one is also not collapsed?
            {
                if (suc->hasEntry)
                {
                    to = start_id[suc->ID] + suc->size(); // L
                }
                else
                {
                    to = start_id[suc->ID]; // the first one
                }
            }
            else // next one is collapsed?
            {
                to = id_vf[suc->ID];
            }
            dot << "n" << from << " -> n" << to << ";" << endl;
        }
    }
    dot << "}" << endl;
    dot_graph(dot);

    delete[] id_vf;
    delete[] start_id;
}

int ControlFlowGraph::buildCEDG(ARGEdit* target_ed , ControlFlowGraph* library)
{
	CLock lock(cs);
    if (this == library && _vlib_built) // library functions only built(L, L) once.
        return 0;

    CBitSet target_contracted_set(bb_len); // local EDGs of G_T that will be contracted
    set<BasicBlock*> candidate_H;
    set<BasicBlock*> candidate_T;

    // step 0: all local EDGs are supposed to be contracted initially.
    for (int i = 0; i < bb_len; i++)
    {
        BasicBlock *bb = &basic_blocks[i];
		if (!bb->has_uncertain_vertexSeq)
        {
            target_contracted_set.set(bb->ID);
        }
    }

#if 0
    dep_graph();
#endif

    bool found = false;
    // step 2: identify candidate H and T in G_T
    for each (BasicBlock * head in library->head_blocks)
    {
        vector<BasicBlock*> candidator;
        for (int i = 0; i < bb_len; i++)
        {
            BasicBlock *bb = &basic_blocks[i];
			//if (!bb->has_uncertain_vertexSeq) // bb has a chance to be contracted.
            {
                // check whether bb contains all instructions of head.
                if (bb->is_contained_all_instructions(head))
                {
                    found = true;
                    target_contracted_set.reset(bb->ID);
                    candidate_H.insert(bb);
                }
            }
        }
    }

    if ((library->head_blocks.size() > 0) && (!found))
    {
        return NO_HT;
    }

    if (library->bb_len > 1)
    {
        found = false;
        for each (BasicBlock * tail in library->tail_blocks)
        {
            bool candidators_should_be_contracted = true;
            vector<BasicBlock*> candidator;
            for (int i = 0; i < bb_len; i++)
            {
                BasicBlock *bb = &basic_blocks[i];
				//if (!bb->has_uncertain_vertexSeq) // bb has a chance to be contracted.
                {
                    // check whether bb contains all instructions of head.
                    if (bb->is_contained_all_instructions(tail))
                    {
                        found = true;
                        target_contracted_set.reset(bb->ID);
                        candidate_T.insert(bb);
                    }
                }
            }
        }

        if ((library->tail_blocks.size() > 0) && (!found))
        {
            return NO_HT;
        }
    }

    // step3: remove all isolated candidates of H and T
    if (library->bb_len == 1) // head and tail in a same block
    {
    }
    else if (library->head_blocks.size() > 0 && library->tail_blocks.size() == 0)
    {

    }
    else if (library->head_blocks.size() == 0 && library->tail_blocks.size() > 0) // first block jumps to itself
    {

    }
    else if (library->head_blocks.size() > 0 && library->tail_blocks.size() > 0)
    {
        bool found = false;
        // check H->T
        CBitSet visited(bb_len);
        CBitSet marked(bb_len);
        for each (BasicBlock * from in candidate_H)
        {
            stack<BasicBlock*> s;
            s.push(from);

            visited.reset_all();
            while (!s.empty())
            {
                BasicBlock* bb = s.top();
                s.pop();

                visited.set(bb->ID);
                if (candidate_T.find(bb) != candidate_T.end())
                {
                    marked.set(from->ID);
                    marked.set(bb->ID);
                    found = true;
                }

                for each(BasicBlock * suc in bb->succeccors)
                {
                    if (!visited.isset(suc->ID))
                    {
                        s.push(suc);
                    }
                }
            }
        }

        for each (BasicBlock * cd in candidate_H)
        {
            if (!marked.isset(cd->ID)) // cd is isolated?
            {
                target_contracted_set.set(cd->ID);
            }
        }

        if (!marked.any())
        {
            return NO_H2T;
        }

        for each (BasicBlock * cd in candidate_T)
        {
            if (!marked.isset(cd->ID)) // cd is isolated?
            {
                target_contracted_set.set(cd->ID);
            }
        }
    }

#if 0
    dot_contract_graph(&target_contracted_set);
#endif
    _contract(target_ed, &target_contracted_set);

    _vlib_built = true;

    return 0;
}

void ControlFlowGraph::_contract(ARGEdit* ed, PCBitSet contracted_set)
{
    int * id_vf = new int[bb_len];
    int *start_id = new int[bb_len];

    for each (VFNODE * vf in vf_graph)
    {
        delete vf;
    }
    vf_graph.clear();

	edge_CEDG_count = 0;
	vertices_CEDG_count = 0;

    for (int i = 0; i < bb_len; i++)
    {
        BasicBlock *bb = &basic_blocks[i];

        if (!contracted_set->isset(bb->ID))
        {
            int start = vf_graph.size();
			vertices_CEDG_count += bb->size();
            // instructions to vf graph
            for (int j = bb->start; j <= bb->end; j++)
            {
                VFNODE *vf = new VFNODE;
                vf->isBB = false;
                vf->data = instructions[j].db_id;
                vf_graph.push_back(vf);
                ed->InsertNode(vf);				
            }

            if (bb->hasEntry)
            {
                VFNODE *vf = new VFNODE;
                vf->isBB = false;
                vf->data = -1;
                vf_graph.push_back(vf);
                ed->InsertNode(vf);
				vertices_CEDG_count++;
            }

            if (bb->hasExit)
            {
                VFNODE *vf = new VFNODE;
                vf->isBB = false;
                vf->data = -2;
                vf_graph.push_back(vf);
                ed->InsertNode(vf);
				vertices_CEDG_count++;
            }

            int L = bb->size();

            // edges of this block
            if (L > 1)
            {
                for (int j = 0; j < L; j++)
                {
                    for each (int dn in bb->node[j].successors)
                    {
                        if (dn >= L)
                        {
                            continue;
                        }
                        int s = start + j;
                        int ee = start + dn;
                        ed->InsertEdge(s,
                                       ee,
                                       NULL);
						edge_CEDG_count++;
                    }
                }

                if (bb->hasEntry)
                {
					edge_CEDG_count += bb->node[L].successors.size();
                    for each(int dn in bb->node[L].successors)
                    {
                        ed->InsertEdge(start + L,
                                       start + dn,
                                       NULL);						
                    }
                }

                if (bb->hasExit)
                {
                    //int exit_index = L + (bb->hasEntry ? 1 : 0);
					edge_CEDG_count += bb->node[L+1].predecessors.size();
					for each (int dn in bb->node[L + 1].predecessors)
                    {
                        ed->InsertEdge(start + dn,
							start + L + (bb->hasEntry ? 1 : 0),
                                       NULL);
                    }
                }
            }
            start_id[bb->ID] = start;
        }
        else // not found?
        {
            // collapse to a node
            VFNODE *vf = new VFNODE;
            vf->isBB = true;
            vf->data = (int)bb;
            id_vf[bb->ID] = vf_graph.size();
            vf_graph.push_back(vf);
            ed->InsertNode(vf);
			vertices_CEDG_count++;
        }
    }

    // step 2: add edges
    for (int i = 0; i < bb_len; i++)
    {
        BasicBlock *bb = &basic_blocks[i];
        int from, to;

        if (bb->succeccors.size() == 0)
        {
            continue;
        }
		
		edge_CEDG_count += bb->succeccors.size();

        // from
        if (!contracted_set->isset(bb->ID)) // not contracted?
        {
            // 'from' is the last node
            if (bb->hasExit)
            {
                from = start_id[bb->ID] + bb->size() + (bb->hasEntry ? 1 : 0); // L + 1
            }
            else
            {
                from = start_id[bb->ID] + bb->end - bb->start; // the last one
            }
        }
        else // contracted?
        {
            from = id_vf[bb->ID];
        }

        // to
        for each (BasicBlock * suc in bb->succeccors)
        {
            if (suc == bb) continue;
            if (!contracted_set->isset(suc->ID)) // next one is also not collapsed?
            {
                if (suc->hasEntry)
                {
                    to = start_id[suc->ID] + suc->size(); // L
                }
                else
                {
                    to = start_id[suc->ID]; // the first one
                }
            }
            else // next one is collapsed?
            {
                to = id_vf[suc->ID];
            }
            // insert edge (start, end)
            ed->InsertEdge(from, to, NULL);
        }
    }
    delete[] id_vf;
    delete[] start_id;
}

void ControlFlowGraph::get_edges_count()
{
	int i;

	if (edge_count>0) // library functions only built(L, L) once.
		return;

	CLock lock(cs);

	// 2. local EDG
	for (i = 0; i < bb_len; i++)
	{
		BasicBlock & bb = basic_blocks[i];
		int L = bb.size();
		if (L == 1) continue;
		for (int j = 0; j < L; j++)
		{
			for each (int dn in bb.node[j].successors)
			{
				if (dn >= L)
				{
					continue;
				}
				
				edge_count++;
			}
		}

		if (bb.hasEntry)
		{
			edge_count += bb.node[L].successors.size();
		}

		if (bb.hasExit)
		{
			edge_count += bb.node[L + 1].predecessors.size();
		}
	}

	// 3. edges among local EDGs
	for (i = 0; i < bb_len; i++)
	{
		BasicBlock & bb = basic_blocks[i];
		edge_count += bb.succeccors.size();
	}
}

#endif

