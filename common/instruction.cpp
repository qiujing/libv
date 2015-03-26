/********************************************************************
	created:	2012/04/17
	created:	17:4:2012   19:26
	file base:	instruction
	file ext:	cpp
	author:		qj

	purpose:	instruction class
*********************************************************************/
#include "instruction.h"
#include <iostream>
#include <sstream>
using namespace std;
char *var_names[] = {"al", "ax", "ah", "eax", "cl", "cx", "ch", "ecx", "dl", "dx", "dh", "edx", "bl", "bx", "bh", "ebx", "sp", "esp", "bp", "ebp", "si", "esi", "di", "edi", "es", "cs", "ss", "ds", "fs", "gs", "of", "sf", "zf", "af", "pf", "cf", "tf", "if", "df", "nt", "rf", "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7", "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7" , "v_mem"
                    };
//int var_i[] = {AL, AX, AH, EAX, CL, CX, CH, ECX, DL, DX, DH, EDX, BL, BX, BH, EBX, SP, ESP, BP, EBP, SI, ESI, DI, EDI, ES, CS, SS, DS, FS, GS, OF, SF, ZF, AF, PF, CF, TF, IF, DF, NT, RF, CR0, CR1, CR2, CR3, CR4, CR5, CR6, CR7, DR0, DR1, DR2, DR3, DR4, DR5, DR6, DR7 ,V_MEM
//};

char *reg_names[] = {"al", "ax", "ah", "eax", "cl", "cx", "ch", "ecx", "dl", "dx", "dh", "edx", "bl", "bx", "bh", "ebx", "sp", "esp", "bp", "ebp", "si", "esi", "di", "edi"};
int reg_i[] = {AL, AX, AH, EAX, CL, CX, CH, ECX, DL, DX, DH, EDX, BL, BX, BH, EBX, SP, ESP, BP, EBP, SI, ESI, DI, EDI};

#define PATCH(eax,ax,ah,al); 	if (b[eax]) \
    {\
        b.set(ax);\
        b.set(ah);\
        b.set(al);\
    }\
    else if (b[ax])\
    {\
        b.set(eax);\
        b.set(ah);\
        b.set(al);\
    }\
    else if (b[ah])\
    {\
        b.set(eax);\
        b.set(ax);\
    }\
    else if (b[al])\
    {\
        b.set(eax);\
        b.set(ax);\
    }\
     
#define PATCH2(esp,sp); if (b[esp])\
    {\
        b.set(sp);\
    }\
    else if (b[sp]) {\
        b.set(esp);\
    }\
     
// patch
void Instruction::patch(bitset<VAR_LENGTH> &b)
{
    PATCH(EAX, AX, AH, AL);
    PATCH(EBX, BX, BH, BL);
    PATCH(ECX, CX, CH, CL);
    PATCH(EDX, DX, DH, DL);
    PATCH2(EDI, DI);
    PATCH2(ESP, SP);
    PATCH2(EBP, BP);
    PATCH2(ESI, SI);
}

#define UNPATCH(eax,ax,ah,al); 	if (b[eax]) \
    {\
        b.reset(ax);\
        b.reset(ah);\
        b.reset(al);\
    }\
    else if (b[ax])\
    {\
        b.reset(eax);\
        b.reset(ah);\
        b.reset(al);\
    }\
    else if (b[ah])\
    {\
        b.reset(eax);\
        b.reset(ax);\
    }\
    else if (b[al])\
    {\
        b.reset(eax);\
        b.reset(ax);\
    }\
     
#define UNPATCH2(esp,sp); if (b[esp])\
    {\
        b.reset(sp);\
    }\
    else if (b[sp]) {\
        b.reset(esp);\
    }\
     
// unpatch
void Instruction::unpatch(bitset<VAR_LENGTH> &b)
{
    UNPATCH(EAX, AX, AH, AL);
    UNPATCH(EBX, BX, BH, BL);
    UNPATCH(ECX, CX, CH, CL);
    UNPATCH(EDX, DX, DH, DL);
    UNPATCH2(EDI, DI);
    UNPATCH2(ESP, SP);
    UNPATCH2(EBP, BP);
    UNPATCH2(ESI, SI);
}

