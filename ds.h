#include "common.h"
#include "simplealloc.h"
// All the partials found are represented as a large tree
// This does mean that the memory usage keeps steadily increasing over time
// till the memory reduction procedure is activated. This tries to empty the tree as much as possible
struct node;
struct fingerprint;

vector<node> tree;

vector<int> forbidden_periods;

array<set<fingerprint>,CACHE_TILL_PERIOD+1> lowperiodhashes;

simplealloc rowdata;
simplealloc fingerprintdata;

struct fingerprint
{
    size_t dataptr;
    uint32_t datasize;

    fingerprint(size_t bitsize)
    {
        datasize = ((bitsize-1)/16)+1;
        dataptr = fingerprintdata.alloc_size(datasize); 
    }

    // needed for std::set
    strong_ordering operator<=>(const fingerprint& other) const
    {
        return lexicographical_compare_three_way(
            fingerprintdata.data.begin()+dataptr,
            fingerprintdata.data.begin()+dataptr+datasize,
            fingerprintdata.data.begin()+other.dataptr,
            fingerprintdata.data.begin()+other.dataptr+other.datasize
        );
    }

    void or_at(uint16_t d, size_t pos)
    {
        assert((pos/16)<datasize);
        fingerprintdata[dataptr+(pos/16)] |= d << (pos%16);
        if ((pos/16)+1<datasize) 
        {
            fingerprintdata[dataptr+((pos/16)+1)] |= d >> (16-(pos%16));
        }
        else assert((d >> (16-(pos%16))) == 0);
    }
};

// adapted from http://graphics.stanford.edu/~seander/bithacks.html
uint16_t reverselowbits(uint16_t v, size_t num_bits)
{
    // swap odd and even bits
    v = ((v >> 1) & 0x5555) | ((v & 0x5555) << 1);
    // swap consecutive pairs
    v = ((v >> 2) & 0x3333) | ((v & 0x3333) << 2);
    // swap nibbles ... 
    v = ((v >> 4) & 0x0F0F) | ((v & 0x0F0F) << 4);
    // swap bytes
    v = ((v >> 8) & 0x00FF) | ((v & 0x00FF) << 8);
    return v>>(16-num_bits);
}

// adapted from cp-algorithms.com, based on lyndon factorisation
size_t min_cyclic_perm(span<uint32_t> s)
{
    int n = 2*s.size();
    int i = 0, ans = 0;
    while (i < n / 2) {
        ans = i;
        int j = i + 1, k = i;
        while (j < n && s[k%int(s.size())] <= s[j%int(s.size())]) {
            if (s[k%int(s.size())] < s[j%int(s.size())])
                k = i;
            else
                k++;
            j++;
        }
        while (i <= k)
            i += j - k;
    }
    return ans;
}


struct node
{
    size_t parent;
    size_t rowdataptr;
    uint32_t size;

    int period() const
    {
        return size;
    }

    uint16_t& row(size_t idx) const
    {
        assert(idx<size);
        return rowdata[rowdataptr+idx];
    }

    // converts the compressed stored representation (1 bit per cell) into an expanded 
    // form with 4 bits per cell.
    void expand(array<uint64_t,MAX_PERIOD>& r1, array<uint64_t,MAX_PERIOD>& r2, int gen_offset) const
    {
        const node& prevnode = tree[parent];
        for (int i = 0; i < MAX_PERIOD; i++)
        {
            r1[i]=_pdep_u64(uint64_t(prevnode.row((i+gen_offset)%(prevnode.size))),CELL_POSN);
        }

        for (int i = 0; i < MAX_PERIOD; i++)
        {
            r2[i]=_pdep_u64(uint64_t(row((i+gen_offset)%(size))),CELL_POSN);
        }
    }

    bool is_terminated() const
    {
        for (size_t i = 0; i < size; i++)
        {
            if (row(i)!=0) return false;
        }
        for (size_t i = 0; i < tree[parent].size; i++)
        {
            if (tree[parent].row(i)!=0) return false;
        }
        return true;
    }

    // used to generate a unique number for each set of 2 rows to help avoid repeating partials
    fingerprint getfingerprint() const
    {
        const size_t P1 = period();
        const size_t P2 = tree[parent].period();
        const node& prevnode = tree[parent];

        assert(P1>=P2 && P1%P2==0);
        assert(P1<=CACHE_TILL_PERIOD);

        array<uint32_t,MAX_PERIOD> noncanon{};
        for (size_t i = 0; i < P1; i++)
        {
            noncanon[i] = ((uint32_t(prevnode.row(i%P2))) <<16) + uint32_t(row(i)); 
        }

        size_t canonstart = min_cyclic_perm(span<uint32_t>(noncanon.begin(),P1));


#ifndef EVEN
        bool saveflipped=false;

        array<uint32_t,MAX_PERIOD> noncanonflipped{};
        for (size_t i = 0; i < P1; i++)
        {
            noncanonflipped[i] = ((uint32_t(reverselowbits(prevnode.row(i%P2),W))) <<16) 
                            + uint32_t(reverselowbits(row(i),W));
        }
        
        size_t canonstartflip = min_cyclic_perm(span<uint32_t>(noncanonflipped.begin(),P1));

        for (size_t i = 0; i < P1; i++)
        {
            if (noncanon[(canonstart+i)%P1]<noncanonflipped[(canonstartflip+i)%P1])  
            {
                saveflipped=false;
                break;
            } 
            if (noncanon[(canonstart+i)%P1]>noncanonflipped[(canonstartflip+i)%P1])  
            {
                saveflipped=true;
                break;
            } 
        }
#endif

        fingerprint outp(P1*W*2);
        for (size_t i = 0; i < P1; i++)
        {
#ifndef EVEN
            if (saveflipped)
            {
                outp.or_at(reverselowbits(row((i+canonstartflip)%P1),W), i*W*2);
                outp.or_at(reverselowbits(prevnode.row((i+canonstartflip)%P2),W), i*W*2+W);
            }
            else
#endif
            {
                outp.or_at(row((i+canonstart)%P1), i*W*2);
                outp.or_at(prevnode.row((i+canonstart)%P2), i*W*2+W);
            }
        }
        return outp;
    }
};

