/*------------------------------------------------------------------
 * vf2_sub_state.cc
 * Implementation of the class VF2SubState
 *
 * Author: P. Foggia
 *-----------------------------------------------------------------*/



/*-----------------------------------------------------------------
 * NOTE:
 *   The attribute compatibility check (methods CompatibleNode
 *   and CompatibleEdge of ARGraph) is always performed
 *   applying the method to g1, and passing the attribute of
 *   g1 as first argument, and the attribute of g2 as second
 *   argument. This may be important if the compatibility
 *   criterion is not symmetric.
 -----------------------------------------------------------------*/


/*---------------------------------------------------------
 *   IMPLEMENTATION NOTES:
 * The six vectors core_1, core_2, in_1, in_2, out_1, out_2,
 * are shared among the instances of this class; they are
 * owned by the instance with core_len==0 (the root of the
 * SSR).
 * In the vectors in_* and out_* there is a value indicating
 * the level at which the corresponding node became a member
 * of the core or of Tin (for in_*) or Tout (for out_*),
 * or 0 if the node does not belong to the set.
 * This information is used for backtracking.
 * The fields t1out_len etc. also count the nodes in core.
 * The true t1out_len is t1out_len-core_len!
 ---------------------------------------------------------*/

#include <cilk/cilk.h>

#include <stddef.h>
#include "vf2_map_sub_state.h"


#include "sortnodes.h"
#include "error.h"

//#include <sys/time.h>
#include <sys/types.h>
#include "stat_obj.h"

#include <map>

#include <string.h>
#include <iostream>

#include <stdio.h>

/*----------------------------------------------------------
 * Methods of the class VF2MapSubState
 ---------------------------------------------------------*/

#ifdef TIME_DEB
static double tdiff(struct timeval* a, struct timeval* b)
// Effect: Compute the difference of b and a, that is $b-a$
//  as a double with units in seconds
{
	return a->tv_sec - b->tv_sec + 1e-6 * (a->tv_usec - b->tv_usec);
}
#endif
/*----------------------------------------------------------
 * VF2MapSubState::VF2MapSubState(g1, g2, sortNodes)
 * Constructor. Makes an empty state.
 * If sortNodes is true, computes an initial ordering
 * for the nodes based on the frequency of their valence.
 ---------------------------------------------------------*/
//__cilk
VF2MapSubState::VF2MapSubState(Graph* ag1, Graph* ag2, bool sortNodes)
{
	g1 = ag1;
	g2 = ag2;
	n1 = g1->NodeCount();
	n2 = g2->NodeCount();

	core_len = orig_core_len = 0;
	t1both_len = t1in_len = t1out_len = 0;
	t2both_len = t2in_len = t2out_len = 0;

	added_node1 = NULL_NODE;

	core_1 = new std::map<int, node_id>();
	core_2 = new std::map<int, node_id>();

	in_1 = new std::map<int, node_id>();
	in_2 = new std::map<int, node_id>();

	out_1 = new std::map<int, node_id>();
	out_2 = new std::map<int, node_id>();

	share_count = new long;

	if (!core_1 || !core_2 || !in_1 || !in_2
			|| !out_1 || !out_2 || !share_count)
		error("Out of memory");

	*share_count = 1;

#ifdef TIME_DEB
	stat = new StatObj();
#endif

}


/*----------------------------------------------------------
 * VF2MapSubState::VF2MapSubState(state)
 * Copy constructor.
 ---------------------------------------------------------*/
VF2MapSubState::VF2MapSubState(const VF2MapSubState& state)
{
#ifdef TIME_DEB
	stat = state.stat;
	struct timeval start, end;
	gettimeofday(&start, 0);
#endif

	g1 = state.g1;
	g2 = state.g2;
	n1 = state.n1;
	n2 = state.n2;

	core_len = orig_core_len = state.core_len;
	t1in_len = state.t1in_len;
	t1out_len = state.t1out_len;
	t1both_len = state.t1both_len;
	t2in_len = state.t2in_len;
	t2out_len = state.t2out_len;
	t2both_len = state.t2both_len;

	added_node1 = NULL_NODE;

	core_1 = state.core_1;
	core_2 = state.core_2;
	in_1 = state.in_1;
	in_2 = state.in_2;
	out_1 = state.out_1;
	out_2 = state.out_2;
	share_count = state.share_count;

	++ *share_count;

#ifdef TIME_DEB
	gettimeofday(&end, 0);
	stat->sc_count++;
	stat->sc_time += tdiff(&end, &start);
#endif

}

