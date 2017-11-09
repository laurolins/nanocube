#include <iostream>

#include <vector>
#include <cassert>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <map>

#include "nested_hierarchy.hh"
#include "hierarchy.hh"
#include "mmap.hh"

namespace test_hierarchy {
    
    using namespace hierarchy;
    
    LabelArray la(const std::vector<Label>& values) {
        static std::vector<Label> a;
        a = values;
        return LabelArray(&a[0],a.size());
    }
    
    std::ostream& operator<<(std::ostream& os, Hierarchy& h) {
        auto n = h.code();
        std::vector<char> buf(n);
        h.code(&buf[0],buf.size());
        os << &buf[0];
        return os;
    }
    
    void test() {
        
        // mmap 1TB of virtual memory in this process
        alloc::memory_map::MMap mmap(1ULL<<40);
        
        // @Check why 8 is the minimum again?
        auto& allocator = alloc::slab::allocator(mmap.memory_block());
        auto hierarchy_cache = allocator.cache_create("Hierarchy", sizeof(Hierarchy));
        // std::cout << sizeof(Hierarchy) << std::endl;
        auto &hierarchy = *(new (hierarchy_cache->alloc()) Hierarchy(allocator,2));
        
        //    hierarchy.insert(la({0,0,0}));
        //    std::cout << hierarchy << std::endl;
        //    hierarchy.insert(la({0,0,1}));
        //    std::cout << hierarchy << std::endl;
        //    hierarchy.insert(la({0,1}));
        //    std::cout << hierarchy << std::endl;
        
        //    hierarchy.insert(la({0,0}));
        //    std::cout << hierarchy << std::endl;
        //    hierarchy.insert(la({0,1}));
        //    std::cout << hierarchy << std::endl;
        //    hierarchy.insert(la({1}));
        //    std::cout << hierarchy << std::endl;
        
        hierarchy.insert(la({0,0,0}));
        std::cout << hierarchy << std::endl;
        hierarchy.insert(la({0,0,1}));
        std::cout << hierarchy << std::endl;
        hierarchy.insert(la({0,1}));
        std::cout << hierarchy << std::endl;
        hierarchy.insert(la({0,1,1}));
        std::cout << hierarchy << std::endl;
        hierarchy.insert(la({0,1,0}));
        std::cout << hierarchy << std::endl;
        
        auto stats = hierarchy.stats();
        
        // allocator.memory_block().size();
        
        std::cout << "-------------------------" << std::endl;
        std::cout << "Stats: " << stats.nodes() << "," << stats.bytes() << "," << (int) stats.depth() << std::endl;
        
        std::cout << "Done." << std::endl;
        
    } // test()
    
} // test_hierarchy

namespace test_nested_hierarchy {
    
    using namespace nested_hierarchy;

    // using NumBitsArray = ReadArray<NumBits, PathLength>;

    // num bits array
    NumBitsArray nba(std::vector<NumBits> &&values) {
        static std::vector<NumBits> num_bits;
        std::swap(num_bits, values);
        return NumBitsArray(&num_bits[0], (int) num_bits.size());
    }

    LabelArrayList lal(std::vector<const std::vector<Label>> &&array_of_label_arrays) {
        //
        // keep one of these static vector of vector per thread
        // while using the multi-part insert algorithm
        //
        static std::vector<const std::vector<Label>> static_array_of_label_arrays;
        static std::vector<LabelArrayList>           static_list_nodes;
        
        std::swap(static_array_of_label_arrays, array_of_label_arrays);
        
        static_list_nodes.reserve(static_array_of_label_arrays.size()); // make sure no relocation will happen
        static_list_nodes.clear();
        
        LabelArrayList *next { nullptr };
        for (auto it=static_array_of_label_arrays.rbegin();it!=static_array_of_label_arrays.rend();++it) {
            static_list_nodes.push_back(LabelArrayList(&(*it)[0],it->size()));
            static_list_nodes.back().next(next);
            next = &static_list_nodes.back();
        }
        return static_list_nodes.back();
    }

    
    namespace item_iter {

        using u64 = std::uint64_t;
        
        enum Type { NONE, NODE, PARENT_CHILD_EDGE, CONTENT_EDGE, RECORD_SET_EDGE, RECORD_SET };
        
        struct Item {
        public:
            Type     _type { NONE };
            Node     _src;
            Node     _dst;
            u64      _record_set { 0 };
            bool     _shared { false };
            int      _suffix_length { 0 };
        public:
            Item() = default;
            Item& type(Type t) { _type=t; return *this; }
            Item& src(Node n) { _src=n; return *this; }
            Item& dst(Node n) { _dst=n; return *this; }
            Item& record_set(u64 n) { _record_set=n; return *this; }
            
