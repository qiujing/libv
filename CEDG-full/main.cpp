// general library function identification

#include "../common/shared.h"
#include "../common/InstructionComparator.h"
#include "../common/InstructionComparator2.h"
#include "../common/InstructionComparator3.h"

bool lib_mode = false; // diasm library functions?
unsigned int lib_count = 0;
bool bFull = false;
long found_c = 0;
long check_failed = 0;

dbDatabase db;
REGISTER_IN(LibM, &db);

// target file
MAP_FILE_STRUCT stMapFile;
PIMAGE_OPTIONAL_HEADER pOH;
PIMAGE_NT_HEADERS pImageNtH;

bool limit_size = false; //limit the lib size?

char* lib_root_path; // library root path
bool bSerialize = false;
bool bMultiThread = false;

vector<function_info> f_info;

int total_function_len = 0;

unsigned int db_id_count = 0;

CRITICAL_SECTION cs;

// thread number
#ifdef _DEBUG
#define THREAD_NUM (1)
#else
#define THREAD_NUM (8)
#endif

DWORD instruction_count[THREAD_NUM]; //instruction number
DWORD r1[THREAD_NUM]; // LR
DWORD r34[THREAD_NUM]; // BBNR and BBMLR
DWORD r2[THREAD_NUM]; // ITR
DWORD r5[THREAD_NUM]; // BBSR
DWORD r6[THREAD_NUM]; // HTR
DWORD r0[THREAD_NUM]; // TRY
DWORD fn[THREAD_NUM]; // function number
long fulled = 0;

double contract_rate[THREAD_NUM];

//#define FORCE_INLINE


void clean(ControlFlowGraph* target_cfg)
{
    if (target_cfg != NULL)
    {
        delete target_cfg;
    }

    //db.detach();
}


