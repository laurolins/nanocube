#include <iostream>
#include <unordered_map>

#include <FlatTree.hh>
#include <QuadTree.hh>
#include <STree.hh>

#include <boost/mpl/vector.hpp>

struct CountContent
{
    CountContent():
        count(0)
    {}

    int add(int weight) {
        count+=weight;
        return 0;
    }
    int count;

    CountContent *makeLazyCopy() const {
        return new CountContent(count);
    }

private:
    CountContent(int count):
        count(count)
    {}
};

typedef flattree::FlatTree<CountContent> D3;
typedef typename D3::AddressType D3Addr;

typedef quadtree::QuadTree<2,D3> D2;
typedef typename D2::AddressType D2Addr;

typedef quadtree::QuadTree<2,D2> D1;
typedef typename D1::AddressType D1Addr;

// typedef boost::mpl::
typedef boost::mpl::vector< D1, D2, D3 > Dimensions;

typedef stree::STree<Dimensions, int> NanoCube;
typedef typename NanoCube::AddressType Address;

struct Scanner {

    typedef typename D1::NodeType    N1;
    typedef typename D1::AddressType A1;

    typedef typename D2::NodeType    N2;
    typedef typename D2::AddressType A2;

    typedef typename D3::NodeType    N3;
    typedef typename D3::AddressType A3;

    struct V1 {

        V1():
            scanner(nullptr)
        {}

        void setScanner(Scanner *scanner) {
            this->scanner = scanner;
        }

        void visit(N1 *node, A1 &address) {
            scanner->visit1(node, address);
        }

        Scanner *scanner;
    };

    struct V2 {

        V2():
            scanner(nullptr)
        {}

        void setScanner(Scanner *scanner) {
            this->scanner = scanner;
        }

        void visit(N2 *node, A2 &address) {
            scanner->visit2(node, address);
        }

        Scanner *scanner;
    };

//    struct V3 {

//        V3():
//            scanner(nullptr)
//        {}

//        void setScanner(Scanner *scanner) {
//            this->scanner = scanner;
//        }

//        void visit(N3 *node, A3 &address) {
//            scanner->visit3(node, address);
//        }

//        Scanner *scanner;
//    };

    template <typename A>
    std::string toStr(const A &obj) {
        std::stringstream ss;
        ss << obj;
        return ss.str();
    }

//    template <typename N, typename A>
//    void visitQuadtreeNodePhase1(N *node, A *address, std::string prefix) {

//        int  id         = this->getId(prefix + toStr(address));

//        this->object_to_id_map[(void*) node] = id; // update object to id map

//        int  n = node->getNumChildren();
//        auto children_pointers = node->getChildrenArray();
//        for (int i=0;i<n;i++) {

//            auto &child_pointer = children_pointers[i];
//            bool proper         = child_pointer.isProper();

//            if (proper) {
//                N*  child_node = child_pointer.getNode();

//                quadtree::ChildName child_actual_index = node->getChildActualIndex(i);
//                A child_address = address.childAddress(child_actual_index);

//                int child_id     = this->getId(prefix + toStr(child_address));

//                this->object_to_id_map[(void*) child_node] = child_id; // update object to id map
//            }
//        }

//        // set context address for the content traversal
//        context_address_1 = address;

//        if (node->contentIsProper()) {
//            node->getContent()->scan(v2);
//        }

//    }

    void visit1(N1 *node, A1 &address) {

        // first pass: associate unique id to proper address
        // associate acutal objects found throught its proper path
        // to id
        if (phase == 1) {

            int  id         = this->getId(toStr(address));

            this->object_to_id_map[(void*) node] = id; // update object to id map

            int  n = node->getNumChildren();
            auto children_pointers = node->getChildrenArray();
            for (int i=0;i<n;i++) {

                auto &child_pointer = children_pointers[i];
                bool proper         = child_pointer.isProper();

                if (proper) {
                    N1*  child_node = child_pointer.getNode();

                    quadtree::ChildName child_actual_index = node->getChildActualIndex(i);
                    A1 child_address = address.childAddress(child_actual_index);

                    int child_id     = this->getId(toStr(child_address));

                    this->object_to_id_map[(void*) child_node] = child_id; // update object to id map
                }
            }

            // set context address for the content traversal
            context_address_1 = address;

            if (node->contentIsProper()) {
                node->getContent()->scan(v2);
            }

        }

        // Second pass: log structure
        else if (phase == 2) {

            int  id = object_to_id_map[(void*) node];

            int  n = node->getNumChildren();
            auto children_pointers = node->getChildrenArray();

            std::cout << "[" << id << "]"
                      << " dim: 1 "
                      << address
                      << " children: ";

            for (int i=0;i<n;i++) {
                auto &child_pointer = children_pointers[i];
                N1*  child_node = child_pointer.getNode();
                bool shared = child_pointer.isShared();

                int child_id = object_to_id_map[(void*) child_node];
                std::cout << child_id << (shared ? "S" : "P") << " ";
            }

            int  content_id = object_to_id_map[(void*) node->getContent()->root];
            bool content_is_shared = node->contentIsShared();
            std::cout << " content: "
                      << content_id
                      << (content_is_shared ? "S" : "P");

            std::cout << std::endl;

            if (!content_is_shared) {
                node->getContent()->scan(v2);
            }
        }
    }

