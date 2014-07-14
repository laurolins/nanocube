#include "Heap.hh"

//-------------------------------------------------------------------------
// Hash
//-------------------------------------------------------------------------

// HashTable::HashTable()
// {
//
// }
//
// void HashTable::insert(std::string w)
// {
//     std::unordered_map<std::string,int>::iterator found = m.find(w);
//
//     if(found == m.end())
//     {
//         m.emplace(w, 1);
//     }
//     else
//     {
//         found->second = found->second + 1;
//     }
// }


//-------------------------------------------------------------------------
// Heap
//-------------------------------------------------------------------------

Heap::Heap()
{
    state = DIRTY;
}

void Heap::insert(Node n)
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

void Heap::merge(WordsHandler& wh, int s)
{

    Heap& h = (Heap&)wh;

    h.heapify();

    s = std::min(s, (int)h.v.size());
    int i = 0;
    for(i = 0; i < s; i++)
    {
        Node max = h.v[0];
        insert(max);

        std::pop_heap(h.v.begin(), h.v.end()-i, compare_node());
    }

    for(i = 0; i < s; i++)
    {
        std::push_heap(h.v.begin(), h.v.end()-i, compare_node());
    }

    state = DIRTY;
}

void Heap::heapify()
{
    if(state == HEAP)
        return;

    std::make_heap(v.begin(),v.end(), compare_node());
    state = HEAP;
}

void Heap::getTopK(int k)
{

    heapify();

    k = std::min(k, (int)v.size());
    int i = 0;
    for(i = 0; i < k; i++)
    {
        Node max = v[0];
        std::cout << max.word << " " << max.count << std::endl;

        std::pop_heap(v.begin(), v.end()-i, compare_node());
    }

    for(i = 0; i < k; i++)
    {
        std::push_heap(v.begin(), v.end()-i, compare_node());
    }
}