bool my_visitor2(int n, node_id ni1[], node_id ni2[], void *usr_data)
{
    int *d = (int *)usr_data;
    ControlFlowGraph * cfg = (ControlFlowGraph*)d[0];
    PCBitSet lib_info = (PCBitSet)d[3];
    int startEA = d[1];

    if (!lib_info->isset(ni2[0]))
    {
        extern long found_c;
        InterlockedExchangeAdd(&found_c, 1);
        ///printf("1\t0\t%08X\t%s\n", cfg->instructions[ni2[0]].diasm.EIP - cfg->instructions[0].diasm.EIP + startEA, (char*)d[2]);
        printf("1\t0\t%s\n", (char*)d[2]);

        //if (show_dump)
        //{
        //	//int offset = startEA - cfg->instructions[0].diasm.EIP;
        //	//BasicBlock* bb =
        //	for (int i = 0; i < n; i++)
        //	{
        //		printf("%08X  %s\n", cfg->instructions[ni2[i]].diasm.EIP, cfg->instructions[ni2[i]].diasm.CompleteInstr);
        //	}
        //	bool f = validate_result(cfg, n, ni2);
        //	printf(f ? "OK\n" : "Failed\n");
        //	printf("\n\n");
        //}
        lib_info->set(ni2[0]);
    }

    return false;// is_full;

}
//
// process thread
//
unsigned __stdcall MatchThread(void* pParam)
{
    int FLEN;
    int n = 0, n1, i;
    dbQuery sql;
    ControlFlowGraph* target_cfg;

    dbCursor<LibM> cursor1;
    int thread_id = (int)pParam;
    instruction_count[thread_id] = 0;
    fn[thread_id] = 0;
    contract_rate[thread_id] = 0;
    db.attach();


    while ((i = GetM()) != -1)
    {
        int startEA = f_info[i].startEA;
        FLEN = f_info[i].len;

        // disasm
        byte* bin = (byte*)RvaToPtr(pImageNtH, stMapFile.ImageBase, startEA - pOH->ImageBase);
        if (bin == NULL)
        {
            continue;
        }

        target_cfg = (ControlFlowGraph*)disasm(bin, FLEN, false, NULL);

        if (target_cfg == NULL)
        {
            continue;
        }

        fn[thread_id]++;

        // rule1: LR
        sql = "MOV_COUNT<=", target_cfg->MOV_COUNT, " and CTI_COUNT<=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT<=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT<=", target_cfg->LOGI_COUNT, " and STRING_COUNT<=", target_cfg->STRING_COUNT, " and ETC_COUNT<=", target_cfg->ETC_COUNT;

        n = cursor1.select(sql);
        if (n == 0)
        {
            r1[thread_id] += lib_count;
            clean(target_cfg);
            continue;
        }

        r1[thread_id] += lib_count - n;

        instruction_count[thread_id] += target_cfg->instructions.size();
        target_cfg->build();

        // rule3: BBNR
        // rule4: BBMLR
        sql = "MOV_COUNT<=", target_cfg->MOV_COUNT, " and CTI_COUNT<=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT<=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT<=", target_cfg->LOGI_COUNT, " and STRING_COUNT<=", target_cfg->STRING_COUNT, " and ETC_COUNT<=", target_cfg->ETC_COUNT, " and instruction_size<=", target_cfg->instructions.size(), "and block_size<=", target_cfg->bb_len, "order by instruction_size desc";


        n1 = cursor1.select(sql);
        if (n1 == 0)
        {
            r34[thread_id] += n;
            clean(target_cfg);
            continue;
        }
        r34[thread_id] += (n - n1);

        CBitSet lib_info(target_cfg->instructions.size());
        do
        {
            ControlFlowGraph* library_cfg = (ControlFlowGraph*)(cursor1->cfg);
            //cout <<"try: "<< cursor1->lib_name << endl;
#if 0
            // BBLR
            bitset<10240> t = target_cfg->bblen_set;
            t.flip();
            t &= library_cfg->bblen_set;
            if (t.any())
            {
                r2[thread_id]++;
                return;
            }
#endif
            target_cfg->buildDepGraph(false);
            library_cfg->buildDepGraph(true);
#if 0
            // rule5: BBSF
            if (!matchBBSF(target_cfg, library_cfg))
            {
                r5[thread_id]++;
                return;
            }
#endif
            target_cfg->serialize();
            library_cfg->serialize();

            ARGEdit* target_ed = new ARGEdit;
            vector<VFNODE*> library_vf_graph;
            vector<VFNODE*> target_vf_graph;

            int status = target_cfg->buildCEDG(target_ed, library_cfg);
            if (status != 0)
            {
                r6[thread_id]++;
                delete target_ed;
                clean(target_cfg);
                return;
            }

            library_cfg->buildCEDG(&library_cfg->vlibARGEdit, library_cfg);

            // statatics
            target_cfg->get_edges_count();

            size_t EDG_n = target_cfg->instructions.size();
            size_t EDG_e = target_cfg->edge_count;
            size_t CEDG_n = target_cfg->vertices_CEDG_count;
            size_t CEDG_e = target_cfg->edge_CEDG_count;
            contract_rate[thread_id] += (double)(CEDG_n + CEDG_e) / (EDG_n + EDG_e);

            printf("%08X %s %d %d %d %d\n", startEA, cursor1->lib_name, CEDG_n, CEDG_e, EDG_n, EDG_e);


            r0[thread_id]++;
            Graph _g(target_ed);
            Graph _m(&library_cfg->vlibARGEdit);
            _m.SetNodeComparator(new InstructionComparator2);

            VF2SubState s0(&_m, &_g);
            int d[4];
            d[0] = (int)target_cfg;
            d[1] = startEA;
            d[2] = (int)cursor1->lib_name;
            d[3] = (int)&lib_info;

            Match m(&s0, my_visitor2, &d);
            m.match_par2();

            // clean up
            delete target_ed;
        }
        while (cursor1.next());

        clean(target_cfg);
    }
    db.detach();
    return 0;
}

//dbQuery q1;
//dbQuery q2;
//
//struct QueryParams
//{
//    int MOV_COUNT;
//    int CTI_COUNT;
//    int ARITHMETIC_COUNT;
//    int LOGI_COUNT;
//    int STRING_COUNT;
//    int ETC_COUNT;
//    int instruction_size;
//    int bb_len;
//};
//
//void open()
//{
//    QueryParams* params = (QueryParams*)NULL;
//    q1 = "MOV_COUNT=", params->MOV_COUNT, " and CTI_COUNT=", params->CTI_COUNT, " and ARITHMETIC_COUNT=", params->ARITHMETIC_COUNT, " and LOGI_COUNT=", params->LOGI_COUNT, " and STRING_COUNT=", params->STRING_COUNT, " and ETC_COUNT=", params->ETC_COUNT;
//
//    q2 = "MOV_COUNT=", params->MOV_COUNT, " and CTI_COUNT=", params->CTI_COUNT, " and ARITHMETIC_COUNT=", params->ARITHMETIC_COUNT, " and LOGI_COUNT=", params->LOGI_COUNT, " and STRING_COUNT=", params->STRING_COUNT, " and ETC_COUNT=", params->ETC_COUNT, " and instruction_size=", params->instruction_size, "and block_size=", params->bb_len, "order by instruction_size desc";
//}



