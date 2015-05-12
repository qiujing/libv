#include "shared.h"
#include "inline_lib.h"

extern bool limit_size;
extern unsigned int lib_count;

extern bool bFull;
extern char* lib_root_path;

extern CRITICAL_SECTION cs;
extern int total_function_len;

unsigned int BKDRHash(char* str)
{
    unsigned int seed = 23;
    unsigned int hash = 0;

    while ((*str != 0) && (*str != ' '))
    {
        hash = hash * seed + (*str++);
    }

    return (hash & 0xFFFF);
}

//
// disassemble
// bin: binary code
// length: code length
// for_lib: is a library function?
// return: control flow graph
void* disasm(byte* bin, int length, bool for_lib, char* lib_name, bool is_inlined_function, char* from_file)
{
    if (limit_size && length < MIN_INS_LENGTH)
    {
        return NULL;
    }

#if 0
    {
        if (lib_name && strcmp(lib_name, "_strncpy"))
        {
            return NULL;
        }
    }
#endif

#if 0
    if (lib_name)
    {
        if (!strcmp(lib_name, "___report_gsfailure"))
        {
            return NULL;
        }
        if (!strcmp(lib_name, "__invoke_watson"))
        {
            return NULL;
        }
        if (!strcmp(lib_name, "___strgtold12_l"))
        {
            return NULL;
        }
        if (!strcmp(lib_name, "_$I10_OUTPUT"))
        {
            return NULL;
        }
        if (!strcmp(lib_name, "__input_l"))
        {
            return NULL;
        }
        if (!strcmp(lib_name, "__input_s_l"))
        {
            return NULL;
        }
        if (!strcmp(lib_name, "_abort"))
        {
            return NULL;
        }
        //__call_reportfault
        if (!strcmp(lib_name, "__call_reportfault"))
        {
            return NULL;
        }
    }
#endif
#if 0
    if (lib_name)
    {
        if (strcmp(lib_name, "___report_gsfailure") && strcmp(lib_name, "__invoke_watson") && strcmp(lib_name, "___strgtold12_l") && strcmp(lib_name, "_$I10_OUTPUT") && strcmp(lib_name, "__input_l") && strcmp(lib_name, "__input_s_l") && strcmp(lib_name, "_abort") && strcmp(lib_name, "__call_reportfault"))
        {
            return NULL;
        }
    }
#endif


    DISASM MyDisasm;
    memset(&MyDisasm, 0, sizeof(DISASM));
    MyDisasm.EIP = (UIntPtr)bin;
    int len = length;

    ControlFlowGraph* cfg = new ControlFlowGraph;

    vector<DISASM> instructions;
    vector<int> instruction_lens;

    while (len > 0)
    {
        int len2 = Disasm(&MyDisasm);
        if (len2 == UNKNOWN_OPCODE)
        {
            break;
        }
        if (MyDisasm.Instruction.Opcode == 0xCC)
        {
            break;
        }
        int ID = cfg->instructions.size();

        instructions.push_back(MyDisasm);
        instruction_lens.push_back(len2);
        len -= len2;

        MyDisasm.EIP = MyDisasm.EIP + (UIntPtr)len2;
    }

    if (limit_size && instructions.size() < MIN_INS_LENGTH)
    {
        delete cfg;
        return NULL;
    }

    // walk instructions
    int LENGTH = instructions.size();
    CBitSet visited(LENGTH);
    stack<int> work_list;
    work_list.push(0);
    while (!work_list.empty())
    {
        int todo = work_list.top();
        work_list.pop();
        if (todo >= LENGTH ||  visited.isset(todo))
        {
            continue;
        }
        visited.set(todo);

        // next
        DISASM& disam = instructions[todo];

        switch (disam.Instruction.BranchType)
        {
        case 0: // normal instruction?
        case CallType:
        {
            work_list.push(todo + 1); // next instruction
        }
        break;
        case RetType: // ret? stop searching.
            break;
        default: // jxx
        {
            if (disam.Argument1.ArgType & CONSTANT_TYPE)
            {
                for (int i = 0; i < instructions.size(); i++)
                {
                    if (instructions[i].EIP == disam.Instruction.AddrValue)
                    {
                        work_list.push(i);
                    }
                }
            }

            if (disam.Instruction.BranchType != JmpType)
            {
                work_list.push(todo + 1);
            }
        }

        break;
        }
    }

    DWORD instruction_len = 0;
    for (int i = 0; i < instructions.size(); i++)
    {
        if (visited.isset(i))
        {
            Instruction ins;
            memcpy(&ins.diasm, &instructions[i], sizeof(DISASM));
            ins.ID = cfg->instructions.size();
            ins.byte_length = instruction_lens[i];
            // numbering instruction
            ins.generalize();

            // db_id
            ins.db_id = BKDRHash(instructions[i].Instruction.Mnemonic) | ins.generalize_result << 16;
            ins.parse();

            int cat = instructions[i].Instruction.Category & 0xFF;
            bool changed = false;
            switch (cat)
            {
            case DATA_TRANSFER:
                cfg->MOV_COUNT++;
                break;
            case ARITHMETIC_INSTRUCTION:
                cfg->ARITHMETIC_COUNT++;
                break;
            case LOGICAL_INSTRUCTION:
                cfg->LOGI_COUNT++;
                break;
            case CONTROL_TRANSFER:
                cfg->CTI_COUNT++;
                break;
            case STRING_INSTRUCTION:
                cfg->STRING_COUNT++;
                break;
            default:
                cfg->ETC_COUNT++;
                break;
            }

            cfg->instructions.push_back(ins);
            instruction_len += instruction_lens[i];
        }
    }

    // insert into database
    if (for_lib)
    {
        cfg->build(for_lib);
        LibM lib;
        lib.block_size = cfg->bb_len;
        lib.cfg = (int4)cfg;
        lib.is_inline_function = is_inlined_function;
        lib.lib_name = lib_name;
        lib.instruction_size = cfg->instructions.size();
        lib.MOV_COUNT = cfg->MOV_COUNT;
        lib.CTI_COUNT = cfg->CTI_COUNT;
        lib.ARITHMETIC_COUNT = cfg->ARITHMETIC_COUNT;
        lib.LOGI_COUNT = cfg->LOGI_COUNT;
        lib.STRING_COUNT = cfg->STRING_COUNT;
        lib.ETC_COUNT = cfg->ETC_COUNT;
        insert(lib);

        lib_count++;
#if 0
        bool f1 = false;
        bool f2 = false;
        if (cfg->bb_len == 3 &&
                cfg->instructions.size() < 40)
        {
            cfg->buildDepGraph(false);
            cfg->serialize();
            for each(Instruction ins in cfg->instructions)
            {
                if (ins.db_id == -1)
                {
                    f1 = true;
                }
                /*if (ins.db_id == -2){
                	f2 = true;
                }*/
            }

            for (size_t i = 0; i < cfg->bb_len; i++)
            {
                if (cfg->basic_blocks[i].has_uncertain_vertexSeq)
                {
                    f2 = true;
                    break;
                }
            }

            if (//f1
                //&&
                f2
            )
            {
                printf("%s\n", lib_name);
                cfg->dep_graph();
                cfg->dot_vfgraphForFull();
                char c;
                cin.read(&c, 1);

                //ExitProcess(0);
            }
        }
#endif
        //cfg->bb_graph();
        //cfg->buildDepGraph(false);
        //cfg->dep_graph();
        //ExitProcess(0);
    }
    return cfg;
}

