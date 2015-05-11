#include <set>
#include <vector>
#include <stack>
#include <iostream>
using namespace std;

#include "bitset.h"
#include "ControlFlowGraph.h"
#include "BasicBlock.h"
#include "define.h"

bool trace_mode = false;
//extern bool lib_mode;

BasicBlock::BasicBlock(void)
{
    hasEntry = false;
    hasExit = false;
    _serialized = false;
    depGraph = NULL;
#ifdef CEDG
    OAM = NULL;
    has_uncertain_vertexSeq = false;
#endif
}

BasicBlock::~BasicBlock(void)
{
    if (depGraph)
    {
        delete depGraph;
    }

#ifdef CEDG
    if (OAM)
    {
        delete OAM;
    }
#endif // CEDG
}

extern void print_(bitset<VAR_LENGTH> &b);

#if 0
void BasicBlock::buildDepGraph(bool _lib_mode)
{
    int i, j, k;
    int L = size();
    ControlFlowGraph *_cfg = (ControlFlowGraph *)cfg;

    if (L == 1)
    {
        if (isHead())
        {
            if (predecessors.size() == 0)
            {
                _cfg->head_blocks.insert(this);
            }
        }

        if (isTail())
        {
            if (!_cfg->instructions[end].IsCTI())
            {
                _cfg->tail_blocks.insert(this);
            }
        }

        DepGraphNode dn;
        dn.instruction_local_id = 0;
        dn.ID = _cfg->instructions[start].db_id;

        node.push_back(dn);
        return;
    }
    //depGraph = new CBitSet(L * L);
    for (i = 0; i < L ; i++)
    {
        DepGraphNode dn;
        dn.instruction_local_id = i;
        dn.ID = _cfg->instructions[start + i].db_id;
        node.push_back(dn);
    }

    for (i = 0; i < L - 1; i++)
    {
        node[i].successors.insert(i + 1);
        node[i + 1].predecessors.insert(i);
    }
}
#endif
#if 1
// build dependence graph
void BasicBlock::buildDepGraph(bool _lib_mode)
{
    int i, j, k;
    int L = size();
    ControlFlowGraph *_cfg = (ControlFlowGraph *)cfg;

    if (L == 1)
    {
        if (isHead())
        {
            if (predecessors.size() == 0)
            {
                _cfg->head_blocks.insert(this);
            }
        }

        if (isTail())
        {
            if (!_cfg->instructions[end].IsCTI())
            {
                _cfg->tail_blocks.insert(this);
            }
        }

        DepGraphNode dn;
        dn.instruction_local_id = 0;
        dn.ID = _cfg->instructions[start].db_id;

        node.push_back(dn);
        return;
    }

    bool f = _cfg->instructions[end].IsCTI();

#define EXIT_NODE (L+1)
#define ENTRY_NODE (L)

    // allocate memory for depGraph
    depGraph = new CBitSet(L * L);
    for (i = 0; i < L + 2; i++)
    {
        DepGraphNode dn;
        dn.instruction_local_id = i;
        dn.ID = i < L ? _cfg->instructions[start + i].db_id : -1;
        node.push_back(dn);
    }

#define BIT_SET(i,j) depGraph->set((i)*L+(j))
#define BIT_RESET(i,j) depGraph->reset((i)*L+(j))
#define BIT_ISSET(i,j) depGraph->isset((i)*L+(j))
#define W(i) _cfg->instructions[start+(i)].W
#define R(i) _cfg->instructions[start+(i)].R

#if 0
    // live-variable analysis
    bitset<VAR_LENGTH> *LV_OUT = new bitset<VAR_LENGTH>[L];
    LV_OUT[L - 1].reset();
    bitset<VAR_LENGTH> t;

    for (i = L - 2; i >= 0; i--)
    {
        t = W(i);
        t.flip();
        LV_OUT[i] = LV_OUT[i + 1];
        LV_OUT[i] &= t;
        LV_OUT[i] |= R(i + 1);
    }
#endif

#if 0
    if (trace_mode)
    {
        for (i = 0; i < L; i++)
        {
            Instruction &ins = _cfg->instructions[start + i];
            cout << ins.diasm.CompleteInstr << endl;
            ins.print_R();
            ins.print_W();
            cout << "OUT:";
            print_(LV_OUT[i]);
            cout << "--------------------" << endl;
        }
    }
#endif

    // create the initial graph
    for (i = 0; i < L; i++)
    {
        for (j = i + 1; j < L; j++)
        {
            Instruction& ins1 = _cfg->instructions[start + i];
            Instruction& ins2 = _cfg->instructions[start + j];
            if (ins1.hasDependency(ins2))
            {
                BIT_SET(i, j);
#if 0
                t = R(i);
                t &= W(j);

                if (t.none())
                {
                    t = W(i);
                    t &= R(j);
                }
                if (t.none())
                {
                    t = W(i);
                    t &= W(j);
                    bitset<VAR_LENGTH> t1, t2;
                    t1 = LV_OUT[i];
                    t1.flip();
                    t2 = LV_OUT[j];
                    t2.flip();
                    t1 &= t2;
                    t1 &= t;
                    if (t1.any())
                    {
                        BIT_RESET(i, j);
                    }
                }
#endif
            }
        }

        // all instruction point to the tail
        if (f)
        {
            BIT_SET(i, L - 1);
        }
    }

    // simplification
    for (i = 0; i < L; i++)
    {
        for (j = i + 1; j < L; j++)
        {
            if (BIT_ISSET(i, j)) // i->j?
            {
                for (k = i + 1; k < j; k++)
                {
                    if (BIT_ISSET(i, k) && BIT_ISSET(k, j)) // i->k,k->j?
                    {
                        BIT_RESET(i, j); // delete i->j
                        break;
                    }
                }

                if (k == j) // i->j?
                {
                    node[i].successors.insert(j);
                    node[j].predecessors.insert(i);
                }
            }
        }
    }

    // need entry: if more than one vertex has no predecessors.
    // need exit: if more than one vertex has no successors.
    vector<int> s1;
    vector<int> s2;

#if 1
    for (i = 0; i < L; i++)
    {
        if (node[i].predecessors.size() == 0) // entry -> i?
        {
            s1.push_back(i);
        }

        if (node[i].successors.size() == 0) // i -> exit?
        {
            s2.push_back(i);
        }
    }
#endif
    hasEntry = true;// s1.size() > 1;
    hasExit = true;// s2.size() > 1;
    if (_lib_mode) // lib function?
    {
        if (isHead())//(ID == 0)
        {
            if (this->predecessors.size() == 0)
            {
                _cfg->head_blocks.insert(this);
            }
            hasEntry = false;
        }

        if (isTail())//(ID == _cfg->bb_len-1)
        {
            if (!_cfg->instructions[end].IsCTI())
            {
                _cfg->tail_blocks.insert(this);
            }

            hasExit = false;
        }
    }

    // add entry or exit
    if (hasEntry)
    {
        for each(int id in s1)
        {
            node[ENTRY_NODE].successors.insert(id);
            node[id].predecessors.insert(ENTRY_NODE);
        }
        Instruction ENTRY;
        ENTRY.ID = _cfg->instructions.size();
        ENTRY.db_id = -1;
        _cfg->instructions.push_back(ENTRY);
        Entry = ENTRY.ID;
    }

    if (hasExit)
    {
        for each(int id in s2)
        {
            node[id].successors.insert(EXIT_NODE);
            node[EXIT_NODE].predecessors.insert(id);
        }
        Instruction EXIT;
        EXIT.ID = _cfg->instructions.size();
        EXIT.db_id = -2;
        _cfg->instructions.push_back(EXIT);
        Exit = EXIT.ID;
    }
}
#endif