#define VCOPY(dest, src, type, nelem) memcpy((dest),(src),(nelem)*sizeof(type))

VF2MapSubState::VF2MapSubState(const VF2MapSubState& state, bool deep)
{
	if (!deep)
		return;

#ifdef TIME_DEB
	stat = state.stat;
	struct timeval start, end;
	gettimeofday(&start, 0);
#endif

	// Graphs do not need to be deep.
	g1 = state.g1;
	g2 = state.g2;
	n1 = state.n1;
	n2 = state.n2;

	//  order=state.order;

	core_len = orig_core_len = state.core_len;
	t1in_len = state.t1in_len;
	t1out_len = state.t1out_len;
	t1both_len = state.t1both_len;
	t2in_len = state.t2in_len;
	t2out_len = state.t2out_len;
	t2both_len = state.t2both_len;

	added_node1 = NULL_NODE;

	//  printf("Core Sparseness: %d,%d \n", (core_len + t1in_len + t1out_len +
	//				       t1both_len + t2in_len + t2out_len + t2both_len), 3 * n1 + 3 * n2);

	core_1 = new std::map<int, node_id>(*state.core_1);
	core_2 = new std::map<int, node_id>(*state.core_2);
	in_1 = new std::map<int, node_id>(*state.in_1);
	in_2 = new std::map<int, node_id>(*state.in_2);
	out_1 = new std::map<int, node_id>(*state.out_1);
	out_2 = new std::map<int, node_id>(*state.out_2);
	share_count = new long;
	*share_count = 1;


#ifdef TIME_DEB
	gettimeofday(&end, 0);
	stat->dc_count++;
	stat->dc_time += tdiff(&end, &start);
#endif
}



void VF2MapSubState::propogate(const State& state1)
{
	VF2MapSubState& state = (VF2MapSubState&) state1;

	VCOPY(core_1, state.core_1, node_id, n1);
	VCOPY(core_2, state.core_2, node_id, n2);
	VCOPY(in_1, state.in_1, node_id, n1);
	VCOPY(in_2, state.in_2, node_id, n2);
	VCOPY(out_1, state.out_1, node_id, n1);
	VCOPY(out_2, state.out_2, node_id, n2);
}

/*---------------------------------------------------------------
 * VF2MapSubState::~VF2MapSubState()
 * Destructor.
 --------------------------------------------------------------*/
VF2MapSubState::~VF2MapSubState()
{
	if (-- *share_count == 0)
	{
		delete [] core_1;
		delete [] core_2;
		delete [] in_1;
		delete [] out_1;
		delete [] in_2;
		delete [] out_2;
		delete share_count;
	}
}


/*--------------------------------------------------------------------------
 * bool VF2MapSubState::NextPair(pn1, pn2, prev_n1, prev_n2)
 * Puts in *pn1, *pn2 the next pair of nodes to be tried.
 * prev_n1 and prev_n2 must be the last nodes, or NULL_NODE (default)
 * to start from the first pair.
 * Returns false if no more pairs are available.
 -------------------------------------------------------------------------*/
