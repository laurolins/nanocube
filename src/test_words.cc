
#include <chrono>
#include <stdio.h>
#include <string.h>

#include "PartialSort.hh"
#include "Heap.hh"
#include "WordsCommon.hh"

std::chrono::time_point<std::chrono::high_resolution_clock> t0;
std::chrono::time_point<std::chrono::high_resolution_clock> t1;

void start_clock()
{
    t0 = std::chrono::high_resolution_clock::now();
}

uint64_t stop_clock()
{
    t1 = std::chrono::high_resolution_clock::now();
    uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
    return elapsed_nanoseconds;
}

void words_handler(int num_wh, WordsHandler** wh)
{
    std::cout << "Starting" << std::endl;


    int count = 0;
    int current_wh = 0;

    start_clock();
    while(true)
    {
        std::string word;
        getline(std::cin, word);
        if (word.length() == 0)
            break;

        //std::cout << word << " to " << current_ps << std::endl;

        current_wh++;
        if(current_wh >= num_wh)
        {
            current_wh = 0;
        }

        wh[current_wh]->insert(Node(word));
    }
    std::cout << "Insertion time: " << stop_clock() << std::endl;


    start_clock();
    wh[0]->getTopK(1);
    std::cout << "Get top 1 time: " << stop_clock() << std::endl;

    start_clock();
    wh[1]->getTopK(1);
    std::cout << "Get top 1 time: " << stop_clock() << std::endl;

    start_clock();
    wh[0]->merge(*wh[1], 99);
    std::cout << "Merge 99 time: " << stop_clock() << std::endl;

    start_clock();
    wh[0]->getTopK(1);
    std::cout << "Get top 1 time: " << stop_clock() << std::endl;

    std::cout << "Finished" << std::endl;
}

int main(int argc, char **argv)
{

    if(argc <= 1)
    {
        std::cout << "Usage: ./test-words [ps,h]" << std::endl;
        exit(0);
    }

    if(strcmp(argv[1], "ps") == 0)
    {
        PartialSort** ps = new PartialSort*[2];
        ps[0] = new PartialSort();
        ps[1] = new PartialSort();
        words_handler(2, (WordsHandler**)ps);
        delete ps;
    }
    else if(strcmp(argv[1], "h") == 0)
    {
        Heap** h = new Heap*[2];
        h[0] = new Heap();
        h[1] = new Heap();
        words_handler(2, (WordsHandler**)h);
        delete h;
    }
}