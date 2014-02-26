#include "QuadTreeNode.hh"

namespace quadtree
{

ChildName childNameToEntryIndex[16][4] = {
    {-1, -1, -1, -1},
    { 0, -1, -1, -1},
    {-1,  0, -1, -1},
    { 0,  1, -1, -1},
    {-1, -1,  0, -1},
    { 0, -1,  1, -1},
    {-1,  0,  1, -1},
    { 0,  1,  2, -1},
    {-1, -1, -1,  0},
    { 0, -1, -1,  1},
    {-1,  0, -1,  1},
    { 0,  1, -1,  2},
    {-1, -1,  0,  1},
    { 0, -1,  1,  2},
    {-1,  0,  1,  2},
    { 0,  1,  2,  3}
};

ChildName childEntryIndexToName[16][4] = {
    {-1, -1, -1, -1},
    { 0, -1, -1, -1},
    { 1, -1, -1, -1},
    { 0,  1, -1, -1},
    { 2, -1, -1, -1},
    { 0,  2, -1, -1},
    { 1,  2, -1, -1},
    { 0,  1,  2, -1},
    { 3, -1, -1, -1},
    { 0,  3, -1, -1},
    { 1,  3, -1, -1},
    { 0,  1,  3, -1},
    { 2,  3, -1, -1},
    { 0,  2,  3, -1},
    { 1,  2,  3, -1},
    { 0,  1,  2,  3}
};

const ChildName NodeType0000::childrenIndices[] = {-1, -1, -1, -1};
const ChildName NodeType0000::actualIndices[] = {};
const ChildName NodeType1000::childrenIndices[] = {0, -1, -1, -1};
const ChildName NodeType1000::actualIndices[] = {0};
const ChildName NodeType0100::childrenIndices[] = {-1, 0, -1, -1};
const ChildName NodeType0100::actualIndices[] = {1};
const ChildName NodeType1100::childrenIndices[] = {0, 1, -1, -1};
const ChildName NodeType1100::actualIndices[] = {0, 1};
const ChildName NodeType0010::childrenIndices[] = {-1, -1, 0, -1};
const ChildName NodeType0010::actualIndices[] = {2};
const ChildName NodeType1010::childrenIndices[] = {0, -1, 1, -1};
const ChildName NodeType1010::actualIndices[] = {0, 2};
const ChildName NodeType0110::childrenIndices[] = {-1, 0, 1, -1};
const ChildName NodeType0110::actualIndices[] = {1, 2};
const ChildName NodeType1110::childrenIndices[] = {0, 1, 2, -1};
const ChildName NodeType1110::actualIndices[] = {0, 1, 2};
const ChildName NodeType0001::childrenIndices[] = {-1, -1, -1, 0};
const ChildName NodeType0001::actualIndices[] = {3};
const ChildName NodeType1001::childrenIndices[] = {0, -1, -1, 1};
const ChildName NodeType1001::actualIndices[] = {0, 3};
const ChildName NodeType0101::childrenIndices[] = {-1, 0, -1, 1};
const ChildName NodeType0101::actualIndices[] = {1, 3};
const ChildName NodeType1101::childrenIndices[] = {0, 1, -1, 2};
const ChildName NodeType1101::actualIndices[] = {0, 1, 3};
const ChildName NodeType0011::childrenIndices[] = {-1, -1, 0, 1};
const ChildName NodeType0011::actualIndices[] = {2, 3};
const ChildName NodeType1011::childrenIndices[] = {0, -1, 1, 2};
const ChildName NodeType1011::actualIndices[] = {0, 2, 3};
const ChildName NodeType0111::childrenIndices[] = {-1, 0, 1, 2};
const ChildName NodeType0111::actualIndices[] = {1, 2, 3};
const ChildName NodeType1111::childrenIndices[] = {0, 1, 2, 3};
const ChildName NodeType1111::actualIndices[] = {0, 1, 2, 3};

} // end namespace quadtree
