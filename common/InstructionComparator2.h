#pragma once
#include "../vflib2/include/argraph.h"
class InstructionComparator2:
	public AttrComparator
{
public:
	InstructionComparator2(void);
	~InstructionComparator2(void);
	bool InstructionComparator2::compatible(void *attr1, void *attr2);
};

