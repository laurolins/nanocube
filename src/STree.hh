#pragma once

#include <typeinfo>
#include <iostream>

#include <unordered_set>

#include <algorithm>
#include <vector>
#include <cassert>

#include <boost/mpl/at.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/front_inserter.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/begin_end.hpp>

#include <boost/mpl/at.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>

// #include <boost/mpl/next_prior.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>

// #include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/mpl/aux_/na_fwd.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/mpl/assert.hpp>

#include "Util.hh"

#include "Tuple.hh"

#include "Common.hh"

#define xDEBUG_STREE

#include <cxxabi.h>


template <typename T>
std::string _getTypeName()
{
    char   buffer[10000];
    size_t length = 10000;
    int    status;
    abi::__cxa_demangle(typeid(T).name(),
                        buffer,
                        &length,
                        &status);
    buffer[length] = 0;
    return std::string(buffer);
}


using namespace boost;

namespace stree {

typedef uint8_t LevelOffset;

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------

template <typename STreeType, typename IteratorType>
struct UpdateProcedure;


template <typename STreeType, typename IteratorType, typename Visitor>
struct QueryProcedure;

//-----------------------------------------------------------------------------
// Entry
//-----------------------------------------------------------------------------

struct Entry
{
    enum Type { POINT_AND_OFFSET,  RANGE };

    Entry(void *address,     LevelOffset target_level_offset);
    Entry(void *min_address, void *max_address);

    Type type;

    void *address;
    LevelOffset  target_level_offset;

    void *min_address;
    void *max_address;

};

//-----------------------------------------------------------------------------
// Query
//-----------------------------------------------------------------------------

template <typename STree>
struct Query
{
    static const int Dimension = STree::Dimension;

    typedef STree                            StructureType;
    typedef typename STree::AddressTypeList  AddressTypeList;
    typedef typename STree::Types            Types;

    Query();
    ~Query();

    template <typename Index>
    std::vector<Entry>& getEntries();

    void clear();

    template <typename Address>
    void add(Address &addr, LevelOffset target_level_offset=0);

    template <typename Address>
    void add(Address &min_address, Address &max_address);

    // the type will define which dimension we are talking about
    template <typename Address>
    const Address& getCurrentAddress();

    // the type will define which dimension we are talking about
    template <typename Address>
    const Address& getCurrentBaseAddress();

    // the type will define which dimension we are talking about
    template <typename Address>
    void setCurrentAddress(Address &addr);


    // the type will define which dimension we are talking about
    template <typename Address>
    void setCurrentBaseAddress(Address &addr);

//    void compile()
//    {
//        for (int i=0;i<Dimension;i++)
//            if (entries[i].size() == 0)
//                entries[i].push_back( boost::mpl::at<typename STree::Types, boost::mpl::int_<i>>::type::AddressType()  );
//
//    }

    std::vector<void*> base_addresses;
    std::vector<void*> addresses;
    std::vector<std::vector<Entry>> entries;

};




//-----------------------------------------------------------------------------
// Metafunctions: addressType
//-----------------------------------------------------------------------------

template <typename T>
struct addressType
{
    typedef typename T::AddressType type;
};

//-----------------------------------------------------------------------------
// STree
//-----------------------------------------------------------------------------

// assuming TypeList is a boost::mpl::vector
template <typename TypeList, typename Data>
struct STree {

    typedef TypeList                               Types;

    typedef typename mpl::transform< Types, addressType<mpl::_> >::type AddressTypeList;

    // tuple needs a boost::mpl::list. _AuxAddressList is a mpl::list version of AddressTypeList (a mpl::vector)
    typedef typename mpl::reverse_fold<AddressTypeList, mpl::list<>, mpl::push_front<mpl::_1, mpl::_2> >::type _AuxAddressList;

    typedef tuple::Tuple<_AuxAddressList>          AddressType;

    typedef typename mpl::size<Types>::type        DimensionType;

    typedef typename mpl::reverse<TypeList>::type  ReverseTypes;

    typedef typename mpl::front<TypeList>::type    FrontType;
    typedef typename mpl::back<TypeList>::type     BackType;

    typedef typename mpl::begin<TypeList>::type    BeginIteratorType;

    typedef Data                                   DataType;

    static const int Dimension = DimensionType::value;

public:

    static uint64_t count_proper_leaf_unique_adds; // unique addresses seen
    static int      add_flag; // unique addresses seen

public:

    STree();
    ~STree();

    // set address
//    template <typename Address>
//    void setAddress(Address &addr);

    // find address and add data
    void add(AddressType &address, Data data);

    template <typename Visitor>
    void visit(Query<STree> &query, Visitor &visitor);

    // count
    CountRecord count();

public: // this should not be used by the client user

    FrontType root; // root of the STree

    Data data;

//    std::vector<void*> addresses;

};

template <typename TypeList, typename Data>
uint64_t STree<TypeList, Data>::count_proper_leaf_unique_adds = 0;

template <typename TypeList, typename Data>
int STree<TypeList, Data>::add_flag = 0;

//-----------------------------------------------------------------------------
// UpdateProcedure
//-----------------------------------------------------------------------------

template <typename STreeType, typename IteratorType>
struct UpdateProcedure
{
    typedef typename mpl::deref<IteratorType>::type                  CurrentStructureType;
    typedef typename mpl::next<IteratorType>::type                   NextIteratorType;
    typedef typename mpl::deref<NextIteratorType>::type              NextStructureType;

    typedef typename STreeType::AddressType                          AddressType;
    typedef typename STreeType::DataType                             DataType;

    static const int Index = IteratorType::pos::value;

    static const bool LAST_DIMENSION = is_same<CurrentStructureType, typename STreeType::BackType >::value;

    UpdateProcedure(AddressType               &address,
                    CurrentStructureType      &current_structure,
                    CurrentStructureType      *child_structure,
                    DataType                   data,
                    std::vector<void*>        &updated_contents);

    std::vector<void*> &updated_contents;
};

//-----------------------------------------------------------------------------
// QueryProcedure
//-----------------------------------------------------------------------------

template <typename STreeType, typename IteratorType, typename Visitor>
struct QueryProcedure
{
    typedef typename mpl::deref<IteratorType>::type      CurrentStructureType;
    typedef typename mpl::next<IteratorType>::type       NextIteratorType;
    typedef typename mpl::deref<NextIteratorType>::type  NextStructureType;

    typedef typename STreeType::DimensionType            DimensionType;

    typedef typename CurrentStructureType::AddressType   AddressType;
    typedef typename CurrentStructureType::NodeType      NodeType;


    typedef typename IteratorType::pos Index;

    static const bool LAST_DIMENSION = is_same<CurrentStructureType, typename STreeType::BackType >::value;

    explicit
    QueryProcedure(CurrentStructureType &current_structure,
                   Query<STreeType>     &query,
                   Visitor              &visitor);

    // local visit
    void visit(NodeType *node, AddressType &addr);


public: // attributes