// get R/W
void Instruction::parse()
{
    if (diasm.Instruction.Opcode == 0x90) // NOP?
    {
        return;
    }

    _parse(diasm.Argument1);
    _parse(diasm.Argument2);

    int imp = diasm.Instruction.ImplicitModifiedRegs;
#define CHECK_M(reg); \
    if (imp & reg) \
    { \
        W.set(REG2Var(reg));\
    } \
     
    CHECK_M(REG0);
    CHECK_M(REG1);
    CHECK_M(REG2);
    CHECK_M(REG3);
    CHECK_M(REG4);
    CHECK_M(REG5);
    CHECK_M(REG6);
    CHECK_M(REG7);

    // fix
    char * cmd = diasm.Instruction.Mnemonic;
    if ((!strcmp(cmd, "inc ")) || (!strcmp(cmd, "not "))
            || !strcmp(cmd, "neg ")
            || !strcmp(cmd, "or ")
            || !strcmp(cmd, "rcl ")
            || !strcmp(cmd, "rcr ")
            || !strcmp(cmd, "rol ")
            || !strcmp(cmd, "ror ")
            || !strcmp(cmd, "sal ")
            || !strcmp(cmd, "sar ")
            || !strcmp(cmd, "shl ")
            || !strcmp(cmd, "shr ")
       )
    {
        R.reset();
        R |= W;
        //if (diasm.Argument1.ArgType & MEMORY_TYPE)   // not []
        {
            //W.reset();//TODO: trace the mem
        }
    }
    else if (!strncmp(cmd, "scas", 4))
    {
        //R.set(EAX);
        R.set(ECX);
        W.set(ECX);
    }
    else if (!strcmp(cmd, "xor ") || !strcmp(cmd, "add ") || !strcmp(cmd, "dec ")
             || !strcmp(cmd, "and ")
             || !strcmp(cmd, "sub ")
             || !strcmp(cmd, "xchg ")
             || !strcmp(cmd, "xadd ")
            )
    {
        if (diasm.Argument1.ArgType & REGISTER_TYPE)
        {
            W |= R;
        }
        else if (diasm.Argument1.ArgType & MEMORY_TYPE) // and [], reg?
        {
            R.set(V_MEM);
            W.set(V_MEM);
        }
    }
    else if (!strcmp(cmd, "aaa "))
    {
        W.set(AF);
        W.set(CF);
    }
    else if (!strcmp(cmd, "aad "))
    {
        R.set(AX);
        W.set(AX);
    }
    else if (!strcmp(cmd, "aam "))
    {
        R.set(AL);
        W.set(AX);
        W.set(SF);
        W.set(ZF);
        W.set(PF);
    }
    else if (!strcmp(cmd, "aas "))
    {
        R.set(AX);
        R.set(AH);
        W.set(AL);
        W.set(AF);
        W.set(CF);
    }
    else if (!strcmp(cmd, "adc "))
    {
        if (diasm.Argument1.ArgType & REGISTER_TYPE)
        {
            W |= R;
        }
        R.set(CF);
    }
    else if (!strcmp(cmd, "lea "))
    {
        //W.reset(V_MEM);
    }
    else if (!strncmp(cmd, "cmps", 4))
    {
        R.set(ESI);
        R.set(EDI);
        W.set(ESI);
        W.set(EDI);
    }
    else if (!strcmp(cmd, "call "))
    {
        //W.set(EAX);// always assume that EAX stores the result
        R.set();
        W.set();
    }

    if (diasm.Prefix.RepPrefix == InUsePrefix ||  diasm.Prefix.RepnePrefix == InUsePrefix)
    {
        R.set(ECX);
        W.set(ECX);
    }

    // EFLAGS
    EFLStruct & f =  (diasm.Instruction.Flags);
#if 1
#define EF(f,of);	\
    if (f) \
    { \
        if (f & TE_) R.set(of);\
        if (f & (MO_ | RE_ | SE_ | PR_)) W.set(of);\
    }\
     
    EF(f.OF_, OF);
    EF(f.SF_, SF);
    EF(f.ZF_, ZF);
    EF(f.AF_, AF);
    EF(f.PF_, PF);
    EF(f.CF_, CF);
    EF(f.TF_, TF);
    EF(f.IF_, IF);
    EF(f.DF_, DF);
    EF(f.NT_, NT);
    EF(f.RF_, RF);
#endif
    //patch(R);
    //patch(W);
}

