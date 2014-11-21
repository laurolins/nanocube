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

#include "Query.hh"
#include "QueryResult.hh"

#include "NanoCubeQueryException.hh"

#include "cache.hh"

#include "maps.hh"

#include <vector>
#include <stack>
#include <stdint.h>

#include <unordered_map>
#include <stdexcept>

namespace nanocube {

namespace query {

//-----------------------------------------------------------------------------
// Query
//-----------------------------------------------------------------------------

template <typename NanoCube, int Index=0>
struct Query
{

public: // subtypes & class constants

    typedef ::query::QueryDescription                                query_description_type;
    typedef ::query::result::Result                                  query_result_type;

    typedef NanoCube                                                 nanocube_type;

    typedef typename nanocube_type::dimension_types                  dimension_types;
    typedef typename nanocube_type::entry_type                       entry_type;

    typedef Query<nanocube_type, Index>                              query_type;

    typedef typename boost::mpl::at_c<dimension_types, Index>::type         dimension_type;
    typedef typename boost::mpl::at_c<dimension_types, Index+1>::type       next_dimension_type;

    typedef typename dimension_type::AddressType                     dimension_address_type;      // QuadTree or FlatTree
    typedef typename dimension_type::NodeType                        dimension_node_type;      // QuadTree or FlatTree
    typedef typename dimension_type::ContentType                     dimension_content_type;      // QuadTree or FlatTree

    static const int  DIMENSION = nanocube_type::DIMENSION;
    static const int  DIMENSION_INDEX = Index;

    // static const int  VARIABLE_INDEX = query_description_type::VARIABLE_INDEX;
    // static const bool LAST_DIMENSION = (DIMENSION_INDEX == DIMENSION-1);

public: // constructor

    Query(dimension_type            &tree,
          const query_description_type    &query_description,
          query_result_type         &result,
          Cache                     &cache);

public: // methods

    void visit(dimension_node_type *node, const dimension_address_type &addr);

public: // methods

    const ::query::QueryDescription &query_description;

    query_result_type &result;
    Cache             &cache;

    bool    anchored;
    bool    pushed;

};






//-----------------------------------------------------------------------------
// query auxiliar namespace
//-----------------------------------------------------------------------------

namespace aux {

template <typename query_type, bool Flag=false>
struct Eval {

    typedef typename query_type::query_description_type    query_description_type;
    typedef typename query_type::nanocube_type             nanocube_type;
    typedef typename query_type::query_result_type         query_result_type;
    typedef typename query_type::dimension_content_type    dimension_content_type;

    static void eval(dimension_content_type &content, const query_description_type &qd, query_result_type &result, Cache &cache) {

        Query<nanocube_type, query_type::DIMENSION_INDEX + 1> q(content, qd, result, cache);

        // query::Query<QueryDescriptionType> query(root, query_description, result);

    }
};

template <typename query_type>
struct Eval<query_type, true> {

    typedef typename query_type::query_description_type     query_description_type;
    typedef typename query_type::nanocube_type              nanocube_type;
    typedef typename query_type::query_result_type          query_result_type;
    typedef typename query_type::dimension_content_type     dimension_content_type;

