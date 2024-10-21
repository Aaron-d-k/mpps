//CREATED BY: mystical, 2024
/*
This is a search program named multiperiodphotonsearcher v0.2, which can be used to find
photons with arbitrarily large periods. It can be set up to search for a specific period 
(using the feature of forbidden periods) or it can be set up to search all periods less 
than max period. Interestingly, the time taken to search should only be linear in max period. 

The original idea used here is similar to that of photonsrc by LaundryPizza03 (https://conwaylife.com/forums/viewtopic.php?f=9&t=4649),
but I came up with most of the ideas used in this program independently.

The code is slightly user unfriendly for now, and you'll have to set some variables like max period 
& width in the source code itself. some parameters like forbidden periods and the rulespace can 
be specified as command line arguments.

You can compile this program on linux it using the command 

    >>> g++ mpps.cpp -O3 -march=native -Wall -Wextra -std=c++20 -o mpps

and run it using (example search in live free or die with forbidden period 6)

    >>> ./mpps B2/S0 f 6

Certain parameters need to be set directly in the part of the code where it says 'SET PARAMETERS HERE' 
You can also use the program to bulk search rules by feeding them in through stdin and enabling
MULTI_RULE

*/

// see more of my ca related programs at https://github.com/Aaron-d-k
// This program is not yet on my github, but I will put it there when I get the time. 
//TODO: period reduction checking

#pragma once

#define NDEBUG //NOT IN SLOW DEBUG MODE RN

#ifdef NDEBUG
#define dbg(TXTMSG) ((void)0)
#define dbgv(VARN) ((void)0)
#define dbgfor(COND) if constexpr (false) for (COND)

#else
#define _GLIBCXX_DEBUG 1
#define _GLIBCXX_DEBUG_PEDANTIC 1
#pragma GCC optimize("trapv")
#define dbg(TXTMSG) cerr << "\n" << TXTMSG
#define dbgv(VARN) cerr << "\n" << #VARN << " = "<< VARN << ", line " << __LINE__ << "\n"
#define dbgfor(COND) for (COND)

#endif


#include <bits/stdc++.h>
#include <immintrin.h>

using namespace std;
using ll = long long; 
using pll = pair<ll,ll>;
#define e0 first
#define e1 second
constexpr ll INFTY = 2e18;

// SET PARAMETERS HERE
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    constexpr int MAX_PERIOD = 50;
    constexpr size_t W = 14;
    constexpr ll MAX_ITER = INFTY;
    constexpr ll VERBOSITY = 1;// can be -1, 0, 1 or 2
    #define EVEN // for even symmetric searches
    enum {BFS,DFS}; constexpr auto search_style = BFS; // bfs is usually the best
    constexpr int CACHE_TILL_PERIOD = 50; // keeping this very high doesn't impact performance much
    constexpr ll MAX_TREE_SIZE = 20000000;
    //#define MULTI_RULE
    //#define PARALLEL_EVOLVE //enable this only after using genrulecode 
    #define ENABLE_FIND_IMPLICATIONS //disabling this will just make it slower
    //#define ONE_SHIP_PER_RULE
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

static_assert(W<=16, "Sorry, widths above 16 not supported yet.");

#if defined(MULTI_RULE) && defined(PARALLEL_EVOLVE)
#error "Not possible to do parallel evolution when searching multiple rules"
#endif

using otca_rule = uint32_t;
using expanded_row = uint64_t;

constexpr expanded_row CELL_POSN = (expanded_row(-1)>>(64ull-4ull*W))/0xFull;