void print_(bitset<VAR_LENGTH> &b)
{
    for (int i = 0; i < VAR_LENGTH; i++)
    {
        if (b[i])
            std::cout << var_names[i] << " ";
        //		std::cout<<i<<" ";
    }
    std::cout << std::endl;
}

stringstream &print(bitset<VAR_LENGTH> &b)
{
    stringstream *s = new stringstream;
    for (int i = 0; i < VAR_LENGTH; i++)
    {
        if (b[i])
            *s << var_names[i] << " ";
        //*s<<i<<" ";
    }
    return *s;
}

void Instruction::print_R()
{
    std::cout << "R: ";
    print_(R);
}

void Instruction::print_W()
{
    std::cout << "W: ";
    print_(W);
}

int Instruction::REG2Var(int reg)
{
    switch (reg)
    {
    case REG0:
        return EAX;
    case REG1:
        return ECX;
    case REG2:
        return EDX;
    case REG3:
        return EBX;
    case REG4:
        return ESP;
    case REG5:
        return EBP;
    case REG6:
        return ESI;
    case REG7:
        return EDI;
    }
}

// RW
void Instruction::_parse( ARGTYPE arg )
{
    int i;
    if (arg.ArgType & MEMORY_TYPE)
    {
        if (arg.Memory.BaseRegister)
        {
            R.set(REG2Var(arg.Memory.BaseRegister));
        }
        if (arg.Memory.IndexRegister)
        {
            R.set(REG2Var(arg.Memory.IndexRegister));
        }
        (arg.AccessMode == READ ? R : W).set(V_MEM);

        // do not care about the segment reg
    }
    else if (arg.ArgType & (REGISTER_TYPE | GENERAL_REG))
    {
        if (arg.ArgMnemonic[0])
        {
            for (i = 0; i < sizeof(reg_i) / 4; i++)
            {
                if (!strcmp(arg.ArgMnemonic, reg_names[i]))
                {
                    (arg.AccessMode == READ ? R : W).set(i);
                    break;
                }
            }
        }
        else
        {
#define CHECK(reg,al,ax,eax); \
    if (arg.ArgType & reg) \
    { \
        if (arg.ArgSize==8) b.set(al); \
        if (arg.ArgSize==16) b.set(ax); \
        if (arg.ArgSize==32) b.set(eax); \
    } \
     
#define CHECK2(reg,sp,esp); \
    if (arg.ArgType & reg) \
    { \
        if (arg.ArgSize==16) b.set(sp); \
        if (arg.ArgSize==32) b.set(esp); \
    } \
     
            bitset<VAR_LENGTH> & b = arg.AccessMode == READ ? R : W;
            CHECK(REG0, AL, AX, EAX);
            CHECK(REG1, CL, CX, ECX);
            CHECK(REG2, DL, DX, EDX);
            CHECK(REG3, BL, BX, EBX);
            CHECK2(REG4, SP, ESP);
            CHECK2(REG5, BP, EBP);
            CHECK2(REG6, SI, ESI);
            CHECK2(REG7, DI, EDI);
        }
    }
}

Instruction::Instruction()
{
    JMP_ID = -1;
    db_id = 0;
    basic_block = NULL;
}

//
// emit asm code
//
void Instruction::print_disassembly()
{
    std::cout << ID << " " << diasm.CompleteInstr << endl;
}