            Type  type() const { return _type; }
            
            Node& src() { return _src; }
            Node& dst() { return _dst; }
            
            u64   record_set() const { return _record_set; }

            bool  shared() const { return _shared; }
            int   suffix_length() const { return _suffix_length; }

            Item& shared(bool s) { _shared=s; return *this; }
            Item& suffix_length(int len) { _suffix_length=len; return *this; }
        };
        
        struct E {
            E() = default;
            E(RawNode* n, int dim): _node(n), _dimension(dim) {}
            RawNode* node() { return _node; }
            int      dimension() { return _dimension; }
            RawNode* _node      { nullptr };
            int      _dimension { 0 };
        };
        
        struct Iter {
        public:
            nested_hierarchy::NestedHierarchy &_hierarchy;
            std::vector<E>        _node_stack;
            std::vector<Item>     _item_stack;
            Item                  _current;
        public:
            Iter(nested_hierarchy::NestedHierarchy &h): _hierarchy(h) {
                _node_stack.push_back({h.root(),0});
            }
            
            Item* next() {
                while (true) {
                    if (_item_stack.size()) {
                        _current = _item_stack.back();
                        _item_stack.pop_back();
                        return &_current;
                    }
                    
                    if (!_node_stack.size()) { return nullptr; } // done!
                    
                    auto e = _node_stack.back();
                    _node_stack.pop_back();

                    auto raw_node = e.node();
                    auto node     = _hierarchy.to_node(raw_node, e.dimension());
                    
                    _item_stack.push_back(Item().type(NODE).src(node));
                    for (auto i=0;i<node.children().length();++i) {
                        auto &child = node.children().child(i);
                        auto child_node = _hierarchy.to_node(child.node(),e.dimension());
                        _item_stack.push_back(Item().type(PARENT_CHILD_EDGE)
                                              .src(node)
                                              .dst(child_node)
                                              .shared(child.shared())
                                              .suffix_length(child.suffix_length()));
                        if (!child.shared()) {
                            _node_stack.push_back({child_node.raw_node(),e.dimension()});
                        }
                    }
                    if (e.dimension() == _hierarchy.dimensions()-1) {
                        _item_stack.push_back(Item().type(RECORD_SET)
                                              .record_set(raw_node->content_u32()));
                        _item_stack.push_back(Item().type(RECORD_SET_EDGE)
                                              .src(node)
                                              .record_set(raw_node->content_u32()));
                    }
                    else { // send content
                        _item_stack.push_back(Item().type(CONTENT_EDGE)
                                              .src(node)
                                              .dst(_hierarchy.to_node(raw_node->content(),e.dimension()+1)));
                        _node_stack.push_back({raw_node->content(), e.dimension()+1});
                    }
                }
            }
            
            
        
            
        };

    }

    
    
    struct NodeDepth {
        
        NodeDepth() = default; // largest NodeDepth
        
        NodeDepth(RawNode* node) {
            _largest = false;
            _depth[_length] = 0;
            while (node) {
                _depth[_length] += node->path_length();
                if (node->root()) {
                    ++_length;
                    _depth[_length] = 0;
                }
                node = node->parent();
            }
        }
        
        int length() const { return _length; }
        
        int operator[](int index) const
        {
            return _depth[_length-1-index];
        }
        
        bool operator==(const NodeDepth& other) const
        {
            if (_largest && other._largest) return true;
            else if (_length != other._length) return false;
            else {
                return _depth[0] == other._depth[0];
//                for (auto i=0;i<length();++i) {
//                    if (vec[i] != other[i]) return false;
//                }
//                return true;
            }
        }
        
        bool operator<(const NodeDepth& other) const
        {
            if (_largest) return false;
            else if (other._largest) return true;
            else if (length() > other.length()) return false;
            else if (length() < other.length()) return true;
            else {
                return _depth[0] < other._depth[0];
            }
        }
        
        bool _largest { true };
        int  _length  { 0    };
        int  _depth[10];
    };

    std::ostream& operator<<(std::ostream& os, const NodeDepth& node_depth) {
        if (node_depth._largest) os << "[largest]";
        else {
            os << "[";
            for (auto i=0;i<node_depth.length();++i)
                os << (i > 0 ? "," : " ") << node_depth[i];
            os << "]";
        }
        return os;
    }

    
    