void build_instruction_db()
{
    if (!bFull)
    {
        disasm(lib_m, sizeof(lib_m), true, "strlen");
        disasm(lib_m2, sizeof(lib_m2), true, "strcpy");
        disasm(lib_m3, sizeof(lib_m3), true, "div");
        disasm(lib_m4, sizeof(lib_m4), true, "_pos_end");
        disasm(lib_m5, sizeof(lib_m5), true, "memcpy");
        disasm(lib_m6, sizeof(lib_m6), true, "strcat");
        disasm(lib_m7, sizeof(lib_m7), true, "strcmp");
        disasm(lib_m8, sizeof(lib_m8), true, "abs");
        disasm(lib_m9, sizeof(lib_m9), true, "memcmp");
        disasm(lib_m10, sizeof(lib_m10), true, "strlen2");
        disasm(lib_m11, sizeof(lib_m11), true, "strset");
        disasm(lib_m12, sizeof(lib_m12), true, "wcscat");
        //disasm(lib_m13, sizeof(lib_m13), true, "eax?-1:0");
    }
#if 1
    if (bFull)
    {
        limit_size = true;
        TCHAR path1[MAX_PATH];
        wsprintf(path1, "%s\\libcmt.lib", lib_root_path);
        build_from_coff(path1);
        wsprintf(path1, "%s\\libcpmt.lib", lib_root_path);
        build_from_coff(path1);
    }
#endif
}

bool my_visitor(int n, node_id ni1[], node_id ni2[], void* usr_data)
{
    //    int i;
    int* data = (int*)usr_data;
    Graph* m = (Graph*)data[0];
    Graph* g = (Graph*)data[1];
    PCBitSet lib_info = (PCBitSet)data[2];

    //bool is_full = g->NodeCount() == n;
    extern bool bFull;
    // find
    if (lib_info->isset(ni2[0])) {}
    else
    {
        bool passed = true;
        extern long found_c;
        InterlockedExchangeAdd(&found_c, 1);
        lib_info->set(ni2[0]);
    }

    return bFull;
}

bool match(Graph* graph_m, Graph* graph_g, PCBitSet lib_info)
{
    VF2SubState s0(graph_m, graph_g);
    int data[3];
    data[0] = (int)graph_m;
    data[1] = (int)graph_g;
    data[2] = (int)lib_info;

    Match m(&s0, my_visitor, &data);
    if (bFull)
    {
        m.match_par();
    }
    else
    {
        m.match_par2();
    }
    return m.foundFlg;
}


