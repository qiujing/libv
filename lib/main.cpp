// library function identification

#include "../common/shared.h"
#include "../common/InstructionComparator3.h"

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


DWORD instruction_count; //instruction number
DWORD r1; // LR
DWORD r34; // BBNR and BBMLR
DWORD r2; // ITR
DWORD r5; // BBSR
DWORD r0; // TRY
DWORD fn; // function number
long fulled = 0;

#define MULTI_THREAD

//#define FORCE_INLINE
//#define NO_TIME_LIMIT

bool show_dump;

bool my_visitor2(int n, node_id ni1[], node_id ni2[], void* usr_data)
{
    int* d = (int*)usr_data;
    ControlFlowGraph* cfg = (ControlFlowGraph*)d[0];
    PCBitSet lib_info = (PCBitSet)d[3];
    int startEA = d[1];

    if (!lib_info->isset(ni2[0]))
    {
        extern long found_c;
        InterlockedExchangeAdd(&found_c, 1);
        printf("1\t0\t%08X\t%s\n", cfg->instructions[ni2[0]].diasm.EIP - cfg->instructions[0].diasm.EIP + startEA, (char*)d[2]);

        if (show_dump)
        {
            for (int i = 0; i < n; i++)
            {
                printf("%08X  %s\n", cfg->instructions[ni2[i]].diasm.EIP, cfg->instructions[ni2[i]].diasm.CompleteInstr);
            }
            bool f = validate_result(cfg, n, ni2);
            printf(f ? "OK\n" : "Failed\n");
            printf("\n\n");
        }
        lib_info->set(ni2[0]);
    }

    return false;// is_full;
}


void clean(ControlFlowGraph* target_cfg)
{
    if (target_cfg != NULL)
    {
        delete target_cfg;
    }

    db.detach();
}

//
// process thread
//
void MatchThread(int i)
{
    int FLEN;
    int n = 0, n1;
    dbQuery sql;
    ControlFlowGraph* target_cfg = NULL;

    db.attach();

    dbCursor<LibM> cursor1;

    int startEA = f_info[i].startEA;
    FLEN = f_info[i].len;

    // disasm
    byte* bin = (byte*)RvaToPtr(pImageNtH, stMapFile.ImageBase, startEA - pOH->ImageBase);
    if (bin == NULL)
    {
        clean(target_cfg);
        return;
    }

    target_cfg = (ControlFlowGraph*)disasm(bin, FLEN, false, NULL);

    if (target_cfg == NULL || target_cfg->instructions.size() < MIN_INS_LENGTH)
    {
        clean(target_cfg);
        return;
    }

    fn++;

    instruction_count += target_cfg->instructions.size();
    target_cfg->build();

    if (!bFull)
    {
        sql = "MOV_COUNT<=", target_cfg->MOV_COUNT, " and CTI_COUNT<=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT<=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT<=", target_cfg->LOGI_COUNT, " and STRING_COUNT<=", target_cfg->STRING_COUNT, " and ETC_COUNT<=", target_cfg->ETC_COUNT, " and instruction_size<=", target_cfg->instructions.size(), "and block_size<=", target_cfg->bb_len, "order by instruction_size desc";
    }
    else
    {
        sql = "MOV_COUNT=", target_cfg->MOV_COUNT, " and CTI_COUNT=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT=", target_cfg->LOGI_COUNT, " and STRING_COUNT=", target_cfg->STRING_COUNT, " and ETC_COUNT=", target_cfg->ETC_COUNT, " and instruction_size=", target_cfg->instructions.size(), "and block_size=", target_cfg->bb_len, "order by instruction_size desc";
    }

    n1 = cursor1.select(sql);
    if (n1 == 0)
    {
        clean(target_cfg);
        return;
    }

    CBitSet lib_info(target_cfg->instructions.size());
    do
    {
        ControlFlowGraph* library_cfg = (ControlFlowGraph*)(cursor1->cfg);

        // BBLR
        bitset<10240> t = target_cfg->bblen_set;
        t.flip();
        t &= library_cfg->bblen_set;
        if (t.any())
        {
            continue;
        }

        target_cfg->buildDepGraph(false);
        library_cfg->buildDepGraph(true);

        if (bSerialize)
        {
            // rule5: BBSR
            if (!matchBBSF(target_cfg, library_cfg))
            {
                //r5[thread_id]++;
                continue;
            }
        }

        library_cfg->serialize();
        library_cfg->buildVLibGraph();

        target_cfg->serialize();
        target_cfg->buildVLibGraph();

        //r0[thread_id]++;
        Graph _g(&target_cfg->vlibARGEdit);
        Graph _m(&library_cfg->vlibARGEdit);
        _m.SetNodeComparator(new InstructionComparator3);

        VF2SubState s0(&_m, &_g);
        int d[4];
        d[0] = (int)target_cfg;
        d[1] = startEA;
        d[2] = (int)cursor1->lib_name;
        d[3] = (int)&lib_info;
        Match m(&s0, my_visitor2, d);

        m.match_par();

    }
    while (cursor1.next());
    clean(target_cfg);
}

//
// main
//
int main(int argc, char** argv)
{
    dbCursor<LibM> cursor1;

    if (argc != 4)
    {
        printf("Usage: %s target lib_root_path full:0|1 \n", argv[0]);
        return 0;
    }

    lib_root_path = argv[2];
    bFull = argv[3][0] == '1';

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

        if (endEA - startEA < MIN_INS_LENGTH)
        {
            continue;
        }

        function_info fi;
        memset(&fi, 0, sizeof(fi));
        fi.startEA = startEA;
        fi.len = endEA - startEA;
        fi.lib_name[0] = 0;
        f_info.push_back(fi);
    }
    total_function_len = f_info.size();

    printf("OK\n");

    tt = GetTickCount();
    cilk_for(int i = 0; i < f_info.size(); i++)
    {
        MatchThread(i);
    }
    tt = GetTickCount() - tt;

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

    printf("#fn:%d #instr:%d t:%d found:%d\n", fn, instruction_count, tt, found_c);

    printf("r1:\t%d\n", r1);
    printf("r2:\t%d\n", r2);
    printf("r34:\t%d\n", r34);
    printf("r5:\t%d\n", r5);
    printf("r0:\t%d\n", r0);
    printf("fulled:\t%d\n", fulled);
    printf("check failed:\t%d\n", check_failed);
    printf("END.\n");

ex:
    DeleteCriticalSection(&cs);

    db.detach();

#ifdef _DEBUG
    cout << "Press ENTER..." << endl;
    char c;
    c = cin.get();
#endif

    return 0;
}
