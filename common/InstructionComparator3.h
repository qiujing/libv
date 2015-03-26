#pragma once
#include "../vflib2/include/argraph.h"

class InstructionComparator3 :
	public AttrComparator
{
public:
	InstructionComparator3(void);
	~InstructionComparator3(void);
	bool InstructionComparator3::compatible(void *attr1, void *attr2);
};
