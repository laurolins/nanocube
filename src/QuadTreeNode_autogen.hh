template <NodeKey K>
struct NodeKeyToNodeType
{};


struct NodeType0000 {
    enum {key         = 0};
    enum {numChildren = 0};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<0>
{
    typedef NodeType0000 type;
};

struct NodeType1000 {
    enum {key         = 1};
    enum {numChildren = 1};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<1>
{
    typedef NodeType1000 type;
};

struct NodeType0100 {
    enum {key         = 2};
    enum {numChildren = 1};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<2>
{
    typedef NodeType0100 type;
};

struct NodeType1100 {
    enum {key         = 3};
    enum {numChildren = 2};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<3>
{
    typedef NodeType1100 type;
};

struct NodeType0010 {
    enum {key         = 4};
    enum {numChildren = 1};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<4>
{
    typedef NodeType0010 type;
};

struct NodeType1010 {
    enum {key         = 5};
    enum {numChildren = 2};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<5>
{
    typedef NodeType1010 type;
};

struct NodeType0110 {
    enum {key         = 6};
    enum {numChildren = 2};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<6>
{
    typedef NodeType0110 type;
};

struct NodeType1110 {
    enum {key         = 7};
    enum {numChildren = 3};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<7>
{
    typedef NodeType1110 type;
};

struct NodeType0001 {
    enum {key         = 8};
    enum {numChildren = 1};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<8>
{
    typedef NodeType0001 type;
};

struct NodeType1001 {
    enum {key         = 9};
    enum {numChildren = 2};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<9>
{
    typedef NodeType1001 type;
};

struct NodeType0101 {
    enum {key         = 10};
    enum {numChildren = 2};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<10>
{
    typedef NodeType0101 type;
};

struct NodeType1101 {
    enum {key         = 11};
    enum {numChildren = 3};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<11>
{
    typedef NodeType1101 type;
};

struct NodeType0011 {
    enum {key         = 12};
    enum {numChildren = 2};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<12>
{
    typedef NodeType0011 type;
};

struct NodeType1011 {
    enum {key         = 13};
    enum {numChildren = 3};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<13>
{
    typedef NodeType1011 type;
};

struct NodeType0111 {
    enum {key         = 14};
    enum {numChildren = 3};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<14>
{
    typedef NodeType0111 type;
};

struct NodeType1111 {
    enum {key         = 15};
    enum {numChildren = 4};
    static const ChildName childrenIndices[];
    static const ChildName actualIndices[];
};


template<>
struct NodeKeyToNodeType<15>
{
    typedef NodeType1111 type;
};