    // callback object
    Query<STreeType> &query;
    Visitor          &visitor;

};

//-----------------------------------------------------------------------------
// Impl. UpdateProcedure Template Members
//-----------------------------------------------------------------------------

template <typename STreeType, typename IteratorType, typename Visitor>
QueryProcedure<STreeType, IteratorType, Visitor>::QueryProcedure( CurrentStructureType &current_structure,
                                                                  Query<STreeType> &query,
                                                                  Visitor &visitor ):
    query(query),
    visitor(visitor)
{
    // typedef typename Query<STreeType>::Entry Entry;
    // std::clog << "QueryProcedure: create " << std::endl;

    auto &entries = query.template getEntries<Index>();

    // std::clog << "    |entries| = " << entries.size() << std::endl;

    if (entries.size() == 0)
    {
        // std::clog << "   Iterate through entries at level: " << Index::value << std::endl;

        AddressType addr;

        // set base address
        query.setCurrentBaseAddress(addr);

        // send signal
        visitor.changeBaseAddress(Index::value, query);

        // visit subnodes
        current_structure.visitSubnodes(addr, 0, *this);

        // std::clog << "   End entry: " << addr << std::endl;

    }
    else {

        for (auto &e: entries)
        {

            if (e.type == Entry::POINT_AND_OFFSET)
            {

                AddressType& addr = *reinterpret_cast<AddressType*>(e.address);

                // std::clog << "   Iterate through entries at level: " << Index::value << " address " << addr << " entries.size() " << entries.size() << std::endl;

                // set base address
                query.setCurrentBaseAddress(addr);

                // send signal
                visitor.changeBaseAddress(Index::value, query);

                // visit subnodes
                current_structure.visitSubnodes(addr, e.target_level_offset, *this);

                // std::clog << "   End entry: " << addr << std::endl;
            }
            else if (e.type == Entry::RANGE)
            {

                AddressType& min_address = *reinterpret_cast<AddressType*>(e.min_address);
                AddressType& max_address = *reinterpret_cast<AddressType*>(e.max_address);

                // std::clog << "   Iterate through entries at level: " << Index::value << " address " << addr << " entries.size() " << entries.size() << std::endl;

                // set base address
                AddressType addr;
                query.setCurrentBaseAddress(addr);

                // send signal
                visitor.changeBaseAddress(Index::value, query);

                // visit subnodes
                current_structure.visitRange(min_address, max_address, *this);

                // std::clog << "   End entry: " << addr << std::endl;
            }
        }

    }
    // std::clog << "   Done with QueryProcedure: " << Index::value << std::endl;
}


namespace query {

template<bool B = false>
struct Helper {
    template <typename NextIteratorType, typename Content, typename Query, typename Visitor>
    static inline void exec(Content &content, Query &query, Visitor &visitor)
    {
        typedef typename Content::NodeType NodeType;

        // std::cout << "visiting" << std::endl;
        // recurse if needed, maybe use a helper here
        //assert(node->getContent());
        //NextStructureType &next_structure = *node->getContent();

        QueryProcedure< typename Query::StructureType, NextIteratorType, Visitor> q(content, query, visitor);

    }
};

template<>
struct Helper<true>
{
    template <typename NextIteratorType, typename Content, typename Query, typename Visitor>
    static inline void exec(Content &content, Query &query, Visitor &visitor)
    {
        // content.add(stree.data);
        visitor.visit(content, query);
    }
};

}


// local visit
template <typename STreeType, typename IteratorType, typename Visitor>
void QueryProcedure<STreeType, IteratorType, Visitor>::visit(NodeType *node, AddressType &addr)
{
    // std::clog << "QueryProcedure: visit " << std::endl;

    query.setCurrentAddress(addr);

    assert(node->getContent());
    query::Helper<LAST_DIMENSION>::template exec<NextIteratorType>(*node->getContent(), query, visitor);

//    std::cout << "visiting" << std::endl;
//    // recurse if needed, maybe use a helper here
//    NextStructureType &next_structure = *node->getContent();
//    QueryProcedure< STreeType, NextIteratorType, Visitor> q(next_structure, query, visitor);

}



//-----------------------------------------------------------------------------
// Impl. STree Template Members
//-----------------------------------------------------------------------------

template <typename TypeList, typename Data>
template <typename Visitor>
void
STree<TypeList,Data>::visit(Query<STree> &query, Visitor &visitor)
{
    QueryProcedure<
            STree<TypeList,Data>,
            BeginIteratorType,
            Visitor> q(root, query, visitor);
}

template <typename TypeList, typename Data>
STree<TypeList,Data>::STree()
{
    // std::cout << "Constructing STree: " << (unsigned long) this << std::endl;
    // std::cout << "    Root type: " << typeid(root).name() << std::endl;

    // initialize addresses as empty
    // addresses.resize(Dimension,(void*) 0);
}


template <typename TypeList, typename Data>
STree<TypeList,Data>::~STree()
{
    // std::cout << "deleting stree " << (unsigned long) this << std::endl;
}


template <typename TypeList, typename Data>
void STree<TypeList,Data>::add(AddressType &address, Data data)
{
    static std::vector<void*> updated_content(256);
    updated_content.clear();
    UpdateProcedure<
            STree<TypeList,Data>,
            STree<TypeList,Data>::BeginIteratorType> update(address, root, nullptr, data, updated_content);
}


// collect stree statistics
template <typename TypeList, typename Data>
CountRecord STree<TypeList,Data>::count()
{
    return root.count();
}


//-----------------------------------------------------------------------------
// Helper class template for UpdateProcedure
//-----------------------------------------------------------------------------

namespace update {

template<bool B = false>
struct Helper {
    template <typename NextIteratorType, typename STreeType, typename Content, typename CurrentAddressType>
    static inline void exec(typename STreeType::AddressType& address,
                            Content&                         content,
                            Content*                         parallel_content,
                            typename STreeType::DataType     data,
                            std::vector<void*>       &updated_contents)
    {
        // std::cout << "exec (non-base case): " << _getTypeName<typename CurrentStructureType::AddressType>() << std::endl;

        UpdateProcedure<STreeType, NextIteratorType> update(address, content, parallel_content, data, updated_contents);

        // insert content to list of updated contents
        updated_contents.push_back(static_cast<void*>(&content));
    }
};

template<>
struct Helper<true>
{
    template <typename NextIteratorType, typename STreeType, typename Content, typename CurrentAddressType>
    static inline void exec(typename STreeType::AddressType& address,
                            Content&                         content,
                            Content*                         parallel_content,
                            typename STreeType::DataType     data,
                            std::vector<void*>       &updated_contents)
    {
        // here we add and count if this is a new proper address never seen before
        // if (STreeType::add_flag == 0)
        // std::cout << "Proper add";
        // int added_bytes = content.add(data);
        // std::cout << "Added Bytes: " << added_bytes;

        // count unique keys based on STreeType flags
        if (content.add(data) > 0 && STreeType::add_flag == 0) {
//            std::cout << " Yes a proper point!";
            STreeType::count_proper_leaf_unique_adds++;
        }
//        std::cout << std::endl;

        // insert content to list of updated contents
        updated_contents.push_back(static_cast<void*>(&content));

#ifdef DEBUG_STREE
        // uint32_t *aux = (uint32_t*) &address.template get<typename STreeType::AddressType::Head>();
        // if (*(aux + 2) == 3) {
        char *ptr = (char*) &address.template get<CurrentAddressType>();
        // std::cout << "exec (base case): " << _getTypeName<typename CurrentStructureType::AddressType>() << std::endl;
        //  }
        int count = content.entries.back().count();
        std::cout << "Add " << data << " of kind " << (int) *ptr << " content is " << &content << " count " << count << std::endl;
#endif

    }
};

}


#ifdef DEBUG_STREE
static std::string bin(uint32_t x, int length) {
    std::stringstream ss;
    uint32_t aux = x;
    int i = 0;
    while (aux > 0) {
        // std::cout << aux << std::endl;
        ss << (int) (0x1 & aux);
        aux >>= 1;
        i++;
    }

    while(i < length) {
        ss << "0";
        i++;
    }

    return ss.str();
}
#endif


//-----------------------------------------------------------------------------
// UpdateProcedure implementations
//-----------------------------------------------------------------------------

template <typename STreeType, typename IteratorType>
UpdateProcedure<STreeType, IteratorType>::UpdateProcedure(
        AddressType                     &address,
        CurrentStructureType            &current_structure,
        CurrentStructureType            *child_structure,
        DataType                        data,
        std::vector<void*>      &updated_contents):
    updated_contents(updated_contents)
{

#ifdef DEBUG_STREE
    using datatiles::util::fl;
    using datatiles::util::fr;
    using datatiles::util::str;

    int index = Index;

    std::clog << fl("",3*index)
              << "UpdateProcedure(...) index: "
              << index
              << ", current_structure: "
              << static_cast<void*>(&current_structure)
              << std::endl;
#endif
    typedef typename CurrentStructureType::AddressType CurrentAddressType;

    // TODO: maybe can be a bit inneficient for many dimensions, but much cleaner and type safe
    CurrentAddressType &current_dimension_address = address.template get<CurrentAddressType>();

    static typename CurrentStructureType::NodeStackType stack;
    stack.clear();

    // trail proper path
    // the recursive calls or final data delivery call should be
    // on proper content for each node
    current_structure.prepareProperOutdatedPath(child_structure, current_dimension_address, stack);

    //
    typename CurrentStructureType::NodeType *child = nullptr;

#ifdef DEBUG_STREE
    std::clog << fl("",3*index)
              << "|- stack size: "
              << stack.size()
              << std::endl;
#endif

    //
    // These flags are used to count number of unique keys inserted
    //
    // Revision needed, since prepareProperOutdatedPath might not return
    // deepest node because of sharing
    //

    // set dimension flag
    static const int Index = IteratorType::pos::value;
    static const int mask_off  = 1 << Index; // make add_flag Index bit 1 to turn off 'proper flag' (or  with this flag)
    static const int mask_on   = ~mask_off;   // make add_flag Index bit 0 to turn on  'proper flag' (and with this flag)
    // add_flag IteratorType

    // make proper flag on
    if (stack.size() == current_dimension_address.getPathSize()) {
        STreeType::add_flag &= mask_on;
    }
    else {
        STreeType::add_flag |= mask_off;
    }


    int stack_item = -1;
    while (!stack.empty())
    {
        typename CurrentStructureType::NodeType *parent = stack.back();
        stack.pop_back();
        stack_item++;

        if (parent->getNumChildren() == 1)
        {
#ifdef DEBUG_STREE
            std::clog << fl("",3*index)
                      << "|- stack_item: "
                      << stack_item << " node " << static_cast<void*>(parent)
                      << " case B: one child and share content"
                      << "    [flag: " << bin(STreeType::add_flag, STreeType::Dimension) << "]"
                      << std::endl;
#endif

            // just share the content
            parent->setSharedContent(child->getContent()); // sharing content with children
        }

        else
        {

            typedef typename CurrentStructureType::ContentType ContentType;


            // the idea here is to check if shared content is in the list
            // of contents that already had the point added. If this is the
            // case do not dispatch forward. The sharing is still valid.
            // Otherwise create proper content and dispatch forward.

            if (parent->contentIsShared() &&
                std::find(updated_contents.begin(), updated_contents.end(),
                          parent->getContent())!=updated_contents.end()) {
                // updated_contents.count(parent->getContent()) > 0) {
                // proof that we are sharing a content that was already
                // updated, so the sharing is still valid.

#ifdef DEBUG_STREE
            std::clog << fl("",3*index)
                      << "|- stack_item: "
                      << stack_item << " node " << static_cast<void*>(parent)
                      << " case C: found a proof that shared content was already updated"
                      << "    [flag: " << bin(STreeType::add_flag, STreeType::Dimension) << "]"
                      << std::endl;
#endif

                // continue;
            }
            else {

#ifdef DEBUG_STREE
            std::clog << fl("",3*index)
                      << "|- stack_item: "
                      << stack_item << " node " << static_cast<void*>(parent)
                      << " case A: needs a proper content "
                      << (parent->contentIsShared() ? "NEW ONE" : "ONE ALREADY EXISTS!")
                      << "    [flag: " << bin(STreeType::add_flag, STreeType::Dimension) << "]"
                      << std::endl;
#endif

                ContentType &content = *parent->getProperContentCreateIfNeeded();

                ContentType *parallel_content = child ? child->getContent() : nullptr;

                update::Helper<LAST_DIMENSION>::template exec<NextIteratorType, STreeType, ContentType, CurrentAddressType>
                        (address, content, parallel_content, data, updated_contents);
            }
        }

        child = parent;

        STreeType::add_flag |= mask_off; // i-th bit is now 1 indicating we are not in a proper address

    }

}




//-----------------------------------------------------------------------------
// Impl. Query Template Members
//-----------------------------------------------------------------------------

template <typename STree>
Query<STree>::Query()
{
    entries.resize(Dimension);

    // current address
    addresses.resize(Dimension);
    std::fill(addresses.begin(), addresses.end(), nullptr);
    base_addresses.resize(Dimension);
    std::fill(base_addresses.begin(), base_addresses.end(), nullptr);
}

template <typename STree>
Query<STree>::~Query()
{
    clear();
}

//template <typename STree>
//template <int Index, bool Flag=true>
//void Query<STree>::clear_i()
//{
//    typedef typename mpl::at_c<AddressTypeList, Index>::type Address;
//    for (Entry &e: entries[Index]) {
//        Address *a = (Address*) e.address;
//        delete a;
//    }
//    entries[Index].clear();

//    // recurse only if Index > 0
//    static const bool flag = (Index > 0);
//    clear_i<Index-1, flag>();
//}

//template <typename STree>
//template <int Index>
//void Query<STree>::clear_i<Index,false>()
//{}

template <bool Flag=true>
struct QueryClearHelp {
    template <typename Query, int Index>
    static void clear(Query &query) {
        typedef typename mpl::at_c<typename Query::AddressTypeList, Index>::type Address;
        for (Entry &e: query.entries[Index]) {
            if (e.type == Entry::RANGE) {
                delete reinterpret_cast<Address*>(e.min_address);
                delete reinterpret_cast<Address*>(e.max_address);
            }
            else if (e.type == Entry::POINT_AND_OFFSET)
            {
                delete reinterpret_cast<Address*>(e.address);
            }
//            e.deleteAddresses();
        }
        query.entries[Index].clear();

        // recurse only if Index > 0
        static const bool flag = (Index > 0);
        QueryClearHelp<flag>::template clear<Query, Index-1>(query);
    }
};

template <>
struct QueryClearHelp<false> {
    template <typename Query, int Index>
    static void clear(Query &query) {}
};


template <typename STree>
void Query<STree>::clear()
{
    // TODO: not robust if stree has dimension zero
    QueryClearHelp<true>::clear<Query<STree>, Dimension-1>(*this);
    //    clear_i<Dimension>();
    //    for (size_t i=0;i<STree::Dimension;i++) {
    //        clear_i<i>();
    //        // entries[i].clear();
    //    }
}

template <typename STree>
template <typename Index>
std::vector<Entry>& Query<STree>::getEntries()
{
    return entries[Index::value];
}

template <typename STree>
template <typename Address>
void Query<STree>::add(Address &addr, LevelOffset target_level_offset)
{
    typedef typename mpl::find<AddressTypeList, Address>::type iter;
    // typedef typename mpl::deref<iter>::type::StructureType    DimTypeAtIndex;

    // typedef typename mpl::at<Types, typename iter::pos>::type           DimTypeAtIndex;
    // typedef typename boost::mpl::at<Types, boost::mpl::int_<index>>::type DimTypeAtIndex;
    // typedef typename DimTypeAtIndex::AddressType AddressTypeAtIndex;
    // BOOST_MPL_ASSERT((is_same<Address, AddressTypeAtIndex>));
    // std::cout << "Found Address: " << iter::pos::value << std::endl;
    Address *addr_aux = new Address(addr); // TODO Memory Leak!

    // std::cout << "Adding address " << *addr_aux << std::endl;

    entries[iter::pos::value].push_back(Entry(static_cast<void*>(addr_aux), target_level_offset));
}

template <typename STree>
template <typename Address>
void Query<STree>::add(Address &min_address, Address &max_address)
{
    typedef typename mpl::find<AddressTypeList, Address>::type iter;
    // typedef typename mpl::deref<iter>::type::StructureType    DimTypeAtIndex;

    // typedef typename mpl::at<Types, typename iter::pos>::type           DimTypeAtIndex;
    // typedef typename boost::mpl::at<Types, boost::mpl::int_<index>>::type DimTypeAtIndex;
    // typedef typename DimTypeAtIndex::AddressType AddressTypeAtIndex;
    // BOOST_MPL_ASSERT((is_same<Address, AddressTypeAtIndex>));
    // std::cout << "Found Address: " << iter::pos::value << std::endl;
    Address *min_address_aux = new Address(min_address); // TODO Memory Leak!
    Address *max_address_aux = new Address(max_address); // TODO Memory Leak!

    // std::cout << "Adding range " << *min_address_aux << ", " << *max_address_aux << std::endl;

    entries[iter::pos::value].push_back(Entry(static_cast<void*>(min_address_aux),
                                              static_cast<void*>(max_address_aux)));
}



template <typename STree>
template <typename Address>
const Address& Query<STree>::getCurrentAddress()
{
    typedef typename mpl::find<AddressTypeList, Address>::type iter;

    void *aux = addresses[iter::pos::value];
    assert(aux);

    return *reinterpret_cast<Address*>(aux);
}

template <typename STree>
template <typename Address>
const Address& Query<STree>::getCurrentBaseAddress()
{
    typedef typename mpl::find<AddressTypeList, Address>::type iter;

    void *aux = base_addresses[iter::pos::value];
    assert(aux);

    return *reinterpret_cast<Address*>(aux);
}

template <typename STree>
template <typename Address>
void Query<STree>::setCurrentAddress(Address &addr)
{
    typedef typename mpl::find<AddressTypeList, Address>::type iter;
    addresses[iter::pos::value] = &addr;
}

template <typename STree>
template <typename Address>
void Query<STree>::setCurrentBaseAddress(Address &addr)
{
    typedef typename mpl::find<AddressTypeList, Address>::type iter;
    base_addresses[iter::pos::value] = &addr;
}


} // end namespace stree