    static void eval(dimension_content_type &content,
                     const query_description_type &qd,
                     query_result_type      &result,
                     Cache &cache) {

        // content will be a time series and there will be a variable
        // index in which we are interested.

        // get the first variable for now (query_type::VARIABLE_INDEX)

        // context is stored in the result object???
        ::query::Target* target = qd.targets[query_type::DIMENSION_INDEX + 1];

        bool anchored = qd.anchors[query_type::DIMENSION_INDEX + 1];

        if (target->type == ::query::Target::ROOT) { // simplest case

            if (anchored) {
                throw QueryException("Anchors on time dimension should use: base:width:count notation");
            }

            uint64_t last_value = content.entries.back().template get<1>();
            result.store(last_value, ::tree_store::ADD);


        }
        else if (target->type == ::query::Target::BASE_WIDTH_COUNT) {

//            if (!anchored) {
//                throw QueryException("Time dimension with base:width:count should be anchored");
//            }

            ::query::BaseWidthCountTarget &bwc_target = *target->asBaseWidthCountTarget();

            
            // TODO: check this please!!!
            
            uint32_t base  = (uint32_t) bwc_target.base;
            uint32_t width = (uint32_t) bwc_target.width;
            uint32_t count = (uint32_t) bwc_target.count;
            uint32_t a = base;
            for (uint32_t i=0;i<count;i++) {
                uint32_t b = a + width;
                uint64_t value = content.template getWindowTotal<1>(a,b);
                if (value != 0) {
                    if (anchored) {
                        // ::query::RawAddress addr = ((uint64_t) a << 32) + b;
                        std::vector<int> path { (int) i };
                        result.push(path);
                    }
                    result.store(value, ::tree_store::ADD);
                    if (anchored) {
                        result.pop();
                    }
                }
                a = b;
            }
        }
    }
};

} // end namespace aux

//-----------------------------------------------------------------------------
// Query Impl.
//-----------------------------------------------------------------------------

template <typename NanoCube, int Index>
Query<NanoCube, Index>::Query(dimension_type             &tree,
                              const query_description_type     &query_description,
                              query_result_type          &result,
                              Cache                      &cache):
    query_description(query_description),
    result(result),
    cache(cache),
    anchored(false),
    pushed(false)
{
    // context is stored in the result object???
    ::query::Target *target = query_description.targets[Index];

    Query &query = *this;

    if (target->type == ::query::Target::ROOT) { // simplest case

        // default constructor is the root address
        dimension_address_type root_address;

        // use the visitSubnodes interface
        tree.visitSubnodes(root_address, 0, query);

    }
    else if (target->type == ::query::Target::FIND_AND_DIVE) {
        ::query::FindAndDiveTarget &find_and_dive_target = *target->asFindAndDiveTarget();

        dimension_address_type base_address(find_and_dive_target.base);
        int dive_depth = find_and_dive_target.offset;

//        std::cout << "base_address: " << base_address << std::endl;
//        std::cout << "dive_depth: "   << dive_depth << std::endl;


        // use the visitSubnodes interface
        tree.visitSubnodes(base_address, dive_depth, query);

    }
    else if (target->type == ::query::Target::RANGE) {
        ::query::RangeTarget &range_target = *target->asRangeTarget();

        dimension_address_type min_address(range_target.min_address);
        dimension_address_type max_address(range_target.max_address);

//        std::cout << "min_address: " << min_address << std::endl;
//        std::cout << "max_address: " << max_address << std::endl;

        // use the visitSubnodes interface
        tree.visitRange(min_address, max_address, query);

    }
    else if (target->type == ::query::Target::SEQUENCE) {

        //
        // There is a difference here compared to the other
        // cases. We need to pre-process the sequence into a
        // "mask" to guide the traversal (visit with guide).
        // This pre-processing should be done only once
        // (think about the partitioning of a polygon into
        // convex shapes)
        //

        ::query::SequenceTarget &sequence_target = *target->asSequenceTarget();


//        std::cout << "min_address: " << min_address << std::endl;
//        std::cout << "max_address: " << max_address << std::endl;

        // use the visitSubnodes interface
        tree.visitSequence(sequence_target.addresses, query, cache);

//        ::query::RangeTarget &range_target = *target->asRangeTarget();

//        dimension_address_type min_address(range_target.min_address);
//        dimension_address_type max_address(range_target.max_address);
////        std::cout << "min_address: " << min_address << std::endl;
////        std::cout << "max_address: " << max_address << std::endl;
//        // use the visitSubnodes interface
//        tree.visitRange(min_address, max_address, query);
    }
    else if (target->type == ::query::Target::MASK) {
        
//        //
//        // There is a difference here compared to the other
//        // cases. We need to pre-process the sequence into a
//        // "mask" to guide the traversal (visit with guide).
//        // This pre-processing should be done only once
//        // (think about the partitioning of a polygon into
//        // convex shapes)
//        //
//        
//        
        ::query::MaskTarget &mask_target = *target->asMaskTarget();
//
//        
//        //        std::cout << "min_address: " << min_address << std::endl;
//        //        std::cout << "max_address: " << max_address << std::endl;
//        
//        // use the visitSubnodes interface
        tree.visitExistingTreeLeaves(mask_target.root, query);
//        
//        //        ::query::RangeTarget &range_target = *target->asRangeTarget();
//        
//        //        dimension_address_type min_address(range_target.min_address);
//        //        dimension_address_type max_address(range_target.max_address);
//        ////        std::cout << "min_address: " << min_address << std::endl;
//        ////        std::cout << "max_address: " << max_address << std::endl;
//        //        // use the visitSubnodes interface
//        //        tree.visitRange(min_address, max_address, query);
    }

    else {
        throw std::exception();
    }

    if (pushed) {
        result.pop();
    }
}

template <typename NanoCube, int Index>
void Query<NanoCube, Index>::visit(dimension_node_type *node, const dimension_address_type &address) {

    // state
    if (query_description.anchors[Index]) {
        if (pushed) {
            result.pop();
            pushed = false;
        }
        
        // address conversion
        if (query_description.img_hint[Index]) {
            // assume it is a dive target
            auto target = query_description.targets[Index];
            auto dive_target = target->asFindAndDiveTarget();
            if (dive_target) {
                auto raw_base  = dive_target->base;
                auto raw_child = address.raw();
                
                // assume these are quadtree addresses
                auto base_tile = ::maps::Tile(raw_base);
                auto child_tile = ::maps::Tile(raw_child);
                auto relative_tile = base_tile.relativeTile(child_tile);
                result.push(std::vector<int> { relative_tile.x.quantity, relative_tile.y.quantity });
            }
            else {
                throw std::runtime_error("\"img\" hint is only supported with a \"dive\" target");
            }
        }
        else {
            result.push(address.getDimensionPath());
        }
        pushed = true;
    }

    dimension_content_type &content = *node->getContent();

    aux::Eval<query_type, (DIMENSION_INDEX == DIMENSION-1)>::eval(content, query_description, result, cache);

}

} // end query namespace

} // nanocube namespace