bool VF2MapSubState::NextPair(node_id* pn1, node_id* pn2,
							  node_id prev_n1, node_id prev_n2)
{
#ifdef TIME_DEB
	struct timeval start, end;
	gettimeofday(&start, 0);
#endif

	if (prev_n1 == NULL_NODE)
		prev_n1 = 0;
	if (prev_n2 == NULL_NODE)
		prev_n2 = 0;
	else
		prev_n2++;

	if (t1both_len > core_len && t2both_len > core_len)
	{
		while (prev_n1 < n1 &&
				(core_1->count((int)prev_n1) || ! out_1->count((int)prev_n1)
				 || ! in_1->count((int)prev_n1)))
		{
			prev_n1++;
			prev_n2 = 0;
		}
	}
	else if (t1out_len > core_len && t2out_len > core_len)
	{
		while (prev_n1 < n1 &&
				(core_1->count((int)prev_n1) || !out_1->count((int)prev_n1)))
		{
			prev_n1++;
			prev_n2 = 0;
		}
	}
	else if (t1in_len > core_len && t2in_len > core_len)
	{
		while (prev_n1 < n1 &&
				(core_1->count((int)prev_n1) || !in_1->count((int)prev_n1)))
		{
			prev_n1++;
			prev_n2 = 0;
		}
		//}else if (prev_n1==0 && order!=NULL){
		//int i=0;
		//while (i<n1 && core_1[prev_n1=order[i]] != NULL_NODE)
		//  i++;
		//if (i==n1)
		//  prev_n1=n1;
	}
	else
	{
		while (prev_n1 < n1 && core_1->count((int)prev_n1))
		{
			prev_n1++;
			prev_n2 = 0;
		}
	}

	if (t1both_len > core_len && t2both_len > core_len)
	{
		while (prev_n2 < n2 &&
				(core_2->count(prev_n2) || ! out_2->count(prev_n2)
				 || ! in_2->count(prev_n2)))
		{
			prev_n2++;
		}
	}
	else if (t1out_len > core_len && t2out_len > core_len)
	{
		while (prev_n2 < n2 &&
				(core_2->count(prev_n2) || ! out_2->count(prev_n2)))
		{
			prev_n2++;
		}
	}
	else if (t1in_len > core_len && t2in_len > core_len)
	{
		while (prev_n2 < n2 &&
				(core_2->count(prev_n2) || ! in_2->count(prev_n2)))
		{
			prev_n2++;
		}
	}
	else
	{
		while (prev_n2 < n2 && core_2->count(prev_n2))
		{
			prev_n2++;
		}
	}

#ifdef TIME_DEB
	gettimeofday(&end, 0);
	stat->np_count++;
	stat->np_time += tdiff(&end, &start);
#endif

	if (prev_n1 < n1 && prev_n2 < n2)
	{
		*pn1 = prev_n1;
		*pn2 = prev_n2;
		return true;
	}

	return false;
}



/*---------------------------------------------------------------
 * bool VF2MapSubState::IsFeasiblePair(node1, node2)
 * Returns true if (node1, node2) can be added to the state
 * NOTE:
 *   The attribute compatibility check (methods CompatibleNode
 *   and CompatibleEdge of ARGraph) is always performed
 *   applying the method to g1, and passing the attribute of
 *   g1 as first argument, and the attribute of g2 as second
 *   argument. This may be important if the compatibility
 *   criterion is not symmetric.
 --------------------------------------------------------------*/
