/*------------------------------------------------------------------
 * match.h
 * Header of match.cc
 * Declaration of the match function
 *
 * Author: P. Foggia
 * $Id: match.h,v 1.1 1998/09/29 09:49:48 foggia Exp $
 *-----------------------------------------------------------------*/


/*-----------------------------------------------------------------
 * REVISION HISTORY
 *   $Log: match.h,v $
 *   Revision 1.1  1998/09/29 09:49:48  foggia
 *   Initial revision
 *
 *----------------------------------------------------------------*/


#ifndef MATCH_H
#define MATCH_H

#include "argraph.h"
#include "state.h"
//#include <cilk/cilk_mutex.h>
#include <concrt.h>
#include <windows.h>
#include "../../common/lock.h"

/*------------------------------------------------------------
 * Definition of the match_visitor type
 * a match visitor is a function that is invoked for
 * each match that has been found.
 * If the function returns false, then the next match is
 * searched; else the seach process terminates.
 -----------------------------------------------------------*/

typedef bool(*match_visitor)(int n, node_id c1[], node_id c2[],
                             void* usr_data);

class Match
{
public:
    State* s0;
    node_id* c1, *c2;
    int pn;
    bool foundFlg;
    Match(State* s0, match_visitor vis, void* usr_data = NULL);
    void match_par();
    void match_serial();
private:
    //Concurrency::critical_section lock;
    CRITICAL_SECTION cs;
    void match_par_helper(State *s, int ss);
    void match_serial_helper(State *s, int ss);
    void match_par_helper_full_spawn(State* s, int ss, bool* flag, bool run_on_clone);
    match_visitor visitor;
    void* usr_data;
};
#endif