    struct Counter {
    public:
        int  num_bits;
        int  max;
        char value[1024]; // max odometer
        int  length;
        int  pos;
        int  offset;
    public:
        
        Counter(int b, int l, int offset): num_bits(b), length(l), offset(offset) {
            assert(l < 1024);
            max = (1 << b) - 1;
            for (auto i=0;i<offset;++i) value[i] = 0;
            reset();
        }

        void reset() {
            pos = offset;
            value[pos] = -1;
        }
        
        bool next() {
            if (pos < offset) return false;
            while (pos >= offset) {
                ++value[pos];
                if (value[pos] > max) {
                    --pos;
                }
                else if (pos < length-1) {
                    ++pos;
                    value[pos] = -1;
                }
                else { // if (pos == length-1)
                    return true;
                }
            }
            return false;
        }
        
        std::string values() {
            std::stringstream ss;
            for (auto i=0;i<length;++i)
                ss << (int) value[i];
            return ss.str();
        }
        
    };
    
    struct Permutation {
    public:
        int  p[1024];
        char data[1024];
        int  length;
    public:
        Permutation(int length):
            length(length)
        {
            for (auto i=0;i<length;++i) { p[i] = i; }
            for (auto i=length;i>1;--i) {
                auto index = (rand() % i);
                std::swap(p[i-1],p[index]);
            }
        }
        void update(char* raw) {
            for (auto i=0;i<length;++i) {
                data[i] = *(raw + p[i]);
            }
        }
        std::string values() {
            std::stringstream ss;
            for (auto i=0;i<length;++i)
                ss << (int) data[i];
            return ss.str();
        }
    };
    
    void save(NestedHierarchy& nested_hierarchy, const char* filename) {
        
        //
        using u64 = std::uint64_t;
        std::unordered_set<u64> groupings; // record sets
        
        //
        // first pass: write nodes
        //
        std::ofstream ss(filename);
        ss << "digraph nested_hierarchy {\n";
        
        // first pass: nodes
        {
            item_iter::Iter it(nested_hierarchy);
            item_iter::Item *item;
            while ((item = it.next())) {
                if (item->type() == item_iter::NODE) {
                    auto node = item->src();
                    ss << 'n' << (u64) node.raw_node() << " [label=\"";
                    if (node.path_length() == 0) ss << "e";
                    else {
                        for (auto i=0;i<node.path().length();++i) {
                            if (i > 0) { ss << "-"; }
                            ss << (int) node.path().get(i);
                        }
                    }
                    ss << "\"];\n";
                }
                else if (item->type() == item_iter::RECORD_SET) {
                    auto record_set = item->record_set();
                    if (!groupings.count(record_set)) {
                        groupings.insert(record_set);
                        ss << 'g' << record_set << " [label=\"{";
                        bool first_element = true;
                        for (auto i=0;i<64;++i) {
                            if (record_set & (1ull << i)) {
                                if (!first_element) { ss << ","; }
                                ss << (1+i);
                                first_element = false;
                            }
                        }
                        ss << "}\"];\n";
                    }
                }
            }
        }
        
        // second pass: edges
        {
            item_iter::Iter it(nested_hierarchy);
            item_iter::Item *item;
            while ((item = it.next())) {
                if (item->type() == item_iter::PARENT_CHILD_EDGE) {
                    auto parent = item->src();
                    auto child  = item->dst();
                    ss << 'n' << (u64) parent.raw_node() << " -> " << 'n' << (u64) child.raw_node() << " [dir=none penwidth=1.4 style=" << (item->shared() ? "dotted" : "solid")
                    << " label=\"" << item->suffix_length() << "\" ];";
                }
                else if (item->type() == item_iter::RECORD_SET_EDGE) {
                    auto record_set = item->record_set();
                    auto parent = item->src();
                    ss << 'n' << (u64) parent.raw_node() << " -> " << 'g' << record_set << " [dir=forward style=solid color=\"#80B1D3\"];\n";
                }
                else if (item->type() == item_iter::CONTENT_EDGE) {
                    auto parent = item->src();
                    auto child  = item->dst();
                    ss << 'n' << (u64) parent.raw_node() << " -> " << 'n' << (u64) child.raw_node() << " [dir=forward style=solid color=\"#80B1D3\"];\n";
                }
            }
        }
        
        // good test for the back pointer
        {
            std::map<NodeDepth,std::vector<std::string>> map;
            item_iter::Iter it(nested_hierarchy);
            item_iter::Item *item;
            while ((item = it.next())) {
                if (item->type() == item_iter::NODE) {
                    auto node = item->src();
                    NodeDepth key(node.raw_node());
                    // std::cout << 'n' << (u64) node.raw_node() << " --> " << key << std::endl;
                    std::stringstream ssnode;
                    ssnode << 'n' << (u64) node.raw_node();
                    auto &list = map[key];
                    list.push_back(ssnode.str());
                }
                else if (item->type() == item_iter::RECORD_SET) {
                    auto node = item->src();
                    NodeDepth key;
                    // std::cout << key << std::endl;
                    std::stringstream ssnode;
                    ssnode << 'g' << item->record_set();
                    auto &list = map[key];
                    list.push_back(ssnode.str());
                }
            }
            
            
            auto n = map.size();
            for (auto i=1;i<=n;++i) {
                ss << "dummy" << i << " [style=invis width=0.1 label=\"\"];\n";
            }
            ss << " dummy" << 1;
            for (auto i=2;i<=n;++i) {
                ss << " -> dummy" << i;
            }
            ss << "  [style=invis] ;\n";
            
            
            
            // create a dummy node per level to
            // force ordering and different y's
            int level = 0;
            for (auto &it: map) {
                ++level;
                // std::cerr << it.first << std::endl;
                ss << "{ rank=same; dummy" << level;
                for (auto &e: it.second) {
                    ss << ", " << e;
                }
                ss << " };\n";
            }
            
        }
        
        ss << "}";

    }
    
