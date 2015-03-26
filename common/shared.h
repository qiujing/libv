#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <stack>
#include <algorithm>
#include "instruction.h"
#include "utility.h"
#include "ControlFlowGraph.h"

#include "../vflib2/include/match.h"

#include "coff.h"
#include "Lock.h"

#include "../fastdb/inc/fastdb.h"

#include "PEFuncs.h"
//#include <tchar.h>
#include <process.h>

USE_FASTDB_NAMESPACE

typedef struct
{
	int startEA;
	int len;
	char lib_name[256];
	char is_export_function;
} function_info;


// information of identified library
typedef struct
{
    int start_node; // start node index
    int length; // lib length
} identified_lib_info;

// library database
class LibM
{
public:
	int4 block_size;
	int4 cfg;
	int1 is_inline_function;
	char const *lib_name;
	int4 instruction_size; // instruction number
	int4 MOV_COUNT;
	int4 CTI_COUNT;
	int4 ARITHMETIC_COUNT;
	int4 LOGI_COUNT;
	int4 STRING_COUNT;
	int4 ETC_COUNT;
	TYPE_DESCRIPTOR((
		KEY(block_size, INDEXED),
		KEY(MOV_COUNT, INDEXED),
		KEY(CTI_COUNT, INDEXED),
		KEY(ARITHMETIC_COUNT, INDEXED),
		KEY(LOGI_COUNT, INDEXED),
		KEY(STRING_COUNT, INDEXED),
		KEY(ETC_COUNT, INDEXED),
		FIELD(cfg),
		FIELD(is_inline_function),
		FIELD(lib_name),
		KEY(instruction_size, INDEXED)));
};

typedef struct
{
	Graph *m;
	Graph *g;
	PCBitSet lib_info;
	bool result;
} TEST_CONTEXT;


void* disasm(byte *bin, int length, bool for_lib, char *lib_name,  bool is_inlined_function = true, char *from_file = NULL);
void build_instruction_db();
bool my_visitor(int n, node_id ni1[], node_id ni2[], void *usr_data);
bool match(Graph *graph_g, Graph *graph_m, PCBitSet lib_info);
bool matchBBSF(ControlFlowGraph *g, ControlFlowGraph *m);
long GetM();
unsigned __stdcall TestThread(void *pParam);
bool myfunction(function_info& i, function_info& j);
bool validate_result(ControlFlowGraph* GT, int n, node_id ni2[]);

// minimal instruction size
#define MIN_INS_LENGTH (6)


#ifdef _DEBUG
#pragma comment(lib,"../fastdb/vs2013/fastdb_d.lib")
#else
#pragma comment(lib,"../fastdb/vs2013/fastdb.lib")
#endif

#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"../beaengine-win32/Win32/dll/BeaEngine.lib")

#define TIME_OUT (10*1000)