//CREATED BY: mystical, 2024
/*
This is a search program named multiperiodphotonsearcher v0.2, which can be used to find
photons with arbitrarily large periods. It can be set up to search for a specific period 
(using the feature of forbidden periods) or it can be set up to search all periods less 
than max period.
*/

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


#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <deque>
#include <iostream>
#include <regex>
#include <set>
#include <span>
#include <sstream>
#include <vector>

#include <immintrin.h>

using namespace std;
using ll = long long; 
using pll = pair<ll,ll>;
#define e0 first
#define e1 second
constexpr ll INFTY = 2e18;

// SET PARAMETERS HERE
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    enum {BFS,DFS}; constexpr auto search_style = BFS;
    constexpr int CACHE_TILL_PERIOD = 50; 
    constexpr ll MAX_ITER = INFTY;
    //#define MULTI_RULE
    #define ENABLE_FIND_IMPLICATIONS
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifdef MACRO_OPTIONS
    constexpr int MAX_PERIOD = M_MAX_PERIOD;
    constexpr size_t W = M_W;
    constexpr ll VERBOSITY = M_VERBOSITY;
    constexpr ll MAX_TREE_SIZE = M_MAX_TREE_SIZE;
#else
    constexpr int MAX_PERIOD = 50;
    constexpr size_t W = 14;
    constexpr ll VERBOSITY = 1;
    constexpr ll MAX_TREE_SIZE = 20000000;
    #define EVEN 
#endif

static_assert(W<=16, "Sorry, widths above 16 not supported yet.");

#if defined(MULTI_RULE) && defined(PARALLEL_EVOLVE)
#error "Not possible to do parallel evolution when searching multiple rules"
#endif

using otca_rule = uint32_t;
using expanded_row = uint64_t;

constexpr expanded_row CELL_POSN = (expanded_row(-1)>>(64ull-4ull*W))/0xFull;