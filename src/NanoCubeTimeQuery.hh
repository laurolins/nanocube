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

#include <vector>
#include <stack>
#include <stdint.h>
#include <stdexcept>

namespace nanocube {

namespace timequery {

//-----------------------------------------------------------------------------
// Query
//-----------------------------------------------------------------------------

template <typename NanoCube, int Index=0>
struct TimeQuery
{

public: // subtypes & class constants

    typedef ::query::QueryDescription                                query_description_type;
    typedef ::query::result::Result                                  query_result_type;

    typedef NanoCube                                                 nanocube_type;

    typedef typename nanocube_type::dimension_types                  dimension_types;
    typedef typename nanocube_type::entry_type                       entry_type;

    typedef TimeQuery<nanocube_type, Index>                          query_type;

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

    TimeQuery(dimension_type            &tree,
              query_description_type    &query_description,
              query_result_type         &result);

public: // methods

    void visit(dimension_node_type *node, const dimension_address_type &addr);

public: // methods

    ::query::QueryDescription &query_description;

    query_result_type &result;

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

    static void eval(dimension_content_type &content, query_description_type &qd, query_result_type &result) {

        TimeQuery<nanocube_type, query_type::DIMENSION_INDEX + 1> q(content, qd, result);

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
                     query_description_type &qd,
                     query_result_type      &result) {

        // content will be a time series and there will be a variable
        // index in which we are interested.

        // get the first variable for now (query_type::VARIABLE_INDEX)

        // context is stored in the result object???

        if (content.entries.size() == 0)
            throw ::nanocube::query::QueryException("No entries when a timeseries was expected");

        uint64_t a     = content.entries.front().template get<0>();
        uint64_t b     = content.entries.back().template get<0>();
        uint64_t count = content.entries.back().template get<1>();

        // it is an open interval [a,b)
        ::query::RawAddress addr = ((uint64_t) a << 32) + (b+1);
        result.push(addr);
        result.store(count, ::tree_store::ADD);
        result.pop();
    }
};

} // end namespace aux

//-----------------------------------------------------------------------------
// Query Impl.
//-----------------------------------------------------------------------------

template <typename NanoCube, int Index>
TimeQuery<NanoCube, Index>::TimeQuery(dimension_type             &tree,
                              query_description_type     &query_description,
                              query_result_type          &result):
    query_description(query_description),
    result(result),
    anchored(false),
    pushed(false)
{
    // context is stored in the result object???
    ::query::Target *target = query_description.targets[Index];

    TimeQuery &query = *this;

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
    else {
        throw std::exception();
    }

    if (pushed) {
        result.pop();
    }
}

template <typename NanoCube, int Index>
void TimeQuery<NanoCube, Index>::visit(dimension_node_type *node, const dimension_address_type &address) {

    // state
    if (query_description.anchors[Index]) {
        if (pushed) {
            result.pop();
            pushed = false;
        }
        result.push(address.raw());
        pushed = true;
    }

    dimension_content_type &content = *node->getContent();

    aux::Eval<query_type, (DIMENSION_INDEX == DIMENSION-1)>::eval(content, query_description, result);

}

} // end query namespace

} // nanocube namespace
