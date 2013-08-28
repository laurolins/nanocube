
print """// File automatically generated
#pragma once

typedef unsigned char NodeKey;
typedef char          ChildIndex;

template <NodeKey K>
struct NodeKeyToNodeType
{};

"""

for key in xrange(16):

    bitset = [((key >> j) & 1) for j in xrange(0,4)]

    type_name    = 'NodeType' + ''.join([str(j) for j in bitset])

    num_children = sum(bitset)
    # print type_name
    # print num_children

    childrenIndices = [-1, -1, -1, -1]
    actualIndices = []
    index = 0
    for j in xrange(4):
        if bitset[j]:
            childrenIndices[j] = index
            index += 1
            actualIndices.append(j)

    childrenIndicesStr = "{" + \
        ", ".join([str(a) for a in childrenIndices]) +  "}";

    actualIndicesStr = "{" + \
        ", ".join([str(a) for a in actualIndices]) +  "}";
    
    print """struct %s {
    enum {key         = %d};
    enum {numChildren = %d};
    static const ChildIndex childrenIndices[];
    static const ChildIndex actualIndices[];
};

const ChildIndex %s::childrenIndices[] = %s;
const ChildIndex %s::actualIndices[] = %s;

template<>
struct NodeKeyToNodeType<%d>
{
    typedef %s type;
};
""" % (type_name, key, num_children, type_name, childrenIndicesStr, type_name, actualIndicesStr, key, type_name)