    void visit2(N2 *node, A2 &address) {


        if (phase == 1) {

            std::string prefix = toStr(context_address_1);

            int  id = this->getId(prefix + toStr(address));

            this->object_to_id_map[(void*) node] = id; // update object to id map

            int  n = node->getNumChildren();
            auto children_pointers = node->getChildrenArray();
            for (int i=0;i<n;i++) {

                auto &child_pointer = children_pointers[i];
                bool proper         = child_pointer.isProper();

                if (proper) {
                    N2*  child_node = child_pointer.getNode();

                    quadtree::ChildName child_actual_index = node->getChildActualIndex(i);
                    A2 child_address = address.childAddress(child_actual_index);

                    int child_id     = this->getId(prefix + toStr(child_address));

                    this->object_to_id_map[(void*) child_node] = child_id; // update object to id map
                }
            }

            if (node->contentIsProper()) {
                auto content = node->getContent();
                int content_id = this->getId(prefix + toStr(address) + toStr((uint64_t) content));
                this->object_to_id_map[(void*) content] = content_id; // update object to id map
            }
        }

        else if (phase == 2) {

            int  id         = this->object_to_id_map[(void*) node];
            int  content_id = this->object_to_id_map[(void*) node->getContent()];
            bool content_is_shared = node->contentIsShared();

            int  n = node->getNumChildren();
            auto children_pointers = node->getChildrenArray();

            std::cout << "   [" << id << "]"
                      << " dim: 2 "
                      << address
                      << " content: "
                      << content_id
                      << (content_is_shared ? "S" : "P")
                      << " children: ";

            for (int i=0;i<n;i++) {
                auto &child_pointer = children_pointers[i];
                N2   *child_node    = child_pointer.getNode();
                bool  shared        = child_pointer.isShared();
                int   child_id      = this->object_to_id_map[(void*) child_node];
                std::cout << child_id << (shared ? "S" : "P") << " ";
            }
            std::cout << std::endl;
        }
    }

    Scanner():
        free_id(0)
    {
        v1.setScanner(this);
        v2.setScanner(this);
    }

    void scan(D1 &d1) {
        std::cout << "------------- scan --------------" << std::endl;

        // update object_to_id_map
        object_to_id_map.clear();
        phase = 1;
        d1.scan(v1);

        // use object_to_id_map and print
        phase = 2;
        d1.scan(v1);
    }

    int getId(std::string st) {
        auto it = id_map.find(st);
        if (it == id_map.end()) {
            int new_id = free_id++;
            id_map[st] = new_id;
            return new_id;
        }
        else return it->second;
    }

    int free_id;
    V1 v1; // visitor one
    V2 v2; // visitor two

    A1 context_address_1;

    int phase;

    std::unordered_map<std::string, int>   id_map;
    std::unordered_map<void*, int>         object_to_id_map;
};




//
// tile request is of the form:
//    TILE     -> tile/<level>/<x>/<y>/<level>/<offset>
//    TSERIES  -> tseries/<bin0>/<incr>/<count>
//    LABEL    -> label/<label_name>
//    REGION   -> region/<level>/<x0>/<y0>/<x1>/<y1>
//    WHERE    -> where/(<label_name>=<value>(|<value>)*)*
//
// assume it will search 8 levels deeper on the quadtree
// a matrix of 256x256 entries is the answer
//


int main() {

    NanoCube nc;

    Address addr;

    Scanner scanner;

    addr.set(D1Addr(3,3,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D2Addr(0,0,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D3Addr(0));
    nc.add(addr, 1);

    scanner.scan(nc.root);

    addr.set(D1Addr(3,3,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D2Addr(0,1,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D3Addr(2));
    nc.add(addr, 1);

    scanner.scan(nc.root);

    addr.set(D1Addr(2,2,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D2Addr(0,1,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D3Addr(2));
    nc.add(addr, 1);

    scanner.scan(nc.root);

    addr.set(D1Addr(3,2,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D2Addr(1,2,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D3Addr(1));

    nc.add(addr, 1);

    scanner.scan(nc.root);

    addr.set(D1Addr(3,0,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D2Addr(1,2,2,quadtree::FLAG_HIGH_LEVEL_COORDS));
    addr.set(D3Addr(1));

    nc.add(addr, 1);

    scanner.scan(nc.root);

    // VisitorD1 visitor;
    // nc.root.scan(visitor);

}