unsigned __stdcall MatchThreadForFull(void* pParam)
{
    int FLEN;
    int n = 0, n1, i;
    dbQuery sql;
    ControlFlowGraph* target_cfg = NULL;

    db.attach();

    int thread_id = (int)pParam;
    instruction_count[thread_id] = 0;
    fn[thread_id] = 0;

    contract_rate[thread_id] = 0;
    int job[2];

    int loop_count = total_function_len / THREAD_NUM;
    if (total_function_len % THREAD_NUM > 0)
    {
        loop_count++;
    }

    while ((i = GetM()) != -1)
    {
        //for (i = job[0]; i < job[1]; i++)
        //for (size_t kk = 0; kk < loop_count; kk++)
        //for (i = 0; i < loop_count; i++)
        {
            /*int kk = i * THREAD_NUM + thread_id;
            if (kk >= total_function_len)
            {
                break;
            }*/
            int startEA = f_info[i].startEA;
            FLEN = f_info[i].len;

            // disasm
            byte* bin = (byte*)RvaToPtr(pImageNtH, stMapFile.ImageBase, startEA - pOH->ImageBase);
            if (bin == NULL)
            {
                continue;
            }

            target_cfg = (ControlFlowGraph*)disasm(bin, FLEN, false, NULL);

            if (target_cfg == NULL || target_cfg->instructions.size() < MIN_INS_LENGTH)
            {
                clean(target_cfg);
                continue;
            }

            fn[thread_id]++;

            /*QueryParams params;
            params.MOV_COUNT = target_cfg->MOV_COUNT;
            params.CTI_COUNT = target_cfg->CTI_COUNT;
            params.ARITHMETIC_COUNT = target_cfg->ARITHMETIC_COUNT;
            params.LOGI_COUNT = target_cfg->LOGI_COUNT;
            params.STRING_COUNT = target_cfg->STRING_COUNT;
            params.ETC_COUNT = target_cfg->ETC_COUNT;
            params.instruction_size = target_cfg->instructions.size();
            params.bb_len = target_cfg->bb_len;*/

            dbCursor<LibM> cursor1;

            // rule1: LR
            sql = "MOV_COUNT=", target_cfg->MOV_COUNT, " and CTI_COUNT=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT=", target_cfg->LOGI_COUNT, " and STRING_COUNT=", target_cfg->STRING_COUNT, " and ETC_COUNT=", target_cfg->ETC_COUNT;

            n = cursor1.select(sql);
            if (n == 0)
            {
                r1[thread_id] += lib_count;
                clean(target_cfg);
                continue;
            }

            r1[thread_id] += lib_count - n;

            instruction_count[thread_id] += target_cfg->instructions.size();
            target_cfg->build();

            // rule3: BBNR
            // rule4: BBMLR
            sql = "MOV_COUNT=", target_cfg->MOV_COUNT, " and CTI_COUNT=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT=", target_cfg->LOGI_COUNT, " and STRING_COUNT=", target_cfg->STRING_COUNT, " and ETC_COUNT=", target_cfg->ETC_COUNT, " and instruction_size=", target_cfg->instructions.size(), "and block_size=", target_cfg->bb_len, "order by instruction_size desc";

            n1 = cursor1.select(sql);

            if (n1 == 0)
            {
                r34[thread_id] += n;
                clean(target_cfg);
                continue;
            }
            r34[thread_id] += (n - n1);

            int ins_size = target_cfg->instructions.size();

            CBitSet lib_info(target_cfg->instructions.size());
            bool _found = false;

            do
            {
                ControlFlowGraph* library_cfg = (ControlFlowGraph*)(cursor1->cfg);

                if (library_cfg->bb_len == 1)
                {
                    target_cfg->buildDepGraph(false);
                    library_cfg->buildDepGraph(true);
                    target_cfg->serialize();
                    library_cfg->serialize();
                    r0[thread_id]++;
#if 0
                    target_cfg->bb_graph();
                    target_cfg->dot_vfgraphForFull();
                    target_cfg->dep_graph();
                    library_cfg->bb_graph();
                    library_cfg->dot_vfgraphForFull();
                    library_cfg->dep_graph();
#endif
                    if (library_cfg->basic_blocks[0].has_uncertain_vertexSeq ||
                            target_cfg->basic_blocks[0].has_uncertain_vertexSeq)
                    {
                        library_cfg->buildVLibGraph();
                        target_cfg->buildVLibGraph();

                        r0[thread_id]++;
                        Graph _g(&target_cfg->vlibARGEdit);
                        Graph _m(&library_cfg->vlibARGEdit);
                        _m.SetNodeComparator(new InstructionComparator3);

                        TEST_CONTEXT tc;
                        tc.g = &_g;
                        tc.m = &_m;
                        tc.lib_info = &lib_info;
                        tc.result = false;

                        //HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, &TestThread, &tc, 0, NULL);
                        //DWORD ret = WaitForSingleObject(thread, INFINITE);//TIME_OUT);
                        /*if (ret == WAIT_TIMEOUT)
                        {
                            tc.result = true;
                            TerminateThread(thread, 0);
                            WaitForSingleObject(thread, 1000);
                            printf("Time out. graph size:%d\n", target_cfg->instructions.size());
                        }*/

                        tc.result = match(tc.m, tc.g, tc.lib_info);

                        if (tc.result)
                        {
                            printf("0\t%X\t%s\n", startEA, cursor1->lib_name);
                        }
                    }
                    else
                    {
                        if (library_cfg->basic_blocks[0].equal(&target_cfg->basic_blocks[0]))
                        {
                            printf("1\t%X\t%s\n", startEA, cursor1->lib_name);
                            InterlockedExchangeAdd(&fulled, 1);
                            if (!_found)
                            {
                                InterlockedExchangeAdd(&found_c, 1);
                                _found = true;
                            }
                        }
                    }
                }
                else
                {
#if 1
                    // BBLR
                    bitset<10240> t = target_cfg->bblen_set;
                    t.flip();
                    t &= library_cfg->bblen_set;
                    if (t.any())
                    {
                        r2[thread_id]++;
                        continue;
                    }
#endif
                    target_cfg->buildDepGraph(false);
                    library_cfg->buildDepGraph(true);
#if 1
                    if (bSerialize)
                    {
                        // rule5: BBSF
                        if (!matchBBSF(target_cfg, library_cfg))
                        {
                            r5[thread_id]++;
                            continue;
                        }
                    }
#endif
                    target_cfg->serialize();
                    library_cfg->serialize();
                    target_cfg->buildVLibGraphForFullIdentification();
                    library_cfg->buildVLibGraphForFullIdentification();
#if 0
                    //library_cfg->bb_graph();
                    target_cfg->dot_vfgraphForFull();
                    //target_cfg->dep_graph();
                    library_cfg->dot_vfgraphForFull();
                    //library_cfg->dep_graph();
#endif
                    target_cfg->get_edges_count();
                    size_t EDG_n = target_cfg->instructions.size();
                    size_t EDG_e = target_cfg->edge_count;
                    size_t CEDG_n = target_cfg->vertices_CEDG_count;
                    size_t CEDG_e = target_cfg->edge_CEDG_count;
                    contract_rate[thread_id] += (double)(CEDG_n + CEDG_e) / (EDG_n + EDG_e);

                    //printf("%d time out. BB:%d Instr:%d Vertices:%d Edge:%d CEDG_n:%d CEDG_e:%d \n", thread_id, //target_cfg->bb_len, target_cfg->instructions.size(), EDG_n, EDG_e, CEDG_n, CEDG_e);

                    r0[thread_id]++;
                    Graph _g(&target_cfg->vlibARGEdit);
                    Graph _m(&library_cfg->vlibARGEdit);
                    _m.SetNodeComparator(new InstructionComparator2);

                    TEST_CONTEXT tc;
                    tc.g = &_g;
                    tc.m = &_m;
                    tc.lib_info = &lib_info;
                    tc.result = false;

                    //            HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, &TestThread, &tc, 0, NULL);
                    //            DWORD ret = WaitForSingleObject(thread,
                    //#ifndef _DEBUG
                    //                                            INFINITE
                    //                                            //10 * 1000
                    //#else
                    //                                            TIME_OUT
                    //#endif
                    //                                           );
                    tc.result = match(tc.m, tc.g, tc.lib_info);

                    /*if (ret == WAIT_TIMEOUT)
                    {
                    tc.result = true;
                    TerminateThread(thread, 0);
                    WaitForSingleObject(thread, 1000);
                    printf("Time out. BB:%d Instr:%d Vertices:%d Edge:%d CEDG_n:%d CEDG_e:%d \n", target_cfg->bb_len, target_cfg->instructions.size(), EDG_n, EDG_e, CEDG_n, CEDG_e);
                    }*/

                    if (tc.result)
                    {
                        printf("%d\t1\t%X\t%s\n", thread_id, startEA, cursor1->lib_name);
                        //InterlockedExchangeAdd(&fulled, 1);
                    }
                }
            }
            while (cursor1.next());

            clean(target_cfg);
        }
    }
    db.detach();
    printf("#%d done.\n", thread_id);
    return 0;
}