//
// return if has data depence on "ins"
//
bool Instruction::hasDependency(Instruction &ins)
{
    bitset<VAR_LENGTH> bs;
    bs = W;
    bs &= ins.R;
    if (bs.any()) return true;

    bs = ins.W;
    bs &= R;
    if (bs.any()) return true;

    bs = ins.W;
    bs &= W;
    if (bs.any()) return true;

    return false;
}

// compare
bool Instruction::equal(Instruction& rhs)
{
    if (!strcmp(diasm.CompleteInstr, rhs.diasm.CompleteInstr))
    {
        return true;
    }
    return false;
}

// standarize instruction
void Instruction::generalize()
{
    generalize_result = 0;
    if (diasm.Argument1.ArgType == NO_ARGUMENT)
    {
        return;
    }

    // first operand
    if (diasm.Argument1.ArgType & (REGISTER_TYPE | GENERAL_REG))
    {
        generalize_result = 1;
    }
    else if ((diasm.Argument1.ArgType & MEMORY_TYPE) && (diasm.Argument1.ArgMnemonic[0]))
    {
        generalize_result = 2;
    }
    else if (diasm.Argument1.ArgType & CONSTANT_TYPE)
    {
        generalize_result = 5;
    }
    if (diasm.Argument2.ArgType == NO_ARGUMENT)
    {
        return;
    }

    // second
    if (diasm.Argument2.ArgType & (REGISTER_TYPE | GENERAL_REG))
    {
        if (generalize_result == 1)   // first op is reg?
        {
            int r1 = diasm.Argument1.ArgType & (0x8FFF);
            int r2 = diasm.Argument2.ArgType & (0x8FFF);
            if (r1 & r2)   // op1 and op2 are the same?
            {
                generalize_result += 10;
            }
            else
            {
                generalize_result += 20;
            }
        }
        else     // first op is not reg£¿
        {
            generalize_result += 10;
        }
    }
    else if (diasm.Argument2.ArgType & MEMORY_TYPE)
    {
        generalize_result += 40;
    }
    else if (diasm.Argument2.ArgType & CONSTANT_TYPE)
    {
        generalize_result += 50;
    }
    if (diasm.Argument3.ArgType == NO_ARGUMENT)
    {
        return;
    }

    // third
    if (diasm.Argument3.ArgType & (REGISTER_TYPE | GENERAL_REG))
    {
        int r1 = diasm.Argument1.ArgType & (0x8FFF);
        int r2 = diasm.Argument2.ArgType & (0x8FFF);
        int r3 = diasm.Argument3.ArgType & (0x8FFF);

        if (r1 & r3)
        {
            generalize_result += 100;
        }
        else
        {
            if (r3 & r2)
            {
                generalize_result += 200;
            }
            else
            {
                generalize_result += 300;
            }
        }
    }
    else if (diasm.Argument3.ArgType & MEMORY_TYPE)
    {
        generalize_result += 400;
    }
    else if (diasm.Argument3.ArgType & CONSTANT_TYPE)
    {
        generalize_result += 500;
    }
}

bool Instruction::IsCTI()
{
	return ((diasm.Instruction.Category & 0xFF) == CONTROL_TRANSFER);
}

bool Instruction::argument_is_constant_memory(ARGTYPE *arg)
{
    if (arg->ArgType & MEMORY_TYPE)
    {
        if (arg->Memory.BaseRegister == 0
                && arg->Memory.IndexRegister == 0
                && arg->Memory.Scale == 0)
        {
            return true;
        }
    }
    return false;
}

int Instruction::constant_memory_argument()
{
    if (argument_is_constant_memory(&diasm.Argument1))
    {
        return 1;
    }

    if (argument_is_constant_memory(&diasm.Argument2))
    {
        return 2;
    }

    if (argument_is_constant_memory(&diasm.Argument3))
    {
        return 3;
    }

    return 0;
}

bool Instruction::is_memory_access()
{
    return (diasm.Argument1.ArgType & MEMORY_TYPE) || (diasm.Argument2.ArgType & MEMORY_TYPE) || (diasm.Argument3.ArgType & MEMORY_TYPE);
}