    void test() {
        
        // mmap 1GB of virtual memory in this process
        alloc::memory_map::MMap mmap(1ULL<<30);
        
        
#if 0
        //
        // test 0
        // In this test we have an example of a shared node that
        // shrink a suffix length becomes outdated. At the moment
        // we detect this issue we should be able to solve easily:
        // reshare to parent
        //
        auto& allocator             = alloc::slab::allocator(mmap.memory_block());
        auto nested_hierarchy_cache = allocator.cache_create("NestedHierarchy", sizeof(NestedHierarchy));
        auto &nested_hierarchy      = *(new (nested_hierarchy_cache->alloc()) NestedHierarchy(allocator,nba({2,2,2}))); // bits per level
        nested_hierarchy.insert(lal({{1,1,2},{1,1},{1,1}}));
        nested_hierarchy.insert(lal({{2,2,1},{2,2},{1,2}}));
        nested_hierarchy.insert(lal({{1,1,2},{1,2},{2,2}}));

#elif 0

        //
        // test 1
        // In this test we have an example of a shared node that
        // shrink a suffix length becomes outdated. At the moment
        // we detect this issue we should be able to solve easily:
        // reshare to parent
        //
        auto& allocator             = alloc::slab::allocator(mmap.memory_block());
        auto nested_hierarchy_cache = allocator.cache_create("NestedHierarchy", sizeof(NestedHierarchy));
        auto &nested_hierarchy      = *(new (nested_hierarchy_cache->alloc()) NestedHierarchy(allocator,nba({1,1,1}))); // bits per level
        
        // /tmp/test_0603_p4-4_000000000000100100000000010000100001_b1_d3_l3.dot
        
        // 000 000 000
        // 100 100 000
        // 001 000 010
        // 000 100 001
        nested_hierarchy.insert(lal({{0,0,0},{0,0,0},{0,0,0}}),0);
        save(nested_hierarchy,"/tmp/g1.dot");
        nested_hierarchy.insert(lal({{1,0,0},{1,0,0},{0,0,0}}),1);
        save(nested_hierarchy,"/tmp/g2.dot");
        nested_hierarchy.insert(lal({{0,0,1},{0,0,0},{0,1,0}}),2);
        save(nested_hierarchy,"/tmp/g3.dot");
        nested_hierarchy.insert(lal({{0,0,0},{1,0,0},{0,0,1}}),3);
        save(nested_hierarchy,"/tmp/g4.dot");
        nested_hierarchy::print(nested_hierarchy);
        
#else
        //
        // try finding minimal examples to stress the code
        // considering all examples with 1, 1, 1
        //
        {
            // two levels deep and one bit
            
            int num_points         = 5;
            int num_dimensions     = 4;
            int num_levels_per_dim = 4;
            int num_bits           = 1;

            // array with number of bits in each dimension
            NumBits      b[num_dimensions];
            std::fill(&b[0],&b[num_dimensions],(NumBits) num_bits);
            
            NumBitsArray bb(&b[0],num_dimensions);

            
            auto depth_per_record = num_levels_per_dim * num_dimensions;
            Counter counter(num_bits,
                            depth_per_record * num_points,
                            depth_per_record);
            
            LabelArrayList address[10]; // some max dimensions
            assert(num_dimensions <= 10);
            for (auto i=0;i<num_dimensions-1;++i) address[i].next(&address[i+1]);
            address[num_dimensions-1].next(nullptr);

            Permutation p(depth_per_record * num_points);
            
            char buffer[128];
            
            const float PRINT_PROBABILITY = 0.0f;
            
            int count = 0;
            while (counter.next()) {
                ++count;
                auto& allocator             = alloc::slab::allocator(mmap.memory_block());
                auto nested_hierarchy_cache = allocator.cache_create("NestedHierarchy", sizeof(NestedHierarchy));
                auto &nested_hierarchy      = *(new (nested_hierarchy_cache->alloc()) NestedHierarchy(allocator,bb)); // bits per level
                bool save_experiment = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) < PRINT_PROBABILITY;
                
                p.update(counter.value);
                auto p_labels = (const Label*) &p.data;
                
                for (auto i=0;i<num_points;++i) {
                    snprintf(buffer,sizeof(buffer),
                             "/tmp/test_%04d_p%d-%d_%s_b%d_d%d_l%d.dot",count,(i+1),num_points,p.values().c_str(),num_bits,num_dimensions,num_levels_per_dim);

                    
                    
                    // set address
                    for (auto j=0;j<num_dimensions;++j) {
                        address[j].reset(&p_labels[i * num_dimensions * num_levels_per_dim + (j * num_levels_per_dim)],
                                         num_levels_per_dim);
                    }
                    std::cerr << buffer << std::endl;
                    nested_hierarchy.insert(address[0],i);
                    if (save_experiment) {
                        save(nested_hierarchy, buffer);
                    }
                }
            }
        }
#endif

        
        
        
        //
        // test 1
        //
//        nested_hierarchy.insert(lal({{1,1,2},{1,1},{1,1}}));
//        nested_hierarchy.insert(lal({{2,2,1},{1,2},{1,2}}));
//        nested_hierarchy.insert(lal({{1,1,2},{1,2},{2,2}}));
        // nested_hierarchy.insert(lal({{2,1,2},{2,2},{2,1}}));
        // nested_hierarchy.insert(lal({{1,1,3},{2,2},{1,1}}));
        
