#include "common.h"

otca_rule parse_rule(const string& rule)
{
    otca_rule outp=0;
    if (0==rule.size() || rule[0]!='B') throw runtime_error("rule format error:1");

    ll idx=1;
    while (idx<ll(rule.size()) && '0'<=rule[idx]&&rule[idx]<='8')
    {
        outp |= 1<<(rule[idx]-'0');
        idx++;
    }

    if (idx+1>=ll(rule.size()) || rule[idx]!='/' || rule[idx+1]!='S') throw runtime_error("rule format error:2");
    idx+=2;

    while (idx<ll(rule.size()) && '0'<=rule[idx]&&rule[idx]<='8')
    {
        outp |= 1<<(rule[idx]-'0'+9);
        idx++;
    }

    if (idx!=ll(rule.size())) throw runtime_error("rule format error:3");

    return outp;
}

string int_to_rule(otca_rule rule)
{
    string o="B";
    for (ll i = 0; i < 9; i++)
    {
        if ((rule>>i)&1) {
            o+= i+'0';
        }
    }
    o+="/S";
    for (ll i = 0; i < 9; i++)
    {
        if ((rule>>(i+9))&1) {
            o+= i+'0';
        }
    }
    return o;
}

vector<otca_rule> getrulespace(otca_rule minrule, otca_rule maxrule)
{
    if ((minrule&~maxrule)!=0) throw runtime_error("invalid rulespace");

    otca_rule variabletrans = minrule^maxrule;
    ll numtrans = __builtin_popcount(variabletrans);
    vector<otca_rule> outp(1<<numtrans,0);
    for (otca_rule i = 0; i < (1ull<<numtrans); i++)
    {
        outp[i]= minrule|_pdep_u32(i, variabletrans);
    }
    return outp;
}

array<bool,18> birth_survival;
string rule_name;
otca_rule rule_int;

void set_rule(otca_rule rule)
{
    birth_survival.fill(0);
    for (ll i = 0; i < 18; i++)
    {
        if ((rule>>i)&1) {
            birth_survival[i]=1;
        }
    }
    rule_name=int_to_rule(rule);
    rule_int=rule;
}

uint64_t rulelookup(uint64_t s, uint64_t v)
{
    return birth_survival[s+v*9];
}


#ifdef PARALLEL_EVOLVE
#include "pevo.h"

void test_pevo()
{
    for (uint64_t v = 0; v <= 1; v++)
    {
        for (uint64_t s = 0; s <= 8; s++)
        {
            for (uint64_t sh = 0; sh < W; sh++)
            {
                if (rulelookup(s,v)!=(parallel_evolve(s<<(4*sh),v<<(4*sh))>>(4*sh)))
                {
                    throw runtime_error("parallel evolve not tuned for current rule. Error using arguments: " + to_string(s) + "," + to_string(v) + "," + to_string(sh));
                }
            }
        }
    }
    
}

#endif



//TODO: test this
uint64_t find_next_implication(uint64_t smin, uint64_t sunknown, uint64_t v)
{
    assert(smin+sunknown<=8 && v<=2);
    const uint64_t poss_outcome_mask = ((1ull<<(sunknown+1))-1);
    if (v==2)
    {
        uint64_t poss0 = (rule_int>>(smin+0*9)) & poss_outcome_mask;
        uint64_t poss1 = (rule_int>>(smin+1*9)) & poss_outcome_mask;

        if (poss0==poss_outcome_mask && poss1==poss_outcome_mask) return 1;

        if (poss0==0 && poss1==0) return 0;
    }
    else 
    {
        uint64_t poss = (rule_int>>(smin+v*9)) & poss_outcome_mask;

        if (poss==poss_outcome_mask) return 1;

        if (poss==0) return 0;
    }
    return 2;
}// much more advanced search space pruning can be done. this is pretty rudimentry and doesn't even depend on period..

// In any genration of any b2 ship of certain width, any two on cells in the rightmost 
// or leftmost column must have at least 2 cells in between.
// Otherwise, the ship will escape its bounds due to b2a or b2c
uint64_t emptyborder_implication(uint64_t numcornercell)
{
    assert(numcornercell<=2);
    if (birth_survival[2])
    {
        if (numcornercell==0) return 2;
        else {
            return 0;

        }
    }
    return 2;
}

