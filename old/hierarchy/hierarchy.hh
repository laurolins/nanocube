#pragma once

#include "common.hh"
#include "slab_allocator.hh"
#include "ptr.hh"

namespace hierarchy {

    //------------------------------------------------------------------------------
    // Forward Declarations
    //------------------------------------------------------------------------------
    
    struct RawNode;

    //------------------------------------------------------------------------------
    // Tied to the slab allocator
    //------------------------------------------------------------------------------
    
    using alloc::slab::Allocator;
    using alloc::slab::Cache;
    
    //------------------------------------------------------------------------------
    // Type Aliaseses
    //------------------------------------------------------------------------------
    
    using PathIndex      = unsigned char;
    using PathLength     = unsigned char;
    using ChildrenLength = unsigned char;
    using ChildrenIndex  = unsigned char;
    using Label          = unsigned char;
    using Header         = char;
    using NumBits        = unsigned char;
    using SizeClass      = unsigned int;
    using NumBytes       = unsigned int;
    using NumNodes       = uint64_t;
    
    static const PathLength     MAX_PATH_LENGTH     = 255;
    static const ChildrenLength MAX_CHILDREN_LENGTH = 255;
    
    using RawNodeOffsetPtr  = ptr::Ptr<RawNode>;
    
    //------------------------------------------------------------------------------
    // Assume a rigid capacity growth structure
    //------------------------------------------------------------------------------
    
    
    //------------------------------------------------------------------------------
    // Capacity from current size
    //------------------------------------------------------------------------------
    
    
    //------------------------------------------------------------------------------
    // Path
    //------------------------------------------------------------------------------

    //
    // Wraps a region reserved for a path with the functionality needed.
    // Desgined to be used shortly every time there is a need to manipulate
    // paths (query or update)
    //
    
    struct Path {
    public:
        Path() {};
        Path(Label* begin, PathLength length, PathLength capacity, NumBits bits);

        void copy(const Path& other, PathLength offset, PathLength length);
        
        inline void copy(const Path& other) { copy(other,0,other.length()); }
        
        Label      get(PathIndex i) const;
        void       set(PathIndex i, Label label);
        
        void       clear();
        
        PathLength length() const;
        PathLength capacity() const;
        
        NumBytes   bytes() const;

    public:
        Label*     _begin { nullptr }; // this is a compressed vector
        PathLength _length { 0 };
        PathLength _capacity { 0 };
        NumBits    _bits_per_label { 0 };
    };

    //------------------------------------------------------------------------------
    // Children
    //------------------------------------------------------------------------------
    
    //
    // Wraps a region reserved for a children node pointer with the functionality
    // needed. Desgined to be used shortly every time there is a need to manipulate
    // children (query or update)
    //
    
    struct Children {
    public:
        Children() {}
        Children(RawNodeOffsetPtr* begin, ChildrenLength length, ChildrenLength capacity);
        
        void copy(Children& other, ChildrenLength offset, ChildrenLength length);

        inline void copy(Children& other) { copy(other,0,other.length()); }

        const RawNode*  get(ChildrenIndex i) const;
        
        RawNode*  get(ChildrenIndex i);
        
        void   set(ChildrenIndex i, RawNode* child);
        
        void append_and_rotate(RawNode *new_child, ChildrenLength index);
        
        void   clear();
        
        inline ChildrenLength length() const { return _length; }
        inline ChildrenLength capacity() const { return _capacity; }
        inline ChildrenLength available_capacity() const { return _capacity - _length; }
        
        RawNode*   child_by_label(Label label);
        
        NumBytes   bytes() const { return (NumBytes)(_capacity * sizeof(RawNodeOffsetPtr)); };
        
    public:
        ChildrenLength _length { 0 };
        ChildrenLength _capacity { 0 };
        RawNodeOffsetPtr* _begin { nullptr };
    };
    

    //------------------------------------------------------------------------------
    // NodeLayout
    //------------------------------------------------------------------------------
    
    struct RawNodeLayout {
    public:
        RawNodeLayout(const RawNode* node, NumBits bits_per_label);
        RawNodeLayout(PathLength path_length, ChildrenLength children_length, NumBits bits_per_label);
    public:
        PathLength     path_capacity;
        ChildrenLength children_capacity;
        NumBits        bits_per_label;
        NumBytes       path_offset;
        NumBytes       children_offset;
        SizeClass      total_size;
    };
    
    //------------------------------------------------------------------------------
    // RawNode
    //------------------------------------------------------------------------------
    
    struct RawNode {
    public:
        RawNode(PathLength path_length, ChildrenLength children_length):
            _path_length(path_length), _children_length(children_length)
        {}
        
        PathLength     path_length()     const { return _path_length;     }
        ChildrenLength children_length() const { return _children_length; }
        
