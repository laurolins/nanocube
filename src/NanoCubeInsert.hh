#pragma once

#include <boost/mpl/if.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>

// #include <boost/mpl/reverse.hpp>
// #include <boost/mpl/begin_end.hpp>
// #include <boost/type_traits/is_same.hpp>

#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/list.hpp>

#include <boost/type_traits/is_same.hpp>

namespace mpl = boost::mpl;

#define xDEBUG_STREE

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


namespace nanocube {

namespace insert {

//-----------------------------------------------------------------------------
// NanoCubeInsert
//-----------------------------------------------------------------------------

template <typename nanocube, typename iterator>
struct Insert
{

public: // subtypes & class constants

    typedef typename mpl::deref<iterator>::type                      dimension_type;      // QuadTree or FlatTree
    typedef typename mpl::next<iterator>::type                       next_iterator;
    typedef typename mpl::deref<next_iterator>::type                 next_dimension_type;

    typedef nanocube                                                 nanocube_type;

    typedef typename dimension_type::ContentType                     dimension_content_type;      // QuadTree or FlatTree

    typedef typename nanocube::address_type                          address_type;
    typedef typename nanocube::entry_type                            entry_type;
    typedef typename nanocube::dimension_types                       dimension_types;
    typedef typename mpl::back<dimension_types>::type                last_dimension_type;

    static const bool LAST_DIMENSION = boost::is_same<dimension_type, last_dimension_type>::value; // flag

    static const int Index = iterator::pos::value;

public: // constructor

    Insert(address_type               &address,
           dimension_type             &current_structure,
           dimension_type             *child_structure,    // parallel structure might be used for sharing
           entry_type                  entry,
           std::vector<void*>         &updated_contents,
           std::vector<void*>         &replaced_nodes);

public: // members

    std::vector<void*> &updated_contents;

};


//-----------------------------------------------------------------------------
// Helper class template for NanoCubeInsert
//-----------------------------------------------------------------------------

template<bool B = false>
struct Helper {

    template <typename nanocube_insert>
    static inline void exec(typename nanocube_insert::address_type           &address,
                            typename nanocube_insert::dimension_content_type &content,
                            typename nanocube_insert::dimension_content_type *parallel_content,
                            typename nanocube_insert::entry_type              entry,
                            std::vector<void*>                               &updated_contents,
                            std::vector<void*>                               &replaced_nodes)
    {
        typedef typename nanocube_insert::nanocube_type nanocube_type;
        typedef typename nanocube_insert::next_iterator next_iterator;

        // std::cout << "exec (non-base case): " << _getTypeName<typename CurrentStructureType::AddressType>() << std::endl;
        Insert<nanocube_type, next_iterator> aux(address, content, parallel_content, entry, updated_contents, replaced_nodes);

        // insert content to list of updated contents
        updated_contents.push_back(static_cast<void*>(&content));
    }
};


template<>
struct Helper<true>
{
    template <typename nanocube_insert>
    static inline void exec(typename nanocube_insert::address_type           &address,
                            typename nanocube_insert::dimension_content_type &content,
                            typename nanocube_insert::dimension_content_type *parallel_content,
                            typename nanocube_insert::entry_type              entry,
                            std::vector<void*>                               &updated_contents,
                            std::vector<void*>                               &replaced_nodes)
    {
        typedef typename nanocube_insert::nanocube_type nanocube;

        // count unique keys based on STreeType flags
        if (content.add(entry) > 0 && nanocube::flags == 0) {
//            std::cout << " Yes a proper point!";
            nanocube::keys++;
        }


//        std::cout << std::endl;

        // insert content to list of updated contents
        updated_contents.push_back(static_cast<void*>(&content));
    }
};


//-----------------------------------------------------------------------------
// Insert Impl.
//-----------------------------------------------------------------------------

template <typename nanocube, typename iterator>
Insert<nanocube, iterator>::Insert(address_type               &address,
                                   dimension_type             &current_structure,
                                   dimension_type             *child_structure,    // parallel structure might be used for sharing
                                   entry_type                  entry,
                                   std::vector<void*>         &updated_contents,
                                   std::vector<void*>         &replaced_nodes):
    updated_contents(updated_contents)
{

    typedef Insert<nanocube, iterator> nanocube_insert_type;
    typedef typename dimension_type::NodeType dimension_node_type;
    typedef typename dimension_type::ContentType dimension_content_type;

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
    typedef typename dimension_type::AddressType dimension_address_type;

    // TODO: maybe can be a bit inneficient for many dimensions, but much cleaner and type safe
    dimension_address_type &dimension_address =
            address.template get<dimension_address_type>();

    static typename dimension_type::NodeStackType stack;
    stack.clear();

    // trail proper path
    // the recursive calls or final data delivery call should be
    // on proper content for each node
    current_structure.prepareProperOutdatedPath(child_structure, dimension_address, replaced_nodes, stack);

    // the top of the stack is a node that is already updated or a nullptr
    typename dimension_type::NodeType* child = stack.back();
    stack.pop_back();

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
    static const int Index = iterator::pos::value;
    static const int mask_off  = 1 << Index; // make flags Index bit 1 to turn off 'proper flag' (or  with this flag)
    static const int mask_on   = ~mask_off;   // make flags Index bit 0 to turn on  'proper flag' (and with this flag)
    // flags IteratorType

    // make proper flag on
    if (stack.size() == dimension_address.getPathSize()) {
        nanocube_type::flags &= mask_on;
    }
    else {
        nanocube_type::flags |= mask_off;
    }


    int stack_item = -1;
    while (!stack.empty())
    {
        dimension_node_type *parent = stack.back();
        stack.pop_back();
        stack_item++;

        if (parent->getNumChildren() == 1)
        {
#ifdef DEBUG_STREE
            std::clog << fl("",3*index)
                      << "|- stack_item: "
                      << stack_item << " node " << static_cast<void*>(parent)
                      << " case B: one child and share content"
                      << "    [flag: " << bin(nanocube_type::flags, nanocube_type::DIMENSION) << "]"
                      << std::endl;
#endif

            // just share the content
            parent->setSharedContent(child->getContent()); // sharing content with children
        }

        else
        {
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
                          << "    [flag: " << bin(nanocube_type::flags, nanocube_type::DIMENSION) << "]"
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
                          << "    [flag: " << bin(nanocube_type::flags, nanocube_type::DIMENSION) << "]"
                          << std::endl;
#endif

                // std::cout << "parent content == " << parent->getContent() << " shared: " << parent->contentIsShared() << std::endl;

                dimension_content_type &content = *parent->getProperContentCreateIfNeeded();

                dimension_content_type *parallel_content = child ? child->getContent() : nullptr;

                insert::Helper<LAST_DIMENSION>::template exec<nanocube_insert_type>
                        (address, content, parallel_content, entry, updated_contents, replaced_nodes);
            }
        }

        child = parent;

        nanocube_type::flags |= mask_off; // i-th bit is now 1 indicating we are not in a proper address

    }

}

} // insert namespace

} // nanocube namespace