//
// match BBSF, if true: skip, false: not matched
//
bool matchBBSF(ControlFlowGraph* g, ControlFlowGraph* m)
{
    if (m->bb_len == 1)
    {
        return true;
    }

    g->build_bb_sorts();

    m->build_bb_sorts();

    // match BBSF
    for (int i = 0; i < m->bb_len; i++)
    {
        BasicBlock* g_m = &m->basic_blocks[m->bb_sorts[i].index];

        // head block?
        if (m->head_blocks.find(g_m) != m->head_blocks.end())
        {
            continue;
        }

        // tail block?
        if (m->tail_blocks.find(g_m) != m->tail_blocks.end())
        {
            continue;
        }

        EnterCriticalSection(&m->cs);
        g_m->serialize();
        LeaveCriticalSection(&m->cs);

        bool next = false;
        int s2 = (m->bb_sorts[i]).size;

        for (int j = 0; (j < g->bb_len) && ((g->bb_sorts[j]).size <= s2); j++)
        {
            if ((g->bb_sorts[j]).size == s2)
            {
                // compare ID sequences
                BasicBlock* g_b = &g->basic_blocks[g->bb_sorts[j].index];

                bool passed = true;
                g_b->serialize(); // Do local EDG serialization when needed.

                // IDS
                if (passed)
                {
                    for (int k = 0; k < s2; k++)
                    {
                        if (g_b->vertexSeq[k]->ID != g_m->vertexSeq[k]->ID)
                        {
                            passed = false;
                            break;
                        }
                    }
                }
                if (passed)
                {
                    next = true;
                    break;
                }
            }
        }

        if (!next)
        {
            return false;
        }
    }

    return true;
}

//
// return a target function
//
long current_m = 0;
bool GetM2(int* job)
{
    CLock lock(cs);

    if (current_m < total_function_len)
    {
        job[0] = current_m;
        current_m += 10;
        if (current_m >= total_function_len)
        {
            current_m = total_function_len;
        }

        job[1] = current_m;
        return true;
    }
    return false;
}

long GetM()
{
    CLock lock(cs);
    if (current_m < total_function_len)
    {
        int ret = current_m;
        current_m++;
        return ret;
    }
    return -1;
}

unsigned __stdcall TestThread(void* pParam)
{
    TEST_CONTEXT* tc = static_cast<TEST_CONTEXT*>(pParam);
    tc->result = match(tc->m, tc->g, tc->lib_info);

    return 0;
}

//
// sort function
//
bool myfunction(function_info& i, function_info& j)
{
    return (i.len < j.len);
}

bool validate_result(ControlFlowGraph* GT, int n, node_id ni2[])
{
    for (size_t i = 0; i < GT->instructions.size(); i++)
    {
        bool isExternalVertex = true;
        for (size_t j = 0; j < n; j++)
        {
            if (ni2[j] == i)
            {
                isExternalVertex = false;
                break;
            }
        }

        if (!isExternalVertex)
        {
            continue;
        }

        BasicBlock* bb0 = static_cast<BasicBlock*>(GT->instructions[ni2[0]].basic_block);
        BasicBlock* bb = static_cast<BasicBlock*>(GT->instructions[i].basic_block);

        if (GT->instructions[i].diasm.EIP < GT->instructions[ni2[0]].diasm.EIP || GT->instructions[i].diasm.EIP > GT->instructions[ni2[n - 1]].diasm.EIP) // out of C_S?
        {
            // check whether they in the same block
            if (bb == bb0)
            {
                continue;
            }

            // check whether n_i is a tail vertex
            if (bb->node[i - bb->start].successors.size() > 0)
            {
                // just consider the tail vertices
                continue;
            }

            for each(BasicBlock * suc in bb->succeccors)
            {
                if (suc != bb0)
                {
                    // check whether suc belongs to S
                    for (size_t j = 1; j < n; j++)
                    {
                        if (ni2[j] == suc->start)
                        {
                            return false;
                        }
                    }
                }
            }
        }
        else
        {
            bool hasPredecessors = false, hasSuccessors = false;
            for each(BasicBlock * pre in bb->predecessors)
            {
                for (size_t j = 1; j < n; j++)
                {
                    if (ni2[j] == pre->start)
                    {
                        hasPredecessors = true;
                        break;
                    }
                }
                if (hasPredecessors)
                {
                    break;
                }
            }

            for each(BasicBlock * suc in bb->succeccors)
            {
                for (size_t j = 1; j < n; j++)
                {
                    if (ni2[j] == suc->start)
                    {
                        hasSuccessors = true;
                        break;
                    }
                }
                if (hasSuccessors)
                {
                    break;
                }
            }

            if (hasPredecessors && hasSuccessors)
            {
                return false;
            }

            for (size_t j = 0; j < n; j++)
            {
                if (ni2[j] >= bb->start && ni2[j] <= bb->end)
                {
                    if (bb->hasDependence(ni2[j] - bb->start, i - bb->start))
                    {
                        return false;
                    }
                }
            }

            if (i == bb->end && GT->instructions[i].IsCTI())
            {
                for each(BasicBlock * suc in bb->succeccors)
                {
                    // check whether suc belongs to S
                    for (size_t j = 1; j < n; j++)
                    {
                        if (ni2[j] == suc->start)
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}