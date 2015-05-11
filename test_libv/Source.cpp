#include <cilk/cilk.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

#include "argraph.h"
#include "argedit.h"
#include "argloader.h"
#include "vf2_sub_state.h"
#include "match.h"
#include "vf2_map_sub_state.h"
#include "vf2_bitset_sub_state.h"

#include <Windows.h>

class EdgeComparator :
    public AttrComparator
{
public:
    EdgeComparator(void) {};
    ~EdgeComparator(void) {};
    bool EdgeComparator::compatible(void *attr1, void *attr2)
    {
        return (int)attr1 == (int)attr2;
    }
};

long found_c = 0;

bool my_visitor(int n, node_id ni1[], node_id ni2[], void* usr_data)
{
    PCBitSet lib_info = (PCBitSet)usr_data;

    // find
    if (lib_info->isset(ni2[0])) {}
    else
    {
        //bool passed = true;
        InterlockedExchangeAdd(&found_c, 1);
        //found_c++;
        lib_info->set(ni2[0]);

        for (size_t i = 0; i < n; i++)
        {
            printf("(%hd, %hd) ", ni1[i], ni2[i]);
        }
        printf("\n");
    }

    return false;
}

void para_norm_test()
{
    ARGEdit ed1, ed2;
    for (size_t i = 0; i < 7; i++)
    {
        ed1.InsertNode(nullptr);
    }

    for (size_t i = 0; i < 3; i++)
    {
        ed2.InsertNode(nullptr);
    }

    ed1.InsertEdge(0, 1, (void*)1);
    ed1.InsertEdge(0, 2, (void*)2);
    ed1.InsertEdge(2, 3, (void*)3);
    ed1.InsertEdge(3, 4, (void*)5);
    ed1.InsertEdge(4, 5, (void*)1);
    ed1.InsertEdge(4, 6, (void*)2);

    ed2.InsertEdge(0, 1, (void*)1);
    ed2.InsertEdge(0, 2, (void*)2);

    // Now the Graph can be constructed...
    Graph gA(&ed2);
    Graph gB(&ed1);
    gA.SetEdgeComparator(new EdgeComparator);
    VF2SubState s0(&gA, &gB);

    CBitSet lib_info(7);

    Match m(&s0, my_visitor, &lib_info);
    m.match_par();

    printf("Found %d match in test.\n", found_c);

    return;

}

int main(int argc, char *arg[])
{
    para_norm_test();
    return 0;
}