        PathLength     _path_length;
        ChildrenLength _children_length;
    };
    
    //------------------------------------------------------------------------------
    // Node
    //------------------------------------------------------------------------------

    struct Node {
    public:
        Node() {};
        Node(RawNode *raw_node, const RawNodeLayout& layout);
    public:
        
        const Path& path() const { return _path; }
        Path& path() { return _path; }
        
        const Children children() const { return _children; }
        Children& children(){ return _children; }
        
        const RawNode* raw_node() const { return _raw_node; }
        RawNode* raw_node() { return _raw_node; }
        
        SizeClass size_class() const { return _size_class; }
        
        inline ChildrenLength degree() const { return _children.length(); }

        inline PathLength path_length() const { return _path.length(); }

    public:
        RawNode  *_raw_node { nullptr };
        Path      _path;
        Children  _children;
        SizeClass _size_class;
    };
    
    //
    // By our definition that a hierarchy can be at most 127 levels deep
    // and the degree of a node can have at most 255 children, the maximum
    // size of a Node is: 2 bytes of control + 255 * 8 bytes + 127 * 1 bytes.
    // A total of 2169 bytes wide.
    //

    //------------------------------------------------------------------------------
    // Labels
    //------------------------------------------------------------------------------

    template <typename T, typename S>
    struct ReadArray {
        ReadArray(const T* begin, S length): _begin(begin), _length(length) {}
        ReadArray drop_prefix(S n) const { return ReadArray(_begin + n, _length - n); }
        inline const T& get(S i) const { return *(_begin + i); }
        inline S  length() const { return _length; }
        const T*  _begin;
        S         _length;
    };
    using LabelArray = ReadArray<Label, PathLength>;
    
    //------------------------------------------------------------------------------
    // Caches
    //------------------------------------------------------------------------------
    
    //
    // will keep it simple: a Node when created has an exact size that is
    // rounded up to some levels. Which cache we use is dependent on that size
    //
    
    struct Caches {
    public:
        static const SizeClass CLASSES[]; /*{
            8, 12, 16, 24, 32, 48, 64, 96,
            128, 192, 256, 384, 512, 768, 1024, 1536,
            2048, 3072, 4096
        };*/
        static const unsigned int NUM_CLASSES;      // 19;
        static const unsigned int MAX_CLASSES = 30; //
    public:
        Caches(Allocator& allocator);
        void* alloc(SizeClass num_bytes);
        void  free(SizeClass num_bytes, void *p);
    public:
        ptr::Ptr<Cache> _caches[MAX_CLASSES];
    };

    //-------------------------------------------------------------------------------
    // Position
    //-------------------------------------------------------------------------------
    
    
    //
    // Position is the result of a "find" method call.
    //
    struct Position {
    public:
        
        Position() = default;
        
        Position(RawNode *parent,
                 RawNode *child,
                 PathLength distance_to_parent,
                 PathLength distance_to_root,
                 ChildrenIndex parent_child_index,
                 ChildrenIndex child_newchild_index):
        _parent(parent),
        _child(child),
        _distance_to_parent(distance_to_parent),
        _distance_to_root(distance_to_root),
        _parent_child_index(parent_child_index),
        _child_newchild_index(child_newchild_index)
        {}

        RawNode* parent() { return _parent; }
        RawNode* child () { return _child;  }
        
        PathLength distance_to_parent() const { return _distance_to_parent; }
        PathLength distance_to_root()   const { return _distance_to_root;   }
        
        ChildrenIndex parent_child_index()   const { return _parent_child_index;   }
        ChildrenIndex child_newchild_index() const { return _child_newchild_index; }
        
        void parent_child_index(ChildrenIndex i)   { _parent_child_index = i; }
        void child_newchild_index(ChildrenIndex i) { _child_newchild_index = i; }
        
    public:
        RawNode*      _parent               { nullptr };
        RawNode*      _child                { nullptr };
        PathLength    _distance_to_parent   { 0u };
        PathLength    _distance_to_root     { 0u };
        ChildrenIndex _parent_child_index   { 0u };
        ChildrenIndex _child_newchild_index { 0u };
    };

    //-------------------------------------------------------------------------------
    // Hierarchy
    //-------------------------------------------------------------------------------

