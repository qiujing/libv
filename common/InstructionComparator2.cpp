#include "InstructionComparator2.h"
#include "ControlFlowGraph.h"

InstructionComparator2::InstructionComparator2(void)
{
}


InstructionComparator2::~InstructionComparator2(void)
{
}

bool InstructionComparator2::compatible( void *attr1, void *attr2 )
{
    VFNODE *a = (VFNODE *)attr1;
    VFNODE *b = (VFNODE *)attr2;

    if (a->isBB != b->isBB)
    {
        return false;
    }

    if (a->isBB) // basic block ?
    {
        BasicBlock *ba = (BasicBlock *)a->data;
        BasicBlock *bb = (BasicBlock *)b->data;

        if (ba->size() != bb->size())
        {
            return false;
        }

        bb->serialize();

        // compare IDS
        if (bb->vertexSeq.size() != ba->vertexSeq.size())
        {
            return false;
        }

        bool bEqual = true;
        for (int i = 0; i < bb->vertexSeq.size(); i++)
        {
            if (bb->vertexSeq[i]->ID != ba->vertexSeq[i]->ID)
            {
                bEqual = false;
                break;
            }
        }

        if (!bEqual)
        {
            return false;
        }

		// compare OAM
		if ((bb->vertexSeq.size()>1) && (bb->OAM!=NULL))
		{
			return bb->OAM->equal(ba->OAM);
		}
        
		/*bb->OAM->print();
		printf("..........\n");
		ba->OAM->print();*/

        return true;
    }
    else
    {
        return (a->data) == (b->data);
    }
}
