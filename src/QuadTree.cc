#include "QuadTree.hh"

namespace quadtree {

    MemUsage::MemUsage():
        totalMem(0),
        totalCount(0)
    {
        memByNumChildren.resize(5, 0);
        countByNumChildren.resize(5, 0);
    }

    MemUsage::~MemUsage()
    {
        // std::cerr << "~MemUsage " << std::endl;
    }

    Count MemUsage::getMemUsage()  const {
        return totalMem;
    }

    Count MemUsage::getMemUsage(NumChildren n)  const {
        return memByNumChildren[n];
    }

    Count MemUsage::getCount() const {
        return totalCount;
    }

    Count MemUsage::getCount(NumChildren n)  const {
        return countByNumChildren[n];
    }



}