    //
    // Hierarchy wraps a single node as the root of a hierarchy.
    // This initial node might be null indicating an initally empty
    // hierarchy. The hierarchy must also have access to the
    // allocator objects to reserve memory for new paths and updates.
    //
    struct Hierarchy {
    public:
        struct Stats {
        public:
            Stats();
        public:
            NumNodes   length_sum() const { return _length_sum; }
            NumNodes   nodes()      const { return _nodes;      }
            PathLength levels()     const { return _levels;     }
            PathLength depth()      const { return _depth;      }
            uint64_t   bytes()      const { return _bytes;      }
            NumNodes   nodes_per_level(PathLength level) const { return _nodes_per_level[level]; }
            NumNodes   nodes_per_depth(PathLength depth) const { return _nodes_per_depth[depth]; }
            NumNodes   nodes_per_degree(ChildrenLength degree) const { return _nodes_per_degree[degree]; }
            NumNodes   nodes_per_path_length(PathLength length) const { return _nodes_per_path_length[length]; }
            void insert(PathLength level,
                        PathLength depth,
                        PathLength path_length,
                        ChildrenLength degree,
                        uint64_t bytes);
        public:
            NumNodes        _length_sum { 0 };
            uint64_t        _bytes   { 0 };
            NumNodes        _nodes   { 0 };
            PathLength      _levels  { 0 };
            PathLength      _depth   { 0 };
            NumNodes        _nodes_per_level[MAX_PATH_LENGTH];
            NumNodes        _nodes_per_depth[MAX_PATH_LENGTH];
            NumNodes        _nodes_per_path_length[MAX_PATH_LENGTH];
            ChildrenLength  _nodes_per_degree[MAX_CHILDREN_LENGTH];
        };
        
        
    public:
        
        Hierarchy(Allocator& allocator, NumBits bits_per_label);
        
        Node allocate_node(PathLength path_length, ChildrenLength children_length, bool clear=true); // it is unitialized

        void free_node(Node& node);
        
        // should use the span template from the C++ guidelines
        void insert(const LabelArray& labels);

        Position find(const LabelArray& labels);
        
        RawNode* root() { return _root.raw_null(); }
        
        const RawNode* root() const { return _root.raw_null(); }
        
        NumBits bits_per_label() const { return _bits_per_label; }
        
        inline Node to_node(RawNode* raw_node) { return Node(raw_node, raw_node_layout(raw_node)); }

        Stats stats();
        
        //
        // Returns number of bytes in excess of capacity that
        // was not written to buffer. if nullptr buffer or
        // capacity == 0, then returns the number of bytes
        // needed to store the code in a second call.
        //
        // Let the client manage its memory usage.
        //
        uint64_t code(char *buffer=nullptr, uint64_t capacity=0);
        
    private:
        
        inline RawNodeLayout raw_node_layout(const RawNode* node) const {
            return RawNodeLayout(node->path_length(), node->children_length(), _bits_per_label);
        }
        
        RawNodeLayout raw_node_layout(PathLength path_length, ChildrenLength children_length) const {
            return RawNodeLayout(path_length, children_length, _bits_per_label);
        }

    public:
        
        ptr::Ptr<Allocator> _allocator; // allocator
        
        Caches _caches; // caches for node class sizes
        
        RawNodeOffsetPtr _root; // might be null
        
        NumBits _bits_per_label; // max of 8
        
    };
    
    
    
    //-------------------------------------------------------------------------------
    // Iter
    //-------------------------------------------------------------------------------
    
    //
    // Iterate through actions that allow a description
    // of a Hierarchy's current state
    //
    
    struct Iter {
        
        enum ActionType { VISIT, POP, DONE };
        
        struct Action {
        public: // class methods
            Action() = default;
            static Action visit(const Node &node) { return Action(VISIT,node); }
            static Action done() { return Action(DONE, Node()); }
            static Action pop()  { return Action(POP, Node()); }
        public:
            const Node& node() const { return _node; }
            bool isVisit() const { return _type == VISIT; }
            bool isPop() const { return _type == POP; }
            bool isDone() const { return _type == DONE; }
            operator bool() { return !isDone(); }
        private:
            Action(ActionType type, const Node& node): _type(type), _node(node) {}
        public:
            ActionType  _type { DONE };
            Node _node;
        };
        
        struct Item {
        public:
            Item() = default;
            static Item first(const Node& node) { return Item(node,0u,true); }
        private:
            Item(const Node& node, ChildrenLength index, bool first):
            _node(node), _index(index), _first(first)
            {}
        public:
            bool first() const { return _first; }
            const Node& node() const { return _node; }
            Node& node() { return _node; }
            ChildrenIndex index() const { return _index; }
            Item next() const { return Item(_node, (_first ? 0 : _index+1), false); }
        public:
            Node           _node;
            ChildrenIndex  _index { 0 };
            bool           _first { false };
        };
        
        struct Stack {
        public:
            static const unsigned int MAX_SIZE = 255;
        public:
            operator bool() const { return _size; }
            bool empty() const { return !_size; }
            const Item& back() const;
            void push_back(const Item& item);
            void pop_back();
        public:
            ChildrenLength _size { 0 };
            Item _data[MAX_SIZE];
        };
    public:
        Iter(Hierarchy& h);
        Action next();
    private:
        Hierarchy &_hierarchy;
        Stack _stack;
    };

    
    
} // hierarchy namespace












