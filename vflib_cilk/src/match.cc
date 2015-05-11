/*-------------------------------------------------------
 * match.cc
 * Definition of the match function
 ------------------------------------------------------*/

#include "argraph.h"
#include "match.h"
#include "state.h"
#include "error.h"
#include "stdio.h"

#include <iostream>

#include <cilk/cilk.h>
//#include <cilk/cilkview.h>

//#include <time.h>
#include <sys/types.h>

#define SPAWN_DEPTH 8
#define SPAWN_MULT 4
#define SPAWN_BROAD 4

//static double tdiff(struct timeval* a, struct timeval* b)
//// Effect: Compute the difference of b and a, that is $b-a$
////  as a double with units in seconds
//{
//	return a->tv_sec - b->tv_sec + 1e-6 * (a->tv_usec - b->tv_usec);
//}

Match::Match(State* s0, match_visitor vis, void* usr_data)
{
    Graph* g1 = s0->GetGraph1();
    Graph* g2 = s0->GetGraph2();

    int n;
    if (g1->NodeCount() < g2->NodeCount())
    {
        n = g2->NodeCount();
    }
    else
    {
        n = g1->NodeCount();
    }

    pn = 0;
    c1 = new node_id[n];
    c2 = new node_id[n];
    foundFlg = false;
    this->s0 = s0;
    this->visitor = vis;
    this->usr_data = usr_data;
    InitializeCriticalSection(&this->cs);
}

void Match::match_par()
{
    //timeval start, end;
    //cilk::cilkview cv;

    //gettimeofday(&start, 0);
    //cv.start();
    //match_par_helper(s0, 1);
    match_par_helper_full_spawn(s0, 1, NULL, false);
    //cv.stop();
    //cv.dump("match_performance");
    //gettimeofday(&end, 0);
    //printf("Matched in %f\n", tdiff(&end, &start));

    //match_serial_helper(s0, 1);
}


void Match::match_par_helper(State *s, int ss)
{
    if (s->IsGoal())
    {
        EnterCriticalSection(&this->cs);
        if (foundFlg)
        {
            LeaveCriticalSection(&this->cs);
            return;
        }

        pn = s->CoreLen();
        s->GetCoreSet(c1, c2);
        printf("Found a matching with %d nodes:\n", pn);
        foundFlg = true;
        LeaveCriticalSection(&this->cs);

        return;
    }

    if (s->IsDead())
    {
        return;
    }

    node_id n1 = NULL_NODE, n2 = NULL_NODE;
    while (!(foundFlg)
            && s->NextPair(&n1, &n2, n1, n2))
    {

        if (s->IsFeasiblePair(n1, n2))
        {
            State *s1;
            //printf("%d %u\n",cilk::current_worker_id(),cilk::current_worker_count());
            if (ss % SPAWN_DEPTH < SPAWN_MULT)
                //if(ss < SPAWN_DEPTH)
            {
                s1 = s->DeepClone();
            }
            else
            {
                s1 = s->Clone();
            }
            s1->AddPair(n1, n2);
            if (ss % SPAWN_DEPTH < SPAWN_MULT)
                //if( ss < SPAWN_DEPTH )
            {
                cilk_spawn match_par_helper(s1, ss + 1);
            }
            else
            {
                match_par_helper(s1, ss + 1);
                s1->BackTrack();
                delete s1;
            }
        }
    }

    cilk_sync;
}

void Match::match_serial_helper(State *s, int ss)
{
    if (s->IsGoal())
    {
        EnterCriticalSection(&this->cs);
        if (foundFlg)
        {
            LeaveCriticalSection(&this->cs);
            return;
        }

        pn = s->CoreLen();
        s->GetCoreSet(c1, c2);
        //printf("Found a matching with %d nodes:\n", pn);
        foundFlg = true;
        LeaveCriticalSection(&this->cs);

        return;
    }

    if (s->IsDead())
    {
        return;
    }

    node_id n1 = NULL_NODE, n2 = NULL_NODE;
    while (!(foundFlg)
            && s->NextPair(&n1, &n2, n1, n2))
    {
        if (s->IsFeasiblePair(n1, n2))
        {
            State *s1 = s->Clone();
            s1->AddPair(n1, n2);
            match_serial_helper(s1, ss + 1);
            if (foundFlg)
            {
                return;
            }
            s1->BackTrack();
            delete s1;
        }
    }
}

void Match::match_par_helper_full_spawn(State* s, int ss, bool* flag, bool run_on_clone)
{
    if (s->IsGoal())
    {
        EnterCriticalSection(&this->cs);
        if (foundFlg)
        {
            LeaveCriticalSection(&this->cs);
            return;
        }

        //lock.lock();
        pn = s->CoreLen();
        s->GetCoreSet(c1, c2);
        visitor(pn, c1, c2, usr_data);
        foundFlg = true;
        LeaveCriticalSection(&this->cs);

        return;
    }

    if (s->IsDead())
    {
        return;
    }

    node_id n1 = NULL_NODE, n2 = NULL_NODE;

    State* sc = s->DeepClone();
    bool* run = new bool();
    *run = false;

    int serial = 0;
    while (!(foundFlg)
            &&
            s->NextPair(&n1, &n2, n1, n2))
    {
        if (s->IsFeasiblePair(n1, n2))
        {
            State* s1;
            bool para = *run;

            if (*run)
            {
                s1 = s->DeepClone();
            }
            else
            {
                serial++;
                *run = true;
                s1 = sc->Clone();
            }
            s1->AddPair(n1, n2);
            cilk_spawn match_par_helper_full_spawn(s1, ss + 1, run, !para);
        }
    }
    if (run_on_clone)
    {
        s->BackTrack();
        *flag = false;
    }

    cilk_sync;

    if (ss > 1)
    {
        delete s;
        delete sc;
    }

    delete run;
}

