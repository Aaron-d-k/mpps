#include "common.h"


struct simplealloc
{
    vector<uint16_t> data;

    size_t alloc_size(size_t s)
    {
        data.insert(data.end(), s, 0);
        return data.size()-s;
    }

    void dealloc_last_alloc(size_t s)
    {
        data.resize(data.size()-s);
    }

    uint16_t& operator[](size_t idx)
    {
        return data[idx];
    }

    template<class MemLocIter>
    void reallocate(MemLocIter& tosave)
    {
        size_t curridx = 0;
        while (tosave.next())
        {
            pair<size_t,size_t> curr = tosave.getoldloc();

            assert(curridx<=curr.e0);

            for (size_t i = 0; i < curr.e1; i++)
            {
                data[curridx+i] = data[curr.e0+i];
            }

            tosave.setnewloc(curridx);
            curridx+=curr.e1;
        }
        data.resize(curridx);
        data.shrink_to_fit();
    }

    void clearmemory()
    {
        data.clear();
        data.shrink_to_fit();
    }
};