//************************************
// Method:    get the number of instructions
// FullName:  BasicBlock::size
// Access:    public
// Returns:   the number of instructions of this BB
//************************************
int BasicBlock::size()
{
    return end - start + 1;
}

int compare(const void *a, const void *b)
{
    PDepGraphNode gn1 = *(PDepGraphNode *)a;
    PDepGraphNode gn2 = *(PDepGraphNode *)b;
    return ((*(PDepGraphNode *)a)->ID - (*(PDepGraphNode *)b)->ID);
}

// serialization
void BasicBlock::serialize()
{
    int i;

    if (vertexSeq.size() > 0)
    {
        return;
    }

    ControlFlowGraph *_cfg = (ControlFlowGraph *)cfg;
    int L = size();
    if (L == 1) // only one vertex?
    {
        vertexSeq.push_back(&node[0]);
        return;
    }

    // create graph from current vertex
    set<int> G; // node index

    for (i = 0; i < L; i++)
    {
        G.insert(i);
    }

    PDepGraphNode *work_list = new PDepGraphNode[node.size()];// candidate set of vertices whose incoming degree is 0
    int work_list_i = 0;

    while (!G.empty())
    {
        // step 1: search vertices whose in degree is 0
        work_list_i = 0;
        for each(int v1 in G)
        {
            bool found = true;
            for each(int v2 in node[v1].predecessors)
            {
                if (G.find(v2) != G.end()) // v2->v1, v2 in current graph?
                {
                    found = false;
                    break;
                }
            }

            if (found) // found one?
            {
                work_list[work_list_i++] = &node[v1];
            }
        }

        for (i = 0; i < work_list_i; i++)
        {
            G.erase(work_list[i]->instruction_local_id);
        }

        // step 2: sort and output
        if (work_list_i == 0) // work list is empty?
        {
            break;
        }

        if (work_list_i == 1) // only one candidate?
        {
            vertexSeq.push_back(work_list[0]);
        }
        else
        {
            // quick sort
            qsort(work_list, work_list_i, sizeof(PDepGraphNode), compare);

            int last_id = 0;
            for (i = 0; i < work_list_i; i++)
            {
                vertexSeq.push_back(work_list[i]);
#ifdef CEDG
                if (work_list[i]->ID == last_id)
                {
                    has_uncertain_vertexSeq = true;
                }
                last_id = work_list[i]->ID;
#endif
            }
        }
    }

    delete[] work_list;

#ifdef CEDG
    if (!has_uncertain_vertexSeq)
    {
        int LEN = vertexSeq.size();
        assert(LEN == L);

        OAM = new CBitSet(LEN * LEN);
        for (i = 0; i < LEN; i++)
        {
            PDepGraphNode dn = vertexSeq[i];
            for each (int suc in dn->successors)
            {
                // seach suc in vertexSeq
                for (int j = i + 1; j < LEN; j++)
                {
                    if (vertexSeq[j]->instruction_local_id == suc)
                    {
                        OAM->set(i * LEN + j);
                        break;
                    }
                }
            }
        }
    }
#endif
}

