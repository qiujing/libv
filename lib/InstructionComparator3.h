#pragma once
#include "../vflib2/include/argraph.h"

class InstructionComparator :
	public AttrComparator
{
public:
	InstructionComparator(void);
	~InstructionComparator(void);
	bool InstructionComparator::compatible(void *attr1, void *attr2);
};