        // insert on nested hierarchy
//        nested_hierarchy.insert(lal({{1},{1},{1,1}}));
//        nested_hierarchy.insert(lal({{2},{2},{1,2}}));
//        nested_hierarchy.insert(lal({{1},{2},{1,2}}));
//        nested_hierarchy.insert(lal({{2},{1},{1,1}}));

//        nested_hierarchy.insert(lal({{2,1,2},{2,2},{2,1}}));
//        nested_hierarchy.insert(lal({{1,1,3},{2,2},{1,1}}));

        
        
//        nested_hierarchy.insert(lal({{1},{1},{1,1}}));
//        nested_hierarchy.insert(lal({{2},{2},{1,2}}));
//        nested_hierarchy.insert(lal({{2},{1},{2,1}}));

//        nested_hierarchy.insert(lal({{1},{2},{2}}));
//        nested_hierarchy.insert(lal({{2},{1},{1}}));
//        nested_hierarchy.insert(lal({{1},{2},{1}}));
//        nested_hierarchy.insert(lal({{0},{0},{1}}));

        
        
//        nested_hierarchy::print(nested_hierarchy);
        
//        nested_hierarchy.insert(lal({{1},{1},{3}}));
//        nested_hierarchy.insert(lal({{1},{2},{1,1}}));
//        nested_hierarchy.insert(lal({{1},{1},{1}}));
//        nested_hierarchy.insert(lal({{2},{2},{2}}));

//        nested_hierarchy.insert(lal({{1},{1},{3}}));
//        nested_hierarchy.insert(lal({{1},{1},{3}}));
        // nested_hierarchy.insert(lal({{1},{1},{1}}));
        // nested_hierarchy.insert(lal({{2},{2},{2}}));
        // nested_hierarchy.insert(lal({{1},{0},{0}}));
        // nested_hierarchy.insert(lal({{3},{3},{3}}));
        
        

        
        
        

    

        
        // std::cout << ss.str() << std::endl;
    
    
//        char buffer[10000];
//        auto code = nested_hierarchy.code(buffer,10000);
//        std::cout << buffer << std::endl;
        
        // std::cout << sizeof(NestedHierarchy) << std::endl;
        
    } // test()
    
} // test_nested_hierarchy


//------
// main
//------

int main() {
    // test_hierarchy::test();
    test_nested_hierarchy::test();
}













