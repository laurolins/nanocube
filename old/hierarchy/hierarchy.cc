#include "hierarchy.hh"

#include <new>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <vector>
#include <sstream>

#include <bitset>
#include <iostream>

namespace hierarchy {
    
    //------------------------------------------------------------------------------
    // Most significant bit
    //------------------------------------------------------------------------------
    
    static inline unsigned int msb32(unsigned int x)
    {
        static const unsigned int bval[] =
        {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
        unsigned int r = 0;
        if (x & 0xFFFF0000) { r += 16/1; x >>= 16/1; }
        if (x & 0x0000FF00) { r += 16/2; x >>= 16/2; }
        if (x & 0x000000F0) { r += 16/4; x >>= 16/4; }
        return r + bval[x];
    }
    
    //
    // Normalize on the three most significant bits
    //
    // |  x | msb2(x) | msb3(x) |
    // |----+---------+---------|
    // |  1 |       1 |       1 |
    // |  2 |       2 |       2 |
    // |  3 |       3 |       3 |
    // |  4 |       4 |       4 |
    // |  5 |       6 |       5 |
    // |  6 |       6 |       6 |
    // |  7 |       8 |       7 |
    // |  8 |       8 |       8 |
    // |  9 |      12 |      10 |
    // | 10 |      12 |      10 |
    // | 11 |      12 |      12 |
    // | 12 |      12 |      12 |
    // | 13 |      16 |      14 |
    // | 14 |      16 |      14 |
    // | 15 |      16 |      16 |
    // | 16 |      16 |      16 |
    //
    
//    static unsigned int normalize_msb3(unsigned int num_bytes) {
//        static const unsigned int class_bits = 3;
//        auto nobits = msb32(num_bytes);
//        if (nobits > class_bits) {
//            auto class_shift = nobits - class_bits;
//            auto remainder   = num_bytes % (1u << class_shift);
//            return ((num_bytes >> class_shift) + (remainder > 0u)) << class_shift;
//        }
//        else {
//            return num_bytes;
//        }
//    }
    
    //
    // Normalize on the two most significant bits
    //
    static unsigned int normalize_msb2(unsigned int num_bytes) {
        static const unsigned int class_bits = 2;
        auto nobits = msb32(num_bytes);
        if (nobits > class_bits) {
            auto class_shift = nobits - class_bits;
            auto remainder   = num_bytes % (1u << class_shift);
            return ((num_bytes >> class_shift) + (remainder > 0u)) << class_shift;
        }
        else {
            return num_bytes;
        }
    }
    
    //------------------------------------------------------------------------------
    // Bit
    //------------------------------------------------------------------------------

    //
    // Assuming it is safe to access memory range:
    //
    //   input[0]  to input[(offset + bits + 7)/8]
    //   output[0] to output[(offset + bits + 7)/8]
    //
    
    static void read_bits(const char*  input,
                          unsigned int offset, // offset on the input
                          unsigned int bits,
                          char*        output) {

        
        // align a 64 bits unsigned int with input array
        std::uint64_t* src        = (std::uint64_t*) (input + (offset / 8));
        auto           src_bitoff = offset & 0x7u; // read
        
        //
        // if src_bitoff == 3 then
        //      mask_left  == 1110000...000000 (64 bits)
        //      mask_right == 0001111...111111 (64 bits)
        //
        auto mask_right = ~0ULL << src_bitoff;   // erase 3 least significant bits and negate
        auto mask_left  = ~mask_right;
        
        std::uint64_t *dst = (std::uint64_t*) output;
        
        auto remaining_bits = bits;
        while (remaining_bits > 0) {
            if (remaining_bits < 64) {
                // copy last bytes from src into a local 64 bit storage space
                std::uint64_t src_copy = 0;
                auto n = (remaining_bits + 7u) / 8u;
                std::copy((char*) src, (char*) src + n, (char*) &src_copy);
                auto mask_end = ~(mask_left | ~(~0ULL >> (64 - remaining_bits - src_bitoff)));
                src_copy &= mask_end; // shift bit offset (<8)
                src_copy >>= src_bitoff;
                auto p = (char*) &src_copy;
                std::copy(p, p + n, (char*) dst);
                remaining_bits = 0;
            }
            else {
                *dst = src_bitoff ? (*src >> src_bitoff) | ( (*(src+1) & mask_left) << (64 - src_bitoff) ) : *src;
                ++src;
                ++dst; // done writing on this 64-bit block. go to next
                remaining_bits -= 64;
            }
        }
    }
    
    static void write_bits(const char *input,
                           unsigned int offset, // offset on the output
                           unsigned int bits,
                           char * output) {

        // align a byte 64 bit unsigned int on the
        std::uint64_t* dst        = (std::uint64_t*) (output + (offset / 8));
        std::uint64_t  dst_bitoff = offset & 0x7u; // which bit offset should I start writing on
        
        //
        // if dst_bitoff == 3 then
        //      mask_left  == 1110000...000000 (64 bits)
        //      mask_right == 0001111...111111 (64 bits)
        //
        auto mask_right = ~0ULL << dst_bitoff;   // erase 3 least significant bits and negate
        auto mask_left  = ~mask_right;
        
        const std::uint64_t *src = (const std::uint64_t*) input;
        
        //
        // consume the bits in data a chunk of 64 at a time
        // they are all byte aligned
        // when less than 64 bits are available write them
        //
        auto remaining_bits = bits;
        while (remaining_bits > 0) {
            
            if (remaining_bits < 64) {
                
                // copy last bytes from src into a local 64 bit storage space
                std::uint64_t src_copy = 0;
                auto n = (remaining_bits + 7u)/ 8u;
                std::copy((char*) src, (char*) src + n, (char*) &src_copy);
                
                //
                // ones on the extremes 1111 000000000 1111
                // # zeros in the middle is equal to remaining_bits
                //
                auto mask_end = mask_left | ~(~0ULL >> (64 - remaining_bits - dst_bitoff));
                
                *dst = (*dst & mask_end) | (src_copy << dst_bitoff);
                
                remaining_bits = 0;
            }
            else {
                // assuming we still need to write 64 or more bits
                // from src to dst
                
                // write lower piece of the data into dst
                *dst = (*dst & mask_left) | (*src << dst_bitoff);
                
                ++dst; // done writing on this 64-bit block. go to next
                
                // is there a right part of the data
                if (dst_bitoff) {
                    
                    //
                    // we still have to write dst_bitoff bits in the beginning of
                    // this new 64 bit block on dst
                    //
                    
                    *dst = (*dst & mask_right) | (*src >> (64 - dst_bitoff));
                }
                remaining_bits -= 64;
            }
        }
        
    }
    
    //------------------------------------------------------------------------------
    // Auxiliar free functions
    //------------------------------------------------------------------------------
    
    PathLength _rawnode_check_common_path_length(const RawNode* raw_node, const LabelArray& labels, NumBits bits_per_label) {
        // this is the compressed labels address
        const char *compressed_labels = (const char*) raw_node + sizeof(RawNode);
        
        unsigned int offset = 0u;
        auto n = std::min(raw_node->path_length(), labels.length());
        PathLength result = 0;
        for (auto i=0;i<n;++i) {
            Label lbl = 0;
            
            //
            // there is an overhead here that can be avoided iterate
            // through fixed bit labels of a compressed array
            //
            read_bits(compressed_labels, offset, bits_per_label, (char*) &lbl);
            // std::cout << (int) lbl << std::endl;
            
            auto input_label = labels.get(i);

            assert((input_label < (1u<<bits_per_label)) && "input path label has too many bits"); // wrong input
            
            if (lbl != input_label) { break; }
            
            ++result;
            offset += bits_per_label;
        }
        
        return result;
    }
    
    Label _rawnode_read_first_label(const RawNode* raw_node, NumBits bits_per_label) {
        // this is the compressed labels address
        const char *compressed_labels = (const char*) raw_node + sizeof(RawNode);
        
        assert(raw_node->path_length() > 0);
        
        Label result = 0;
        read_bits(compressed_labels, 0, bits_per_label, (char*) &result);
        return result;
    }
    
    //
    // if result >= 0 then found
    // else insertion point is -result-1 is the insertion point
    //
    int _children_binary_search_label(Children& children, Label label, NumBits bits_per_label) {
        // @Optimize lots of page misses (could be avoided if we can tag the children pointers)
        
        auto n = children.length();
        if (n == 0) return -1; // didn't find and can insert in position 0
        // (-1) * result - 1 = (-1) * (-1) - 1 = 0
        
        // binary search
        auto l = 0;
        auto r = n-1; // think of label as a lower bound
        while (l < r) {
            auto m  = (l + r) / 2;
            auto arg = _rawnode_read_first_label(children.get(m), bits_per_label);
            if (label > arg) { // need to make class_m larger (move l)
                l = m + 1;
            }
            else if (label < arg) { // need to make class_m smaller (move r)
                r = m;
            }
            else return m;
        }
        
        // didn't find check what it is the result:
        auto arg = _rawnode_read_first_label(children.get(l), bits_per_label);
        return (arg == label) ? l : (-1 * (l + (arg < label) + 1));
    }

    
    
    
    
    
    
//    //
//    // if result >= 0 then found
//    // else insertion point is -result-1 is the insertion point
//    //
//    int _children_copy_and_insert(Children& target, Children& source, Label label, RawNode* new_child, NumBits bits_per_label) {
//        // @Optimize lots of page misses (could be avoided if we can tag the children pointers)
//        
//        target.copy(
//        
//        auto n = children.length();
//        if (n == 0) return -1; // didn't find and can insert in position 0
//        // (-1) * result - 1 = (-1) * (-1) - 1 = 0
//        
//        // binary search
//        auto l = 0;
//        auto r = n-1; // think of label as a lower bound
//        while (l < r) {
//            auto m  = (l + r) / 2;
//            auto arg = _rawnode_read_first_label(children.get(m), bits_per_label);
//            if (label > arg) { // need to make class_m larger (move l)
//                l = m + 1;
//            }
//            else if (label < arg) { // need to make class_m smaller (move r)
//                r = m;
//            }
//            else return m;
//        }
//        
//        // didn't find check what it is the result:
//        auto arg = _rawnode_read_first_label(children.get(l), bits_per_label);
//        return (arg == label) ? l : (-1 * (l + (arg < label) + 1));
//    }

    //------------------------------------------------------------------------------
    // Path
    //------------------------------------------------------------------------------
    
    Path::Path(Label* begin, PathLength length, PathLength capacity, NumBits bits_per_label):
        _begin(begin),
        _length(length),
        _capacity(capacity),
        _bits_per_label(bits_per_label)
    {}
    
    void Path::copy(const Path& other, PathLength offset, PathLength length) {
        assert(_capacity >= length);
        for (auto i=0;i<length;++i) {
            set(i, other.get(offset + i));
        }
        _length = length;
    }
    
    Label Path::get(PathIndex i) const {
        assert(i < _length);
        Label result = 0;
        read_bits((char*) _begin, i * _bits_per_label, _bits_per_label, (char*) &result);
        return result;
    }
    
    void  Path::set(PathIndex i, Label label) {
        assert(i < _length);
        // std::cout << "Path::set(" << (int) label << ")" << std::endl;
        write_bits((const char*) &label, i * _bits_per_label, _bits_per_label, (char*) _begin);
    }
    
    void Path::clear() { std::fill((char*) _begin, (char*) _begin + bytes(), 0u); }
    
    PathLength Path::length() const { return _length; }

    PathLength Path::capacity() const { return _capacity; }
    
    NumBytes Path::bytes() const { return (_capacity * _bits_per_label + 7u) / 8u; }

    
    //------------------------------------------------------------------------------
    // Children
    //------------------------------------------------------------------------------
    
    //
    // Wraps a region reserved for a children node pointer with the functionality
    // needed. Desgined to be used shortly every time there is a need to manipulate
    // children (query or update)
    //
    
    Children::Children(RawNodeOffsetPtr* begin, ChildrenLength length, ChildrenLength capacity):
        _begin(begin), _length(length), _capacity(capacity)
    {}
    
    
    // it is not const on "other" because we copy pointers
    void Children::copy(Children& other, ChildrenLength offset, ChildrenLength length) {
        assert(_capacity >= length);
        _length = length; // overwrite length
        for (auto i=0;i<length;++i) {
            set(i, other.get(offset + i));
        }
    }

    const RawNode* Children::get(ChildrenIndex i) const {
        assert(i < _length);
        return (_begin + i)->raw_null();
    }

    RawNode* Children::get(ChildrenIndex i) {
        assert(i < _length);
        return (_begin + i)->raw_null();
    }
    
    void Children::set(ChildrenIndex i, RawNode* child) {
        assert(i < _length);
        (_begin + i)->reset(child);
    }
        
    void Children::clear() {
        std::fill(_begin, _begin + _length, RawNodeOffsetPtr());
    }
    
    void Children::append_and_rotate(RawNode *new_child, ChildrenLength index) {
        assert(_capacity > _length);
        ++_length;
        set(_length-1, new_child);
        std::rotate(_begin+index, _begin+_length-1, _begin+_length);
    }

    
    //------------------------------------------------------------------------------
    // RawNodeLayout
    //------------------------------------------------------------------------------
    
    RawNodeLayout::RawNodeLayout(PathLength path_length, ChildrenLength children_length, NumBits bits_per_label):
        bits_per_label(bits_per_label)
    {
        path_capacity     = (PathLength) normalize_msb2(path_length);
        children_capacity = normalize_msb2(children_length);
        auto path_bytes     = (bits_per_label * path_capacity + 7u) / 8u;
        auto children_bytes = ((NumBytes) sizeof(RawNodeOffsetPtr) * children_capacity * 8 + 7u) / 8u;
        auto header_bytes   = (NumBytes) sizeof(RawNode);
        path_offset       = header_bytes;
        children_offset   = path_offset + path_bytes;
        total_size        = normalize_msb2(header_bytes + path_bytes + children_bytes);
        // std::cout << total_size << std::endl;
    }
    
    //------------------------------------------------------------------------------
    // RawNode
    //------------------------------------------------------------------------------
    
    inline Label* _raw_path(RawNode *node, const RawNodeLayout& layout) {
        return (Label*) ((char*) node + layout.path_offset);
    }
    
    inline RawNodeOffsetPtr* _raw_children(RawNode *node, const RawNodeLayout& layout) {
        return (RawNodeOffsetPtr*) ((char*) node + layout.children_offset);
    }

    Node::Node(RawNode *raw_node, const RawNodeLayout& layout):
        _raw_node(raw_node),
        _path(_raw_path(raw_node, layout), raw_node->path_length(), layout.path_capacity, layout.bits_per_label),
        _children(_raw_children(raw_node, layout), raw_node->children_length(), layout.children_capacity),
        _size_class(layout.total_size)
    {}
    
    //------------------------------------------------------------------------------
    // Caches
    //------------------------------------------------------------------------------
    
    static inline unsigned int size_class_to_index(SizeClass s) {
        auto msb = msb32(s);
        return (s > 1) * ( (msb-1) * 2 - ( ( (1 << (msb-2)) & s ) == 0 ) );
    }
    
    const SizeClass Caches::CLASSES[] = {
        1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64,
        96, 128, 192, 256, 384, 512, 768, 1024,
        1536, 2048, 3072, 4096
    };
    
    const unsigned int Caches::NUM_CLASSES = sizeof(Caches::CLASSES)/sizeof(SizeClass);
    
    Caches::Caches(Allocator& allocator) {
        char name[32];
        for (auto i=0u;i<NUM_CLASSES;++i) {
            auto s = std::max(6u,CLASSES[i]);  // smaller classes have 6 bytes (size of a ptr::Ptr<T>)
            std::snprintf(name,31,"node%d",s);
            _caches[i] = allocator.cache_create(name, s);
        }
    }
    
    void* Caches::alloc(SizeClass s) {
        // std::cerr << "Caches::alloc(" << s << ")" << std::endl;
        // figure out size class index
        auto index = size_class_to_index(s);
        assert(index < NUM_CLASSES);
        return _caches[index]->alloc();
    }
    
    void  Caches::free(SizeClass s, void *p) {
        // std::cerr << "Caches::free(" << s << "," << p << ")" << std::endl;
        auto index = size_class_to_index(s);
        assert(index < NUM_CLASSES);
        return _caches[index]->free(p);
    }
    
    //------------------------------------------------------------------------------
    // Hierarcy::Stats
    //------------------------------------------------------------------------------

    Hierarchy::Stats::Stats() {
        std::fill(&_nodes_per_level[0],&_nodes_per_level[MAX_PATH_LENGTH],0);
        std::fill(&_nodes_per_path_length[0],&_nodes_per_path_length[MAX_PATH_LENGTH],0);
        std::fill(&_nodes_per_degree[0],&_nodes_per_degree[MAX_CHILDREN_LENGTH],0);
        std::fill(&_nodes_per_depth[0],&_nodes_per_depth[MAX_PATH_LENGTH],0);
    }

    void Hierarchy::Stats::insert(PathLength level,
                                  PathLength depth,
                                  PathLength path_length,
                                  ChildrenLength degree,
                                  uint64_t bytes) {
        ++_nodes;
        _length_sum += path_length;
        _levels = std::max(_levels, level);
        _depth = std::max(_depth, depth);
        _bytes  += bytes;
        _nodes_per_level[level] += 1;
        _nodes_per_degree[degree] += 1;
        _nodes_per_depth[depth] += 1;
        _nodes_per_path_length[path_length] += 1;
    }

    //------------------------------------------------------------------------------
    // Hierarcy
    //------------------------------------------------------------------------------
    
    Hierarchy::Hierarchy(Allocator& allocator, NumBits bits_per_label):
    _allocator(&allocator),
    _caches(allocator),
    _bits_per_label(bits_per_label)
    {}
    
    Node Hierarchy::allocate_node(PathLength path_length, ChildrenLength children_length, bool clear) {
        auto node_layout = raw_node_layout(path_length, children_length);
        auto raw_node    = new (_caches.alloc((SizeClass)node_layout.total_size)) RawNode(path_length,children_length);
        auto node        = Node(raw_node, node_layout);
        if (clear) {
            node.path().clear();
            node.children().clear();
        }
        return node;
    }
    
    void Hierarchy::free_node(Node& node) {
        _caches.free(node.size_class(), node.raw_node());
    }

    Position Hierarchy::find(const LabelArray& labels) {
        
        RawNode* parent = nullptr;
        auto     child  = root();
        
        if (!child) { return Position(); } // base case root is empty
        
        PathLength cumlength = 0u;
        PathLength parent_child_index   = 0u;
        PathLength child_newchild_index = 0u;
        
        LabelArray current_labels = labels;
        while (true) {
            
            // check common prefix length
            auto n = _rawnode_check_common_path_length(child, current_labels, bits_per_label());
            
            cumlength += n;
            
            if (n == current_labels.length()) {
                // found path!
                return Position(parent, child, n, cumlength, parent_child_index, child_newchild_index);
            }
            else if (n == child->path_length()) {
                // see if child has a path to continue matching current_labels
                auto node = to_node(child);
                
                auto input_next_label = current_labels.get(n);
                assert(input_next_label < (1u<<bits_per_label()) && "input path label has too many bits");
                
                auto pos = _children_binary_search_label(node.children(), input_next_label, _bits_per_label);
                if (pos >= 0) {
                    // yes! can continue matching labels on current_labels. repeat.
                    parent = child;
                    parent_child_index = pos;
                    child  = node.children().get(pos);
                }
                else {
                    // no! child would need a new branch. done.
                    child_newchild_index = (ChildrenIndex) (-pos - 1);
                    return Position(parent, child, n, cumlength, parent_child_index, child_newchild_index);
                }
            }
            else {
                // path splits before
                return Position(parent, child, n, cumlength, parent_child_index, child_newchild_index); // found path
            }

            current_labels = current_labels.drop_prefix(n);
        
        }
    }
    
    inline void hierarchy_set_parent(Hierarchy*    hierarchy,
                                     RawNode*      parent,
                                     ChildrenIndex replace_index,
                                     RawNode*      replacing_child) {
        if (parent) {
            auto parent_node = hierarchy->to_node(parent);
            parent_node.children().set(replace_index, replacing_child);
        }
        else {
            hierarchy->_root.reset(replacing_child);
        }
    }
    
    

    std::ostream& operator<<(std::ostream& os, const Path& path) {
        os << "   path len/cap: " << (int) path.length() << "/" << (int) path.capacity() << std::endl;
        os << "   path:         ";
        for (auto i=0;i<path.length();++i) {
            //if (i > 0)
            //    os << ":";
            os << (int) path.get(i);
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Children& children) {
        os << "   children len/cap: " << (int) children.length() << "/" << (int) children.capacity() << std::endl;
        os << "   children:         ";
        for (auto i=0;i<children.length();++i) {
            os << "[" << i << "]";
            os << children.get(i);
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Node& node) {
        os << "......... node ........." << std::endl;
        os << "raw:    " << node._raw_node << std::endl;
        os << "bytes:  " << node.size_class() << std::endl;
        os << node.path()     << std::endl;
        os << node.children();
        return os;
    }
    
    
    
    
    
#define xLOG_INSERT_MESSAGES
#define xLOG_INSERT_NODES


#ifdef LOG_INSERT_MESSAGES
    static const char *insert_messages[] {
        "(0) first insertion",
        "(1) path already exists: no change",
        "(2) continuation: replace leaf with deeper one",
        "(3) insert leaf on existing node",
        "(4) insert leaf on enlarged node",
        "(5) split path"
    };
    void _log_insert_message(int insert_case) {
        assert(insert_case < 6);
        std::cerr << "------------------------" << insert_messages[insert_case] << std::endl;
    };
#define LOG_INSERT_MESSAGE(i) (_log_insert_message(i));
#else
#define LOG_INSERT_MESSAGE(i) ((void)0);
#endif

#ifdef LOG_INSERT_NODES
#define LOG_INSERT_NODE(a,b) std::cerr << a << std::endl << b << std::endl;
#else
#define LOG_INSERT_NODE(a,b) ((void)0);
#endif
    

    
    
    //
    //  (before)
    //
    //     parent_node ----> [*,child_node] ----> [*]
    //
    //  Cases:
    //
    //  (0) empty       ----> [new_node]
    //
    //  (1) (already exists: no change)
    //
    //  (2) parent_node ----> [*,new_node]  (continuation of a leaf; replace with deeper leaf)
    //
    //  (3) parent_node ----> [*,child_node] ----> [*,new_node] (insert on existing node with empty children space)
    //
    //  (4) parent_node ----> [*,extended_child_node] ----> [*,new_node] (insert into enlarged child)
    //
    //  (5) parent_node ----> [*,prefix_child_node] ----> [new_node, suffix_child_node] (split path)
    //
    //
    void Hierarchy::insert(const LabelArray& labels) {

        // here is where the game is played
        if (root()) {
            auto pos = find(labels);
            auto rawparent = pos.parent();
            auto rawchild  = pos.child();

            // the node to insert child already exists
            auto depth_branch = pos.distance_to_root();
            auto depth_new    = labels.length();
            
            if (depth_branch == depth_new) {
                LOG_INSERT_MESSAGE(1);
                return;
            }

            auto depth_parent = pos.distance_to_root() - pos.distance_to_parent();
            
            auto child_node = to_node(rawchild);
            auto depth_child  = depth_parent + child_node.path_length();

            auto dist_pb = depth_branch - depth_parent;
            auto dist_pc = depth_child  - depth_parent;
            auto dist_pn = depth_new    - depth_parent;                              // new_path depth from split
            auto dist_bn = depth_new    - depth_branch;                              // new_path depth from split
            auto dist_bc = depth_child  - depth_branch;                              // new_path depth from split

            if (dist_pb == dist_pc && !child_node.degree()) {
                LOG_INSERT_MESSAGE(2);
                auto new_node = allocate_node(dist_pn, 0);
                for (auto i=0;i<dist_pn;++i) {
                    new_node.path().set(i,labels.get(depth_parent+i));
                }
                hierarchy_set_parent(this, rawparent, pos._parent_child_index, new_node.raw_node());
                free_node(child_node);
            }
            else {
                //
                // allocate and prepare new_child (present in all 3 cases)
                //
                auto new_node = allocate_node(dist_bn, 0);
                for (auto i=0;i<dist_bn;++i) {
                    new_node.path().set(i,labels.get(depth_branch+i));
                }
                Label start_label_new_node = new_node.path().get(0);

                LOG_INSERT_NODE("NEW",new_node);

                if (dist_pb == dist_pc) {
                    if (child_node.children().available_capacity()) {
                        LOG_INSERT_MESSAGE(3);
                        
                        // won't need a replacement child node. simple insertion
                        
                        // @Optimize could use the hint here (avoid an extra binary search)
                        
                        auto index = _children_binary_search_label(child_node.children(),start_label_new_node,bits_per_label());
                        assert(index < 0);
                        auto insertion_index = -index - 1;
                        child_node.children().append_and_rotate(new_node.raw_node(),insertion_index);
                        
                    }
                    else {
                        LOG_INSERT_MESSAGE(4);
                        
                        // allocate new node with more children space
                        auto extended_child_node = allocate_node(child_node.path_length(),
                                                                 child_node.degree() + 1);
                        extended_child_node.path().copy(child_node.path());
                        extended_child_node.children().copy(child_node.children()); // modifies the length of extended_child_node.children()
                        
                        auto index = _children_binary_search_label(child_node.children(),start_label_new_node,bits_per_label());
                        assert(index < 0);
                        auto insertion_index = -index - 1;
                        extended_child_node.children().append_and_rotate(new_node.raw_node(),insertion_index); // wrapper degree is equal

                        hierarchy_set_parent(this, rawparent, pos._parent_child_index, extended_child_node.raw_node());
                        free_node(child_node);
                        
                        LOG_INSERT_NODE("CHILD NODE",child_node);
                        if (rawparent) { LOG_INSERT_NODE("PARENT",to_node(rawparent)); }
                        LOG_INSERT_NODE("EXTENDED", extended_child_node);
                        LOG_INSERT_NODE("NEW", new_node);
                    }
                }
                else {
                    LOG_INSERT_MESSAGE(5);

                    // allocate suffix replace node
                    auto suffix_child_node = allocate_node(dist_bc,child_node.degree());
                    suffix_child_node.path().copy(child_node.path(),dist_pb,dist_bc);
                    suffix_child_node.children().copy(child_node.children()); // same degree

                    
                    auto start_label_suffix_child_node = suffix_child_node.path().get(0);
                    
                    // allocate prefix replace node
                    auto prefix_child_node = allocate_node(dist_pb,2);
                    prefix_child_node.path().copy(child_node.path(),0,dist_pb);

                    // set the two children
                    ChildrenIndex i_new = (start_label_suffix_child_node < start_label_new_node);
                    prefix_child_node.children().set( 1u - i_new, suffix_child_node.raw_node()   );
                    prefix_child_node.children().set(      i_new, new_node.raw_node()            );
                    
                    hierarchy_set_parent(this, rawparent, pos._parent_child_index, prefix_child_node.raw_node());
                    free_node(child_node);

                    LOG_INSERT_NODE("CHILD NODE",child_node);
                    if (rawparent) { LOG_INSERT_NODE("PARENT",to_node(rawparent)); }
                    LOG_INSERT_NODE("PREFIX", prefix_child_node);
                    LOG_INSERT_NODE("SUFFIX", suffix_child_node);
                    LOG_INSERT_NODE("NEW", new_node);
                }
            }
        }
        else {
            LOG_INSERT_MESSAGE(0);

            // new tree with a single path (should cover lots of cases)
            auto node = allocate_node(labels.length(), 0);
            auto &path = node.path();
            for (auto i=0;i<labels.length();++i) {
                path.set(i,labels.get(i));
            }
            
            // std::cout << "path: " << std::hex << std::bitset<16>(*((unsigned short*)path._begin)) << std::endl;
            
            _root = node.raw_node();
        }
    }
    
    auto Hierarchy::stats() -> Stats {
        Stats stats;
        auto &h = *this;
        Iter iter(h);
        Iter::Action a;
        std::vector<PathLength> path_lengths;
        int  level = -1;
        auto depth =  0;
        while ((a = iter.next())) {
            if (a.isVisit()) {
                ++level;
                auto path_length = a.node().path().length();
                auto degree      = a.node().children().length();
                depth += path_length;
                path_lengths.push_back(path_length);
                stats.insert(level, depth, path_length, degree, std::max(8u,a.node().size_class()) ); // at least 8 bytes per node
            }
            else {
                --level;
                depth-=path_lengths.back();
                path_lengths.pop_back();
            }
        }
        return stats;
    }

    
    
    
    std::ostream& operator<<(std::ostream& os, Hierarchy& h) {
        Iter iter(h);
        auto a = Iter::Action::done();
        while ( (a = iter.next()) ) {
            if (a.isVisit()) {
                os << '.';
                auto first = true;
                for (auto i=0;i<a.node().path().length();++i) {
                    if (!first) {
                        // os << ":";
                    }
                    os << (int) a.node().path().get(i);
                    first = false;
                }
                a.node().path();
                
            }
            else if (a.isPop()) {
                os << '<';
            }
            else {
                assert(0 && "ooops!");
            }
        }
        return os;
    }
    
    
    struct CodeIter {
        CodeIter(Hierarchy& h): _iter(h) {}
        const std::string* next() {
            std::stringstream ss;
            auto a = _iter.next();
            if (a.isVisit()) {
                ss << '.'; // node
                for (auto i=0;i<a.node().path().length();++i) {
                    if (i > 0) { ss << "-"; }
                    ss << (int) a.node().path().get(i);
                }
            }
            else if (a.isPop()) {
                ss << '<';
            }
            else if (a.isDone()){
                return nullptr;
            }
            else { assert(0 && "unexpected"); }
            _str = ss.str();
            return &_str;
        }
        Iter _iter;
        std::string _str;
    };
    
//    uint64_t Hierarchy::code_length() const {
//        auto &h = *this;
//        CodeIter code_iter(h);
//        NumBytes result = 0;
//        const std::string *s;
//        while ( (s = code_iter.next()) ) {
//            result += s->size();
//        }
//        result += 1; // to write the \0 null terminator
//        return result;
//    }
//    
    uint64_t Hierarchy::code(char *buffer, uint64_t capacity) {
        if (!buffer)
            capacity=0;
        auto &h = *this;
        CodeIter code_iter(h);
        uint64_t offset = 0;
        const std::string *s;
        uint64_t not_written = 0;
        while ( (s = code_iter.next()) ) {
            auto size = s->size();
            if (not_written > 0) {
                not_written += size;
            }
            else {
                if (offset + size < capacity) {
                    std::copy(s->begin(),s->end(),buffer+offset);
                    offset += size;
                }
                else {
                    not_written = size;
                    if (offset < capacity)
                        buffer[offset] = 0;
                }
            }
        }
        if (not_written > 0) {
            not_written += 1; // null terminator
        }
        return not_written;
    }

    
    
    
    //-------------------------------------------------------------------------------
    // Iter
    //-------------------------------------------------------------------------------

    auto Iter::Stack::back() const -> const Item& {
        assert(!empty());
        return _data[_size-1];
    }
    
    void Iter::Stack::push_back(const Item& item) {
        assert(_size < MAX_SIZE);
        _data[_size] = item;
        ++_size;
    }

    void Iter::Stack::pop_back() {
        assert(_size);
        --_size;
        _data[_size] = Item();
    }

    //-------------------------------------------------------------------------------
    // Iter
    //-------------------------------------------------------------------------------
    
    Iter::Iter(Hierarchy& h):
    _hierarchy(h)
    {
        if (h.root()) {
            _stack.push_back(Item::first(h.to_node(h.root())));
        }
    }

    Iter::Action Iter::next() {
        while(true) {
            if (_stack.empty()) {
                return Action::done();
            }
            auto item = _stack.back();
            _stack.pop_back();
            
            if (item.first()) {
                _stack.push_back(item.next()); // schedule visit of first child
                return Action::visit(item.node());
            }
            else {
                if (item.index() < item.node().degree()) {
                    _stack.push_back(item.next()); // schedule visit of next child
                    auto raw_child = item.node().children().get(item.index());
                    auto child     = _hierarchy.to_node(raw_child);
                    _stack.push_back(Item::first(child));
                }
                else {
                    return Action::pop();
                }
            }
        }
    }
}







































