size_t create_tree_node(array<uint64_t,MAX_PERIOD+1>& rows, int P, size_t parent, int gen_offset)
{
    tree.push_back(node{parent,rowdata.alloc_size(P),uint32_t(P)});
    for (int i = 0; i < P; i++)
    {
        tree.back().row(i) = _pext_u64(rows[(i+gen_offset)%P], CELL_POSN);
    }
    return tree.size()-1;
}

string node_to_rle(size_t nodeidx, string node_rule_name)
{
    ostringstream ss;
    if (tree[nodeidx].is_terminated()) 
    {
        ss << "#C +++++++ p"<< tree[nodeidx].period()<<" ship +++++++\n";
    }
    else 
    {
        ss << "#C ---- p"<<tree[nodeidx].period()<<" partial ----\n";
    }

    ss << "x = 0, y = 0, rule = " << node_rule_name << "\n";
    while (nodeidx!=0)
    {
        uint64_t currrow = tree[nodeidx].row(0);
#ifdef EVEN
        for (ll b = W; b >= 0; b--)
        {
            ss << (((currrow>>b)&1)?'o':'b');
        }
#endif
        for (ll b = 0; b < ll(W); b++)
        {
            ss << (((currrow>>b)&1)?'o':'b');
        }
        nodeidx=tree[nodeidx].parent;
        ss << ((nodeidx==0)?"!\n":"$\n");
    }
    return ss.str();
}

int find_depth(size_t currnode)
{
    int depth=0;
    while (currnode!=0)
    {
        currnode=tree[currnode].parent;
        depth++;
    }
    return depth;
}

struct memiter
{
    ll currtreeidx;
    
    memiter() : currtreeidx(-1) {}

    bool next()
    {
        currtreeidx++;
        return (currtreeidx<ll(tree.size()));
    }

    pair<size_t,size_t> getoldloc() const
    {
        return {tree[currtreeidx].rowdataptr, tree[currtreeidx].size};
    }

    void setnewloc(size_t newloc)
    {
        tree[currtreeidx].rowdataptr=newloc;
    }
};

void reduce_memory_usage(deque<size_t>& queue)
{
    if (VERBOSITY>0)
    {
        clog << "\nTree size limit reached, now trying to remove unused nodes...\n";
    }

    for (size_t i = 0; i < lowperiodhashes.size(); i++)
    {
        lowperiodhashes[i].clear();
    }
    fingerprintdata.clearmemory();
    
    vector<bool> markednodes(tree.size(), false);
    for (size_t i : queue)
    {
        markednodes[i] = true;
    }

    for (ll i = tree.size()-1; i >= 0; i--)
    {
        if (tree[i].is_terminated())
        {
            markednodes[i] = true;
        }
        if (markednodes[i])
        {
            markednodes[tree[i].parent] = true;
        }
    }

    vector<size_t> old_to_new_node(tree.size(), INFTY);
    size_t newnode_count=0;
    for (size_t i = 0; i < tree.size(); i++)
    {
        if (markednodes[i])
        {
            old_to_new_node[i]=newnode_count;
            tree[newnode_count]=tree[i];
            newnode_count++;
        }
    }
    tree.resize(newnode_count);
    tree.shrink_to_fit();

    for (node& n : tree)
    {
        n.parent = old_to_new_node[n.parent];
        assert(n.parent<newnode_count);
    }
    for (size_t& qe : queue)
    {
        qe = old_to_new_node[qe];
        assert(qe<newnode_count);
    }
    old_to_new_node.clear();
    old_to_new_node.shrink_to_fit();
    
    memiter miter;
    rowdata.reallocate<memiter>(miter);

    for (size_t i = 0; i < tree.size(); i++)
    {
        const node& n = tree[i];
        fingerprint f = n.getfingerprint();

        if (!lowperiodhashes[n.period()].insert(f).e1)
        {
            dbgv(n.period());
            dbgv(node_to_rle(i,rule_name+"Super"));
            throw runtime_error("oopsy daisy! there is a bug");
        }
    }
}