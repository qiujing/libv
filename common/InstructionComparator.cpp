#include "InstructionComparator.h"
#include "ControlFlowGraph.h"

InstructionComparator::InstructionComparator(void)
{
}

InstructionComparator::~InstructionComparator(void)
{
}

bool InstructionComparator::compatible(void *attr1, void *attr2)
{
    BasicBlock *ba = (BasicBlock *)attr1;
    BasicBlock *bb = (BasicBlock *)attr2;
	bool ret = ba->equal(bb);

	//if (!ret)
	//{
	//	if (ba->size()==bb->size()){
	//		//ret = ba->equal(bb);
	//		bb->OAM->print();
	//		printf("..........\n");
	//		ba->OAM->print();
	//	}
	//}
	return ret;
}