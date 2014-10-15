#include "PartialSort.hh"



//-------------------------------------------------------------------------
// PartialSort
//-------------------------------------------------------------------------

PartialSort::PartialSort()
{
    state = DIRTY;
}

void PartialSort::insert(Node n)
{

    //Use hashstring or gperf?
    //see:
    //http://www.gnu.org/software/gperf/
    //http://www.strchr.com/hash_functions
    //http://stackoverflow.com/questions/7700400/whats-a-good-hash-function-for-english-words

    state = DIRTY;

    const int size = v.size();
    int i = 0;
    for(i = 0; i < size; ++i)
    {
        if(v[i].word == n.word)
        {
            v[i].count+=n.count;
            return;
        }
    }

    v.push_back(n);
}

void PartialSort::merge(WordsHandler& wh, int s)
{
    PartialSort& h = (PartialSort&)wh;
    h.sort(s);

    s = std::min(s, (int)h.v.size());
    int i = 0;
    for(i = 0; i < s; i++)
    {
        insert(h.v[i]);
    }

    state = DIRTY;
}

void PartialSort::sort(int k)
{
    if(state == SORTED && k <= sorted_up_to)
        return;

    k = std::min(k, (int)v.size());

    std::partial_sort(v.begin(), v.begin()+k, v.end(), compare_node());
    state = SORTED;
    sorted_up_to = k;
}

void PartialSort::getTopK(int k)
{
    sort(k);

    k = std::min(k, (int)v.size());
    int i = 0;
    for(i = 0; i < k; i++)
    {
        std::cout << v[i].word << " " << v[i].count << std::endl;
    }
}