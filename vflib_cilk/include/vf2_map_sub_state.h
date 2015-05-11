/*------------------------------------------------------------
 * vf2_sub_state.h
 * Interface of vf2_sub_state.cc
 * Definition of a class representing a state of the matching
 * process between two ARGs.
 * See: argraph.h state.h
 *
 * Author: P. Foggia
 *-----------------------------------------------------------------*/




#ifndef VF2_MAP_SUB_STATE_H
#define VF2_MAP_SUB_STATE_H

#ifdef TIME_DEB
#include "stat_obj.h"
#endif

#include "argraph.h"
#include "state.h"
#include <map>


/*----------------------------------------------------------
 * class VF2SubState
 * A representation of the SSR current state
 * See vf2_sub_state.cc for more details.
 ---------------------------------------------------------*/
class VF2MapSubState: public State
  { typedef ARGraph_impl Graph;

    private:
      int core_len, orig_core_len;
      int added_node1;
      int t1both_len, t2both_len, t1in_len, t1out_len, 
          t2in_len, t2out_len; // Core nodes are also counted by these...
#ifdef TIME_DEB
      StatObj * stat;
#endif
      std::map<int, node_id> *core_1;
      std::map<int, node_id> *core_2;
      std::map<int, node_id> *in_1;
      std::map<int, node_id> *in_2;
      std::map<int, node_id> *out_1;
      std::map<int, node_id> *out_2;

      Graph *g1, *g2;
      int n1, n2;

	  long *share_count;
    
    public:
      VF2MapSubState(Graph *g1, Graph *g2, bool sortNodes=false);
      VF2MapSubState(const VF2MapSubState &state);
      VF2MapSubState(const VF2MapSubState &state, bool deep);
      ~VF2MapSubState(); 

      Graph *GetGraph1() { return g1; }
      void propogate(const State &state);
      Graph *GetGraph2() { return g2; }
      bool NextPair(node_id *pn1, node_id *pn2,
                    node_id prev_n1=NULL_NODE, node_id prev_n2=NULL_NODE);
      bool IsFeasiblePair(node_id n1, node_id n2);
      void AddPair(node_id n1, node_id n2);
      bool IsGoal() { 
	bool r = ( core_len==n1 ); 
#ifdef TIME_DEB
	if(r){
	  stat->printTime();
	}
#endif
	return r;
      };
      bool IsDead() { 
	return n1>n2  || 
	  t1both_len>t2both_len ||
	  t1out_len>t2out_len ||
	  t1in_len>t2in_len;
      };
      int CoreLen() { return core_len; }
      void GetCoreSet(node_id c1[], node_id c2[]);
      State *Clone();
      State *DeepClone();
      virtual void BackTrack();
  };


#endif