bool VF2MapSubState::IsFeasiblePair(node_id node1, node_id node2)
{
	assert(node1 < n1);
	assert(node2 < n2);
	assert(! core_1->count(node1));
	assert(! core_2->count(node2));

	if (!g1->CompatibleNode(g1->GetNodeAttr(node1), g2->GetNodeAttr(node2)))
		return false;

	int i, other1, other2;
	void* attr1;
	int termout1 = 0, termout2 = 0, termin1 = 0, termin2 = 0, new1 = 0, new2 = 0;

#ifdef TIME_DEB
	struct timeval start, end;
	gettimeofday(&start, 0);
#endif

	// Check the 'out' edges of node1
	for (i = 0; i < g1->OutEdgeCount(node1); i++)
	{
		other1 = g1->GetOutEdge(node1, i, &attr1);
		if (core_1->count(other1))
		{
			other2 = (* core_1)[other1];
			if (!g2->HasEdge(node2, other2) ||
					!g1->CompatibleEdge(attr1, g2->GetEdgeAttr(node2, other2)))
				return false;
		}
		else
		{
			if (in_1->count(other1))
				termin1++;
			if (out_1->count(other1))
				termout1++;
			if (!in_1->count(other1) && !out_1->count(other1))
				new1++;
		}
	}

	// Check the 'in' edges of node1
	for (i = 0; i < g1->InEdgeCount(node1); i++)
	{
		other1 = g1->GetInEdge(node1, i, &attr1);
		if (core_1->count(other1))
		{
			other2 = (* core_1)[other1];
			if (!g2->HasEdge(other2, node2) ||
					!g1->CompatibleEdge(attr1, g2->GetEdgeAttr(other2, node2)))
				return false;
		}
		else
		{
			if (in_1->count(other1))
				termin1++;
			if (out_1->count(other1))
				termout1++;
			if (!in_1->count(other1) && !out_1->count(other1))
				new1++;
		}
	}


	// Check the 'out' edges of node2
	for (i = 0; i < g2->OutEdgeCount(node2); i++)
	{
		other2 = g2->GetOutEdge(node2, i);
		if (core_2->count(other2))
		{
			other1 = (* core_2)[other2];
			if (!g1->HasEdge(node1, other1))
				return false;
		}
		else
		{
			if (in_2->count(other2))
				termin2++;
			if (out_2->count(other2))
				termout2++;
			if (!in_2->count(other2) && !out_2->count(other2))
				new2++;
		}
	}

	// Check the 'in' edges of node2
	for (i = 0; i < g2->InEdgeCount(node2); i++)
	{
		other2 = g2->GetInEdge(node2, i);
		if (core_2->count(other2))
		{
			other1 = (* core_2)[other2];
			if (!g1->HasEdge(other1, node1))
				return false;
		}
		else
		{
			if (in_2->count(other2))
				termin2++;
			if (out_2->count(other2))
				termout2++;
			if (!in_2->count(other2) && !out_2->count(other2))
				new2++;
		}
	}

#ifdef TIME_DEB
	gettimeofday(&end, 0);
	stat->fp_count++;
	stat->fp_time += tdiff(&end, &start);
#endif

	return termin1 <= termin2 && termout1 <= termout2 && new1 <= new2;
}



/*--------------------------------------------------------------
 * void VF2MapSubState::AddPair(node1, node2)
 * Adds a pair to the Core set of the state.
 * Precondition: the pair must be feasible
 -------------------------------------------------------------*/
void VF2MapSubState::AddPair(node_id node1, node_id node2)
{
	assert(node1 < n1);
	assert(node2 < n2);
	assert(core_len < n1);
	assert(core_len < n2);

#ifdef TIME_DEB
	struct timeval start, end;
	gettimeofday(&start, 0);
#endif

	core_len++;
	added_node1 = node1;

	if (!in_1->count(node1))
	{
		(*in_1)[node1] = core_len;
		t1in_len++;
		if (out_1->count(node1))
			t1both_len++;
	}
	if (!out_1->count(node1))
	{
		(*out_1)[node1] = core_len;
		t1out_len++;
		if (in_1->count(node1))
			t1both_len++;
	}

	if (!in_2->count(node2))
	{
		(*in_2)[node2] = core_len;
		t2in_len++;
		if (out_2->count(node2))
			t2both_len++;
	}
	if (!out_2->count(node2))
	{
		(*out_2)[node2] = core_len;
		t2out_len++;
		if (in_2->count(node2))
			t2both_len++;
	}

	(*core_1)[node1] = node2;
	(*core_2)[node2] = node1;


	int i, other;
	for (i = 0; i < g1->InEdgeCount(node1); i++)
	{
		other = g1->GetInEdge(node1, i);
		if (!in_1->count(other))
		{
			(*in_1)[other] = core_len;
			t1in_len++;
			if (out_1->count(other))
				t1both_len++;
		}
	}

	for (i = 0; i < g1->OutEdgeCount(node1); i++)
	{
		other = g1->GetOutEdge(node1, i);
		if (!out_1->count(other))
		{
			(*out_1)[other] = core_len;
			t1out_len++;
			if (in_1->count(other))
				t1both_len++;
		}
	}

	for (i = 0; i < g2->InEdgeCount(node2); i++)
	{
		other = g2->GetInEdge(node2, i);
		if (!in_2->count(other))
		{
			(*in_2)[other] = core_len;
			t2in_len++;
			if (out_2->count(other))
				t2both_len++;
		}
	}

	for (i = 0; i < g2->OutEdgeCount(node2); i++)
	{
		other = g2->GetOutEdge(node2, i);
		if (!out_2->count(other))
		{
			(*out_2)[other] = core_len;
			t2out_len++;
			if (in_2->count(other))
				t2both_len++;
		}
	}

#ifdef TIME_DEB
	gettimeofday(&end, 0);
	stat->ap_count++;
	stat->ap_time += tdiff(&end, &start);
#endif

}



