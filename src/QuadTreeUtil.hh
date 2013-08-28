#pragma once

#include <numeric>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>

#include <QuadTree.hh>
#include <Util.hh>

namespace quadtree {

//
// LevelStatistics
//

template <typename Structure>
struct Stats
{
public:

    typedef Structure StructureType;
    typedef typename Structure::NodeType    NodeType;
    typedef typename Structure::AddressType AddressType;
    static const BitSize N = Structure::AddressSize;

public:

    struct LevelStats
    {
        Level level;
        Count numNodes[5]; // by number of children
        LevelStats();
    };

public:

    Stats();

    void initialize(Structure &quadTree);

    void visit(NodeType *node, AddressType &addr);

    Count getNumNodesByLevel(Level level);
    Count getNumNodesByLevelAndNumChildren(Level level, Count numChildren);
    Count getNumNodesByNumChildren(Count numChildren);

    void dumpReport(std::ostream &os);

    LevelStats levels[N + 1];
    Count      totalNodes;
};


//
// Stats::LevelStats
//

template <typename Structure>
Stats<Structure>::LevelStats::LevelStats()
{
    std::fill_n(numNodes, 5, 0);
}


//
// Stats
//

template <typename Structure>
Stats<Structure>::Stats():
    totalNodes(0)
{
    for (Level i=0;i<N+1;i++)
        levels[i].level = i;
}

template <typename Structure>
void Stats<Structure>::initialize(Structure &quadTree)
{
    AddressType addr(0,0,0); // a,a
    quadTree.visitSubnodes(addr, -1, *this);

    // totalNodes = quadTree.getNumNodes();
}

template <typename Structure>
void Stats<Structure>::visit(NodeType *node, AddressType &addr)
{
    levels[addr.level].numNodes[node->getNumChildren()]++;
}

template <typename Structure>
Count Stats<Structure>::getNumNodesByLevel(Level level)
{
    if (level < 0 || level > N)
        return 0;
    Count *ptr = &levels[level].numNodes[0];
    return std::accumulate(ptr, ptr+5, 0);
}

template <typename Structure>
Count Stats<Structure>::getNumNodesByLevelAndNumChildren(Level level, Count numChildren)
{
    if (level < 0 || level > N || numChildren > 4)
        return 0;
    return levels[level].numNodes[numChildren];
}

template <typename Structure>
Count Stats<Structure>::getNumNodesByNumChildren(Count numChildren)
{
    Count accum = 0;
    for (Level i=0;i<N+1;i++)
        accum +=getNumNodesByLevelAndNumChildren(i,numChildren);
    return accum;
}



template <typename Structure>
void
Stats<Structure>::dumpReport(std::ostream &os)
{
    char sep = '|';

    using datatiles::util::fl;
    using datatiles::util::fr;
    using datatiles::util::str;

    os
            << sep
            << fl("Level")      << sep // number of add calls
            << fl("0-Childr.")  << sep // num nodes in quad tree
            << fl("1-Childr.")  << sep //
            << fl("2-Childr.")  << sep //
            << fl("3-Childr.")  << sep
            << fl("4-Childr.")  << sep << std::endl;

    for (Level i=0;i<=N;i++)
    {
        os
                << sep
                << fr(str(i)); // number of add calls
        for (Count j=0;j<5;j++)
            os
                    << sep
                    << fr(str(getNumNodesByLevelAndNumChildren(i,j)));
        os << sep << std::endl;
    }

    os
            << sep
            << fr("Totals"); // number of add calls
    for (Count j=0;j<5;j++)
        os
                << sep
                << fr(str(getNumNodesByNumChildren(j)));
    os << sep << std::endl;

    os
            << sep
            << fr("Tot.(%)"); // number of add calls
    for (Count j=0;j<5;j++)
        os
                << sep
                << fr(str((100 * getNumNodesByNumChildren(j))/(double)totalNodes));
    os << sep << std::endl;


    Count sizes[] =
    {
        sizeof(ScopedNode<int, NodeType0000>),
        sizeof(ScopedNode<int, NodeType1000>),
        sizeof(ScopedNode<int, NodeType1100>),
        sizeof(ScopedNode<int, NodeType1110>),
        sizeof(ScopedNode<int, NodeType1111>)
    };


    Count sum = 0;
    os
            << sep
            << fr("Memory (B)"); // number of add calls
    for (Count j=0;j<5;j++)
    {
        Count mj = getNumNodesByNumChildren(j) * sizes[j];
        sum += mj;
        os
                << sep
                << fr(str(mj));
    }
    os << sep << std::endl;
    os << "Total Nodes: " << this->totalNodes << std::endl;



    os
            << sep
            << fr("Memory (MB)"); // number of add calls
    for (Count j=0;j<5;j++)
        os
                << sep
                << fr(str((Count) ((getNumNodesByNumChildren(j) * sizes[j])/(double) (1 << 20))));
    os << sep << std::endl;

    os
            << sep
            << fr("Tot.Mem. (B)") << sep
            << fr(str(sum)) << sep << std::endl;

    os
            << sep
            << fr("Tot.Mem. (MB)") << sep
            << fr(str((Count)(sum/(double) (1<<20)))) << sep << std::endl;

}






}
