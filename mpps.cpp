/*
You have to set parameters in common.h
*/

#include "processrule.h"
#include "ds.h"


array<expanded_row,MAX_PERIOD> s1;
array<expanded_row,MAX_PERIOD> s2; 
array<expanded_row,MAX_PERIOD+1> s3;

// evolves a candidate extension (s[3][0]) of the photon for specified number of gens.
// returns false if it fails to work in some way (eg it escapes the bounding box)
bool evolve(int gens)
{
    constexpr expanded_row cellmask = 0xF;

    for (int i = 0; i < gens; i++)
    {
        expanded_row sum = s1[i]+(s1[i]<<4)+(s1[i]>>4)
        +(s2[i]<<4)+(s2[i]>>4)
        +s3[i]+(s3[i]<<4)+(s3[i]>>4)
        #ifdef EVEN
        +((s1[i]+s2[i]+s3[i])&cellmask)
        #endif
        ;

#ifdef PARALLEL_EVOLVE
        s3[i+1] = parallel_evolve(sum, s2[i]);
#else
        s3[i+1]=0;
        for (int j = 0; j < int(W); j++)
        {
            s3[i+1] |= rulelookup(cellmask&(sum>>(j*4)),cellmask&(s2[i]>>(j*4)))<<(j*4);
        }
#endif

        // check if pattern has breached the bounding box
        #ifndef EVEN
        if (0!=rulelookup((s1[i]+s2[i]+s3[i])&cellmask,0)) return false;
        #endif

        if (0!=rulelookup(((s1[i]+s2[i]+s3[i])>>(W*4-4))&cellmask,0)) return false;
    }

    return true;
}

//This function tries to makes inferences about the possible states of cells in the next row.
bool find_implications(int gens)
{
    constexpr expanded_row cellmask = 0xF;

    for (int i = 0; i < gens; i++)
    {
#define P1(x) (x&CELL_POSN)

        expanded_row sum_known = s1[i]+(s1[i]<<4)+(s1[i]>>4)
        +(s2[i]<<4)+(s2[i]>>4)
        +P1(s3[i])+P1(s3[i]<<4)+P1(s3[i]>>4)
        #ifdef EVEN
        +((s1[i]+s2[i]+P1(s3[i]))&cellmask)
        #endif
        ;

#undef P1
#define P1(x) ((x>>1)&CELL_POSN)

        expanded_row sum_unknown = P1(s3[i])+P1(s3[i]<<4)+P1(s3[i]>>4)
        #ifdef EVEN
        +((P1(s3[i]))&cellmask)
        #endif
        ;

#undef P1

        s3[i+1]=0;
        for (int j = 0; j < int(W); j++)
        {
            s3[i+1] |= find_next_implication(
                cellmask&(sum_known>>(j*4)),
                cellmask&(sum_unknown>>(j*4)),
                cellmask&(s2[i]>>(j*4))
            ) <<(j*4);
        }

        #ifndef EVEN //assym untested
        uint64_t borderimpl1 = emptyborder_implication((s1[i]+s2[i])&cellmask);
        if (borderimpl1!=2)
        {
            uint64_t curredgeval = (s3[i])&cellmask;
            if (curredgeval==2)
            {
                s3[i] &= ~(cellmask);
                s3[i] |= borderimpl1;
            }
            else if (curredgeval!=borderimpl1) return false;
        }
        #endif

        uint64_t borderimpl2 = emptyborder_implication(((s1[i]+s2[i])>>(W*4-4))&cellmask);
        if (borderimpl2!=2)
        {
            uint64_t curredgeval = (s3[i]>>(W*4-4))&cellmask;
            if (curredgeval==2)
            {
                s3[i] &= ~(cellmask<<(W*4-4));
                s3[i] |= borderimpl2<<(W*4-4);
            }
            else if (curredgeval!=borderimpl2) return false;
        }
    }

    return true;

}

double bitsused=0;

void tryextend(size_t nodeidx, deque<size_t>& extensions)
{
    node& currnode = tree[nodeidx];
    int P = currnode.period();
    currnode.expand(s1,s2, 0);

    s3[0] = 2*CELL_POSN;
#ifdef ENABLE_FIND_IMPLICATIONS
    
    for (ll numcyc=0; numcyc<3; numcyc++)
    {
        if (!find_implications(P)) return;
        s3[0]=s3[P];
    }

    pll min_unknown_gen = {INFTY,-1};
    for (ll g = 0; g < P; g++)
    {
        min_unknown_gen = min(min_unknown_gen,{popcount<expanded_row>((s3[g]>>1)&CELL_POSN), g});
    }
#else
    pll min_unknown_gen = {W,0};
#endif
    

    expanded_row unkpos = (s3[min_unknown_gen.e1]>>1)&CELL_POSN;
    assert(popcount<expanded_row>(unkpos)==min_unknown_gen.e0);
    expanded_row forcedpos = (s3[min_unknown_gen.e1])&CELL_POSN;
    currnode.expand(s1, s2, min_unknown_gen.e1);
    
    bitsused+=W-min_unknown_gen.e0;

    for (uint64_t i = 0; i < 1ull<<min_unknown_gen.e0; i++)
    {
        s3[0] = forcedpos | _pdep_u64(i,unkpos);
        if (!evolve(MAX_PERIOD-(MAX_PERIOD%P))) continue;

        for (int tryperiod = P; tryperiod <= MAX_PERIOD; tryperiod+=P)
        {
            if (s3[tryperiod]==s3[0])
            {
                for (int p_f : forbidden_periods) 
                {
                    if (tryperiod%p_f==0) goto next_continuation_trial;
                } 
                
                size_t newnode = create_tree_node(s3, tryperiod, nodeidx, tryperiod-min_unknown_gen.e1);

                //Note that the duplicate checking means all but first ship of a certain period are rejected...
                if (tryperiod<=CACHE_TILL_PERIOD)
                {
                    fingerprint f = tree.back().getfingerprint();
                    if (!lowperiodhashes[tryperiod].insert(f).e1)
                    {
                        fingerprintdata.dealloc_last_alloc(f.datasize);
                        rowdata.dealloc_last_alloc(tree.back().size);
                        tree.pop_back();
                        goto next_continuation_trial;
                    }
                }

                extensions.push_back(newnode);
                goto next_continuation_trial;
            }
        }
        next_continuation_trial:; 
    }
}