// print the sequence
void BasicBlock::dump_serial_result()
{
    for each(PDepGraphNode dn in vertexSeq)
    {
        cout << dn->ID << " ";
    }
    cout << endl;
}

// is head?
bool BasicBlock::isHead()
{
    return (predecessors.size() == 0) || (start == 0);
}

// is tail?
bool BasicBlock::isTail()
{
    return (succeccors.size() == 0);
}

//************************************
// Method:    check if the basic block is a mid block.
// FullName:  BasicBlock::isBody
// Access:    public
// Returns:   bool
// Qualifier:
//************************************
bool BasicBlock::isBody()
{
    return (!isHead()) && (!isTail());
}

DepGraphNode *BasicBlock::getEntryNode()
{
    return &node[hasEntry ? size() : 0];
}

//
// v1 and v2 are local id
//
bool BasicBlock::hasDependence(int v1, int v2)
{
    return depGraph->isset((v1) * size() + (v2));
}

bool BasicBlock::is_contained_all_instructions(BasicBlock* bb2)
{
    if (size() < bb2->size())
    {
        return false;
    }

    set<int> ins;
    for (int j = start; j <= end; j++)
    {
        ins.insert(j);
    }

    bool all_in = true;
    for (int i = bb2->start; i <= bb2->end; i++)
    {
        bool one_can_found = false;
        for each (int ins_id in ins)
        {
            if (((ControlFlowGraph *)bb2->cfg)->instructions[i].db_id == ((ControlFlowGraph *)cfg)->instructions[ins_id].db_id) // found ?
            {
                one_can_found = true;
                ins.erase(ins_id);
                break;
            }
        }

        if (!one_can_found)
        {
            all_in = false;
            break;
        }
    }

    return all_in;
}

#ifdef CEDG
bool BasicBlock::equal(BasicBlock* bb2)
{
    if (size() != bb2->size())
    {
        return false;
    }

    //bb->serialize();

    // compare IDS
    if (vertexSeq.size() != bb2->vertexSeq.size())
    {
        return false;
    }

    bool bEqual = true;
    for (int i = 0; i < vertexSeq.size(); i++)
    {
        if (vertexSeq[i]->ID != bb2->vertexSeq[i]->ID)
        {
            bEqual = false;
            break;
        }
    }

    if (!bEqual)
    {
        return false;
    }

    // compare OAM
    if ((vertexSeq.size() > 1) && (OAM != NULL) && (bb2->OAM != NULL))
    {
        return OAM->equal(bb2->OAM);
    }

    return true;
}

#endif