#include <QuadTreeNode.hh>

namespace quadtree
{

ChildIndex nodeChildrenIndices[16][4] = {
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

ChildIndex nodeActualIndices[16][4] = {
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

const ChildIndex NodeType0000::childrenIndices[] = {-1, -1, -1, -1};
const ChildIndex NodeType0000::actualIndices[] = {};
const ChildIndex NodeType1000::childrenIndices[] = {0, -1, -1, -1};
const ChildIndex NodeType1000::actualIndices[] = {0};
const ChildIndex NodeType0100::childrenIndices[] = {-1, 0, -1, -1};
const ChildIndex NodeType0100::actualIndices[] = {1};
const ChildIndex NodeType1100::childrenIndices[] = {0, 1, -1, -1};
const ChildIndex NodeType1100::actualIndices[] = {0, 1};
const ChildIndex NodeType0010::childrenIndices[] = {-1, -1, 0, -1};
const ChildIndex NodeType0010::actualIndices[] = {2};
const ChildIndex NodeType1010::childrenIndices[] = {0, -1, 1, -1};
const ChildIndex NodeType1010::actualIndices[] = {0, 2};
const ChildIndex NodeType0110::childrenIndices[] = {-1, 0, 1, -1};
const ChildIndex NodeType0110::actualIndices[] = {1, 2};
const ChildIndex NodeType1110::childrenIndices[] = {0, 1, 2, -1};
const ChildIndex NodeType1110::actualIndices[] = {0, 1, 2};
const ChildIndex NodeType0001::childrenIndices[] = {-1, -1, -1, 0};
const ChildIndex NodeType0001::actualIndices[] = {3};
const ChildIndex NodeType1001::childrenIndices[] = {0, -1, -1, 1};
const ChildIndex NodeType1001::actualIndices[] = {0, 3};
const ChildIndex NodeType0101::childrenIndices[] = {-1, 0, -1, 1};
const ChildIndex NodeType0101::actualIndices[] = {1, 3};
const ChildIndex NodeType1101::childrenIndices[] = {0, 1, -1, 2};
const ChildIndex NodeType1101::actualIndices[] = {0, 1, 3};
const ChildIndex NodeType0011::childrenIndices[] = {-1, -1, 0, 1};
const ChildIndex NodeType0011::actualIndices[] = {2, 3};
const ChildIndex NodeType1011::childrenIndices[] = {0, -1, 1, 2};
const ChildIndex NodeType1011::actualIndices[] = {0, 2, 3};
const ChildIndex NodeType0111::childrenIndices[] = {-1, 0, 1, 2};
const ChildIndex NodeType0111::actualIndices[] = {1, 2, 3};
const ChildIndex NodeType1111::childrenIndices[] = {0, 1, 2, 3};
const ChildIndex NodeType1111::actualIndices[] = {0, 1, 2, 3};

} // end namespace quadtree
