#include "InstructionComparator.h"

InstructionComparator::InstructionComparator(void)
{
}

InstructionComparator::~InstructionComparator(void)
{
}

bool InstructionComparator::compatible(void *attr1, void *attr2)
{
	return attr1==attr2;
}