/*--------------------------------------------------------------
 * void VF2MapSubState::GetCoreSet(c1, c2)
 * Reads the core set of the state into the arrays c1 and c2.
 * The i-th pair of the mapping is (c1[i], c2[i])
 --------------------------------------------------------------*/
void VF2MapSubState::GetCoreSet(node_id c1[], node_id c2[])
{
	int i, j;
	// TODO: perfect candidate for an iterator...
	for (i = 0, j = 0; i < n1; i++)
	{
		if (core_1->count(i))
		{
			c1[j] = i;
			c2[j] = (* core_1)[i];
			j++;
		}
	}
}


/*----------------------------------------------------------------
 * Clones a VF2MapSubState, allocating with new the clone.
 --------------------------------------------------------------*/
State* VF2MapSubState::Clone()
{
	return new VF2MapSubState(*this);
}

State* VF2MapSubState::DeepClone()
{
	return new VF2MapSubState(*this, true);
}

/*----------------------------------------------------------------
 * Undoes the changes to the shared vectors made by the
 * current state. Assumes that at most one AddPair has been
 * performed.
 ----------------------------------------------------------------*/
void VF2MapSubState::BackTrack()
{
	assert(core_len - orig_core_len <= 1);
	assert(added_node1 != NULL_NODE);

#ifdef TIME_DEB
	struct timeval start, end;
	gettimeofday(&start, 0);
#endif

	if (orig_core_len < core_len)
	{
		int i, node2;

		if (in_1->count(added_node1) && (* in_1)[added_node1] == core_len)
			in_1->erase(added_node1);;
		for (i = 0; i < g1->InEdgeCount(added_node1); i++)
		{
			int other = g1->GetInEdge(added_node1, i);
			if (in_1->count(other) && (* in_1)[other] == core_len)
				in_1->erase(other);
		}

		if (out_1->count(added_node1) && (* out_1)[added_node1] == core_len)
			out_1->erase(added_node1);
		for (i = 0; i < g1->OutEdgeCount(added_node1); i++)
		{
			int other = g1->GetOutEdge(added_node1, i);
			if (out_1->count(other) && (* out_1)[other] == core_len)
				out_1->erase(other);
		}

		node2 = (* core_1)[added_node1];

		if (in_2->count(node2) && (* in_2)[node2] == core_len)
			in_2->erase(node2);
		for (i = 0; i < g2->InEdgeCount(node2); i++)
		{
			int other = g2->GetInEdge(node2, i);
			if (in_2->count(other) && (* in_2)[other] == core_len)
				in_2->erase(other);
		}

		if (out_2->count(node2) && (* out_2)[node2] == core_len)
			out_2->erase(node2);
		for (i = 0; i < g2->OutEdgeCount(node2); i++)
		{
			int other = g2->GetOutEdge(node2, i);
			if (out_2->count(other) && (* out_2)[other] == core_len)
				out_2->erase(other);
		}

		core_1->erase(added_node1);// = NULL_NODE;
		core_2->erase(node2);// = NULL_NODE;

		core_len = orig_core_len;
		added_node1 = NULL_NODE;

	}
#ifdef TIME_DEB
	gettimeofday(&end, 0);
	stat->bt_count++;
	stat->bt_time += tdiff(&end, &start);
#endif

}
