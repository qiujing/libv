/********************************************************************
	created:	2012/04/17
	created:	17:4:2012   19:17
	filename: 	f:\øÏ≈Ã\study\lib\lib\instruction.h
	file path:	f:\øÏ≈Ã\study\lib\lib
	file base:	instruction
	file ext:	h
	author:		qj
	
	purpose:	instruction class
*********************************************************************/
#ifndef INSTRUCTION_HEAD
#define INSTRUCTION_HEAD
#define BEA_ENGINE_STATIC
#define BEA_USE_STDCALL
#include "../beaengine-win32/headers/BeaEngine.h"
#include <bitset>
#include <sstream>
using namespace std;
#include "define.h"

stringstream &print(bitset<VAR_LENGTH> &b);
class Instruction 
{
public:
	bitset<VAR_LENGTH> R; // read set
	bitset<VAR_LENGTH> W; // write set
	DISASM diasm;	
	int ID;
	int JMP_ID; // jxx target
	int db_id;// instruction ID
	short int generalize_result;
	void *basic_block; // belong to which basic block.
	unsigned char byte_length;
public:
	Instruction();
	void print_R();
	void print_W();
	void parse();
	static void patch(bitset<VAR_LENGTH> &b);
	static void unpatch(bitset<VAR_LENGTH> &b);
	void print_disassembly();
	bool hasDependency(Instruction &ins);
	bool equal(Instruction& rhs );
	void generalize();
	bool IsCTI();
	int constant_memory_argument();
	bool is_memory_access();
private:
	void _parse(ARGTYPE arg);
	int REG2Var(int reg);
	bool argument_is_constant_memory(ARGTYPE *arg);
};

typedef Instruction* pInstruction;
#endif
