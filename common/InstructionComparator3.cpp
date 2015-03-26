#include "InstructionComparator3.h"

InstructionComparator3::InstructionComparator3(void)
{
}

InstructionComparator3::~InstructionComparator3(void)
{
}

bool InstructionComparator3::compatible(void *attr1, void *attr2)
{
	return attr1==attr2;
}