//void work(int i)
//{
//    db.attach();
//    if (bFull)
//    {
//        MatchThreadForFull(i);
//    }
//    else
//    {
//        MatchThread(i);
//    }
//    db.detach();
//}
//
// main
//
int main(int argc, char** argv)
{
    dbCursor<LibM> cursor1;

    if (argc != 4)
    {
        printf("Usage: %s target lib_root_path full:0|1\n", argv[0]);
        return 0;
    }

    lib_root_path = argv[2];
    bFull = argv[3][0] == '1';

    //if (!bFull)
    //{
    //    return 0;
    //}

    bMultiThread = true;
    bSerialize = true;

    TCHAR* filename = argv[1];

    TCHAR filename2[MAX_PATH];
    int i;
    DeleteFile("LibM.fdb");
    if (!db.open("LibM"))
    {
        return 0;
    }
    printf("Built on " __DATE__ " " __TIME__ "\nInit db...");
    build_instruction_db();

    printf("total:%d OK\n", lib_count);

    //getch();
    InitializeCriticalSection(&cs);


    printf("Loading...\n");
    if (!LoadFileR(filename, &stMapFile))
    {
        cerr << "ERROR FILE!" << endl;
        //continue;
        goto ex;
    }

    if (!IsPEFile(stMapFile.ImageBase))
    {
        cerr << "NOT PE FILE!" << endl;
        UnLoadFile(&stMapFile);
        //continue;
        goto ex;
    }

    pOH = GetOptionalHeader(stMapFile.ImageBase);
    pImageNtH = GetNtHeaders(stMapFile.ImageBase);

    int c0 = 0, c1 = 0, c2 = 0, c3 = 0, c4 = 0, count = 0;

    // read target function info
    wsprintf(filename2, _T("%s.txt"), filename);
    ifstream is2(filename2, ios::binary);
    if (!is2.is_open())
    {
        goto ex;
    }
    int tt;

    db.detach();
    f_info.clear();
    while (!is2.eof())
    {
        int startEA, endEA;
        is2.read((char*)&startEA, sizeof(int));
        is2.read((char*)&endEA, sizeof(int));

        function_info fi;
        memset(&fi, 0, sizeof(fi));
        fi.startEA = startEA;
        fi.len = endEA - startEA;
        fi.lib_name[0] = 0;
        f_info.push_back(fi);
    }
    total_function_len = f_info.size();

#ifndef _DEBUG
    sort(f_info.begin(), f_info.end(), myfunction);
#endif

    printf("OK\n");

    tt = GetTickCount();

    int thread_num = bMultiThread ? THREAD_NUM : 1;

    HANDLE *thread = new HANDLE[thread_num];
    for (i = 0; i < thread_num; i++)
    {
        thread[i] = (HANDLE)_beginthreadex(NULL, 0, bFull ? (&MatchThreadForFull) : (&MatchThread), (void *)i, 0, NULL);
    }

    WaitForMultipleObjects(thread_num, thread, TRUE, INFINITE);

    tt = GetTickCount() - tt;
    delete[] thread;
    db.attach();

    UnLoadFile(&stMapFile);

    if (cursor1.select() > 0)
    {
        do
        {
            delete(ControlFlowGraph*)cursor1->cfg;
        }
        while (cursor1.next());
    }

    DWORD s = 0;
    DWORD _r0 = 0, _r1 = 0, _r34 = 0, _r2 = 0, _r5 = 0, _r6 = 0, _fn = 0;
    DWORD _edge_count = 0, _edge_CEDG_count = 0;
    double _contract_rate = 0;

    for (i = 0; i < THREAD_NUM; i++)
    {
        s += instruction_count[i];
        _r0 += r0[i];
        _r1 += r1[i];
        _r2 += r2[i];
        _r34 += r34[i];
        _r5 += r5[i];
        _r6 += r6[i];
        _fn += fn[i];
        _contract_rate += contract_rate[i];
    }

    _contract_rate = 1 - _contract_rate / _r0;

    printf("contract_rate:\t%f\n", _contract_rate);
    printf("#fn:%d #instr:%d t:%d found:%d\n", _fn, s, tt, found_c);
    printf("r1:\t%d\n", _r1);
    printf("r2:\t%d\n", _r2);
    printf("r34:\t%d\n", _r34);
    printf("r5:\t%d\n", _r5);
    printf("r6:\t%d\n", _r6);
    printf("r0:\t%d\n", _r0);
    printf("fulled:\t%d\n", fulled);
    printf("check failed:\t%d\n", check_failed);
    printf("END.\n");

ex:
    DeleteCriticalSection(&cs);

    DeleteFile("LibM.fdb");

#ifdef _DEBUG
    char c;
    cin.read(&c, 1);
#endif

    return 0;
}