void rulesearch(ll maxiter, ll minprintperiod)
{
    rowdata.clearmemory();
    fingerprintdata.clearmemory();
    tree.clear();
    lowperiodhashes={};
    bitsused=0;

    s3.fill(0);
    create_tree_node(s3, 1, 0, 0);
    lowperiodhashes[1].insert(tree.back().getfingerprint());

    deque<size_t> extensions;
    extensions.push_back(0);

    ll iter=0;
    ll recorddepth=3;
    auto lastprinttime=chrono::steady_clock::now();
    while (!extensions.empty() && iter++ < maxiter)
    {
        if (tree.size()>MAX_TREE_SIZE) reduce_memory_usage(extensions);

        size_t curr;
        if constexpr (search_style==BFS)
        {
            curr = extensions.front();
            extensions.pop_front();
        }
        else if constexpr (search_style==DFS)
        {
            curr = extensions.back();
            extensions.pop_back();
        }
        else throw runtime_error("Not a valid search style");


        if (VERBOSITY>0 && iter%1000==0) 
        {
            using namespace std::literals::chrono_literals;
            auto currtime= chrono::steady_clock::now();
            if ((currtime-lastprinttime)>10s) 
            {
                int depth = find_depth(curr);
                clog << "\rdepth = " << depth << ", queue = " << extensions.size() << ", tree size = " << tree.size() << ", iter = " << iter << ", bits saved per iter = " << bitsused/iter;
                if (VERBOSITY>1 && recorddepth<depth) 
                {
                    clog << "\n" << node_to_rle(curr,rule_name) << "\n"; 
                    recorddepth=depth; 
                }
                clog.flush();
                lastprinttime=currtime;
            }
        }

        if (curr!=0 && tree[curr].is_terminated())
        {
            if (minprintperiod<=tree[curr].period()) 
            {
                cout << node_to_rle(curr,rule_name);
                cout.flush();
#ifdef ONE_SHIP_PER_RULE
                return;
#endif
            }
        }
        else
        {
            tryextend(curr,extensions);
        }
    }

    if (VERBOSITY>0) 
    {
        clog << "\nFinal tree size = " << tree.size() << ", Average bits saved = " << bitsused/iter << ", Final depth = " << find_depth(tree.size()-1) << endl;
    }
}

void testextend()
{
    tree.clear();
    lowperiodhashes={};
    s3.fill(0);
    create_tree_node(s3, 1, 0, 0);
    s3[0]=1;
    create_tree_node(s3, 1, 0, 0);

    deque<size_t> extensions;
    tryextend(1,extensions);

    for (size_t i = 0; i < tree.size(); i++)
    {
        dbgv(node_to_rle(i,rule_name+"History"));
    }
}

int main(int argc, char *argv[])
{
    cin.tie(nullptr);
    ios_base::sync_with_stdio(false);

    vector<otca_rule> ruleset;
    ll usedargs = 1;
    
    /* Useful to search a whole bunch of rules from stdin */
#ifdef MULTI_RULE
    ll N=0;
    cin >> N;
    for (ll i = 0; i < N; i++)
    {
        string inprule;
        cin >> inprule;
        ruleset.push_back(parse_rule(inprule));
    }

#else
#ifdef PARALLEL_EVOLVE
    ruleset.push_back(parse_rule(pevolve_rule_name));
#else
    if (argc<2) throw runtime_error("too few command line arguments");

    otca_rule minrule = parse_rule(argv[1]);
    usedargs++;

    otca_rule maxrule = minrule;
    if (argc>2 && argv[2][0]!='f')
    {
        maxrule = parse_rule(argv[2]);
        usedargs++;
    } 
    ruleset=getrulespace(minrule,maxrule);
#endif
#endif

    if (argc > usedargs)
    {
        if (argv[usedargs][0]=='f')
        {
            for (int i = usedargs+1; i < argc; i++)
            {
                forbidden_periods.push_back(stoi(argv[i]));
            }
        }
        else
        {
            throw runtime_error("Error while parsing command line arguments");
        }
    }



    auto lastprinttime=chrono::steady_clock::now()-2s;
    for (otca_rule r : ruleset)
    {
        set_rule(r);
        using namespace std::literals::chrono_literals;
        auto currtime= chrono::steady_clock::now();
        if (VERBOSITY>=1 || (VERBOSITY==0&&(currtime-lastprinttime)>5s))
        {
            clog<<"Searching rule " << rule_name << endl;
            lastprinttime=currtime;
        }
#ifdef PARALLEL_EVOLVE
        test_pevo();
#endif
        rulesearch(MAX_ITER,2); 
    }
    cout << endl;
}