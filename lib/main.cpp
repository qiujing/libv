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

char *lib_root_path; // library root path
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
#define THREAD_NUM (2)
#endif

DWORD instruction_count[THREAD_NUM]; //instruction number
DWORD r1[THREAD_NUM]; // LR
DWORD r34[THREAD_NUM]; // BBNR and BBMLR
DWORD r2[THREAD_NUM]; // ITR
DWORD r5[THREAD_NUM]; // BBSR
DWORD r0[THREAD_NUM]; // TRY
DWORD fn[THREAD_NUM]; // function number
long fulled = 0;

#define MULTI_THREAD

//#define FORCE_INLINE
//#define NO_TIME_LIMIT

bool show_dump;

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
		printf("1\t0\t%08X\t%s\n", cfg->instructions[ni2[0]].diasm.EIP - cfg->instructions[0].diasm.EIP + startEA, (char*)d[2]);

		if (show_dump)
		{
			//int offset = startEA - cfg->instructions[0].diasm.EIP;
			//BasicBlock* bb = 
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

//
// process thread
//
unsigned __stdcall MatchThread(void *pParam)
{
	int FLEN;
	int n = 0, n1, i;
	dbQuery sql;
	ControlFlowGraph *target_cfg;

	db.attach();

	dbCursor<LibM> cursor1;
	int thread_id = (int)pParam;
	instruction_count[thread_id] = 0;
	fn[thread_id] = 0;

	while ((i = GetM()) != -1)
	{
		int startEA = f_info[i].startEA;
		FLEN = f_info[i].len;

		// disasm
		byte *bin = (byte *)RvaToPtr(pImageNtH, stMapFile.ImageBase, startEA - pOH->ImageBase);
		if (bin == NULL)
		{
			continue;
		}

		target_cfg = (ControlFlowGraph *)disasm(bin, FLEN, false, NULL);

		if (target_cfg == NULL || target_cfg->instructions.size() < MIN_INS_LENGTH)
		{
			continue;
		}

		fn[thread_id]++;

		instruction_count[thread_id] += target_cfg->instructions.size();
		target_cfg->build();

		{
			sql = "MOV_COUNT<=", target_cfg->MOV_COUNT, " and CTI_COUNT<=", target_cfg->CTI_COUNT, " and ARITHMETIC_COUNT<=", target_cfg->ARITHMETIC_COUNT, " and LOGI_COUNT<=", target_cfg->LOGI_COUNT, " and STRING_COUNT<=", target_cfg->STRING_COUNT, " and ETC_COUNT<=", target_cfg->ETC_COUNT, " and instruction_size<=", target_cfg->instructions.size(), "and block_size<=", target_cfg->bb_len, "order by instruction_size desc";
		}

		n1 = cursor1.select(sql);
		if (n1 == 0)
		{
			continue;
		}

		CBitSet lib_info(target_cfg->instructions.size());
		do
		{
			ControlFlowGraph *library_cfg = (ControlFlowGraph *)(cursor1->cfg);

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
			match(&s0, my_visitor2, d);
		} while (cursor1.next());

		if (target_cfg != NULL)
		{
			delete target_cfg;
		}
	}
	db.detach();

	printf("#%d done.\n", thread_id);
	return 0;
}

//
// main
//
int main(int argc, char **argv)
{
	dbCursor<LibM> cursor1;

	if (argc < 5)
	{
	    printf("Usage: %s target lib_root_path full:0|1 multi_thread:0|1 serialize:0|1 \n", argv[0]);
	    return 0;
	}

	if (argv[3][0] == '1' ||
	    argv[4][0] == '0' || argv[5][0] == '0')
	{
	    return 0;
	}

	show_dump = (argc > 2);

	lib_root_path = argv[2];
	bFull = argv[3][0] == '1';

	bMultiThread = argv[4][0] == '1';
	bSerialize = argv[5][0] == '1';

	TCHAR *filename = argv[1];

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

	while (1)
	{
		printf("Loading...\n");
		if (!LoadFileR(filename, &stMapFile))
		{
			cerr << "ERROR FILE!" << endl;
			//continue;
			break;
		}

		if (!IsPEFile(stMapFile.ImageBase))
		{
			cerr << "NOT PE FILE!" << endl;
			UnLoadFile(&stMapFile);
			//continue;
			break;
		}

		pOH = GetOptionalHeader(stMapFile.ImageBase);
		pImageNtH = GetNtHeaders(stMapFile.ImageBase);

		int c0 = 0, c1 = 0, c2 = 0, c3 = 0, c4 = 0, count = 0;

		// read target function info
		wsprintf(filename2, _T("%s.txt"), filename);
		ifstream is2(filename2, ios::binary);
		if (!is2.is_open()) break;
		int tt;

		db.detach();
		f_info.clear();
		while (!is2.eof())
		{
			int startEA, endEA;
			is2.read((char *)&startEA, sizeof(int));
			is2.read((char *)&endEA, sizeof(int));

			if (endEA - startEA < MIN_INS_LENGTH) continue;

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
		int thread_num = bMultiThread ? THREAD_NUM : 1;
		if (show_dump) thread_num = 1;
		HANDLE *thread = new HANDLE[thread_num];
		for (i = 0; i < thread_num; i++)
		{
			thread[i] = (HANDLE)_beginthreadex(NULL, 0, &MatchThread, (void *)i, 0, NULL);
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
				delete (ControlFlowGraph *)cursor1->cfg;
			} while (cursor1.next());
		}

		DWORD s = 0;
		DWORD _r0 = 0, _r1 = 0, _r34 = 0, _r2 = 0, _r5 = 0, _fn = 0;
		for (i = 0; i < THREAD_NUM; i++)
		{
			s += instruction_count[i];
			_r0 += r0[i];
			_r1 += r1[i];
			_r2 += r2[i];
			_r34 += r34[i];
			_r5 += r5[i];
			_fn += fn[i];
		}

		printf("#fn:%d #instr:%d t:%d found:%d\n", _fn, s, tt, found_c);

		printf("r1:\t%d\n", _r1);
		printf("r2:\t%d\n", _r2);
		printf("r34:\t%d\n", _r34);
		printf("r5:\t%d\n", _r5);
		printf("r0:\t%d\n", _r0);
		printf("fulled:\t%d\n", fulled);
		printf("check failed:\t%d\n", check_failed);
		printf("END.\n");

		break;
	}

	DeleteCriticalSection(&cs);

	db.detach();

#ifdef _DEBUG
	cout << "Press ENTER..." << endl;
	char c;
	c = cin.get();
#endif

	return 0;
}
