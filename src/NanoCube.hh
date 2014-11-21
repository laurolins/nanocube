#pragma once

#include <string>
#include <iostream>

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

#include <vector>

#include "Util.hh"
#include "Tuple.hh"
#include "QuadTree.hh"
#include "FlatTree.hh"
#include "FlatTreeN.hh"
#include "TimeSeries.hh"

#define xDEBUG_STREE

#include "TimeSeriesEntryType.hh"
#include "NanoCubeReportBuilder.hh"
#include "NanoCubeInsert.hh"
#include "NanoCubeQuery.hh"
#include "NanoCubeTimeQuery.hh"
#include "NanoCubeSchema.hh"

// boost pre-processor
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace mpl = boost::mpl;


//
// simple routine:
// given a list of types and a root type
//
//   t_1, t_2, t_3, t_4, ... , t_n  and tr
//
// let tt_i be a template class with one parameter
// derived from t_i. We want to produce a list:
//
//   l_1, l_2, ... , l_n
//
// such that
//
//   l_n = tt_n<tr>   (i=n)
//
//   l_i = tt_i<l_{i+1}>   (1<=i<n)
//

namespace nanocube {

//------------------------------------------------------------------------------
// Dimension Types
//------------------------------------------------------------------------------

//template <typename Content>
//struct QuadTree {};

//template <typename Content>
//struct FlatTree {};

//------------------------------------------------------------------------------
// Dimension Names
//------------------------------------------------------------------------------

static const int QUADTREE=0;
static const int FLATTREE=1;

// categorical dimension with 1 byte
struct c1  { static const int Kind = FLATTREE; static const int NumBytes = 1; };
struct c2  { static const int Kind = FLATTREE; static const int NumBytes = 2; };
struct c3  { static const int Kind = FLATTREE; static const int NumBytes = 3; };
struct c4  { static const int Kind = FLATTREE; static const int NumBytes = 4; };


// quadtrees levels
struct q0  { static const int Kind = QUADTREE; static const int Levels = 0 ; };
struct q1  { static const int Kind = QUADTREE; static const int Levels = 1 ; };
struct q2  { static const int Kind = QUADTREE; static const int Levels = 2 ; };
struct q3  { static const int Kind = QUADTREE; static const int Levels = 3 ; };
struct q4  { static const int Kind = QUADTREE; static const int Levels = 4 ; };
struct q5  { static const int Kind = QUADTREE; static const int Levels = 5 ; };
struct q6  { static const int Kind = QUADTREE; static const int Levels = 6 ; };
struct q7  { static const int Kind = QUADTREE; static const int Levels = 7 ; };
struct q8  { static const int Kind = QUADTREE; static const int Levels = 8 ; };
struct q9  { static const int Kind = QUADTREE; static const int Levels = 9 ; };
struct q10 { static const int Kind = QUADTREE; static const int Levels = 10; };
struct q11 { static const int Kind = QUADTREE; static const int Levels = 11; };
struct q12 { static const int Kind = QUADTREE; static const int Levels = 12; };
struct q13 { static const int Kind = QUADTREE; static const int Levels = 13; };
struct q14 { static const int Kind = QUADTREE; static const int Levels = 14; };
struct q15 { static const int Kind = QUADTREE; static const int Levels = 15; };
struct q16 { static const int Kind = QUADTREE; static const int Levels = 16; };
struct q17 { static const int Kind = QUADTREE; static const int Levels = 17; };
struct q18 { static const int Kind = QUADTREE; static const int Levels = 18; };
struct q19 { static const int Kind = QUADTREE; static const int Levels = 19; };
struct q20 { static const int Kind = QUADTREE; static const int Levels = 20; };
struct q21 { static const int Kind = QUADTREE; static const int Levels = 21; };
struct q22 { static const int Kind = QUADTREE; static const int Levels = 22; };
struct q23 { static const int Kind = QUADTREE; static const int Levels = 23; };
struct q24 { static const int Kind = QUADTREE; static const int Levels = 24; };
struct q25 { static const int Kind = QUADTREE; static const int Levels = 25; };
struct q26 { static const int Kind = QUADTREE; static const int Levels = 26; };
struct q27 { static const int Kind = QUADTREE; static const int Levels = 27; };
struct q28 { static const int Kind = QUADTREE; static const int Levels = 28; };
struct q29 { static const int Kind = QUADTREE; static const int Levels = 29; };

//------------------------------------------------------------------------------
// Variable Types: unsigned ints
//------------------------------------------------------------------------------

struct u1 { static const int size = 1; };
struct u2 { static const int size = 2; };
struct u3 { static const int size = 3; };
struct u4 { static const int size = 4; };
struct u5 { static const int size = 5; };
struct u6 { static const int size = 6; };
struct u7 { static const int size = 7; };
struct u8 { static const int size = 8; };


//------------------------------------------------------------------------------
// expand_flattree
//------------------------------------------------------------------------------

template <typename Content, int NumBytes>
struct expand_flattree {
    using type = flattree_n::FlatTree<NumBytes, Content>; // general implementation
};

template <typename Content>
struct expand_flattree<Content,1> {
    using type = flattree::FlatTree<Content>; // space optimized implementation
};

//------------------------------------------------------------------------------
// expand_dim_type
//------------------------------------------------------------------------------

template <typename Dim, int Kind>
struct expand_dim_type {};

template <typename Dim>
struct expand_dim_type<Dim, QUADTREE> {
    template <typename Content>
    struct content { using type = quadtree::QuadTree<Dim::Levels,Content>; };
};

template <typename Dim>
struct expand_dim_type<Dim, FLATTREE> {
    template <typename Content>
    struct content { using type = typename expand_flattree<Content, Dim::NumBytes>::type; };
};

//------------------------------------------------------------------------------
// mapDimensionNameToDimensionType
//------------------------------------------------------------------------------

template <typename Dim>
struct mapDimensionNameToDimensionType {
    template <typename Content>
    struct content {
        using type = typename expand_dim_type< Dim, Dim::Kind >::template content<Content>::type;
    };
};

//------------------------------------------------------------------------------
// Unfold
//------------------------------------------------------------------------------

template <typename dim_names, typename CurrentDimensionTypesList, typename Content, bool Done=false>
struct Unfold {

    typedef typename mpl::back<dim_names>::type current_dimension_name;

    typedef typename mpl::pop_back<dim_names>::type next_dimension_names_input_list;

    static const bool EmptyFlag = mpl::empty<next_dimension_names_input_list>::type::value;

    typedef typename mapDimensionNameToDimensionType<current_dimension_name>::template content<Content>::type current_dimension_type;

    typedef typename mpl::push_front<CurrentDimensionTypesList, current_dimension_type>::type current_dimension_types_list;

    typedef typename Unfold<next_dimension_names_input_list,
                            current_dimension_types_list,
                            current_dimension_type,
                            EmptyFlag>::type type;
};

template <typename dim_names, typename CurrentDimensionTypesList, typename Content>
struct Unfold<dim_names, CurrentDimensionTypesList, Content, true> {
    typedef CurrentDimensionTypesList type;
};

//------------------------------------------------------------------------------
// translateDimensionNamesToDimensionsTypes
//------------------------------------------------------------------------------

template<typename dim_names, typename LeafType>
struct translateDimensionNamesToDimensionsTypes {

    typedef typename mpl::vector<> EmptyList;

    static const bool EmptyFlag = mpl::empty<dim_names>::type::value;

    typedef typename Unfold<dim_names, EmptyList, LeafType, EmptyFlag>::type type;
};

//------------------------------------------------------------------------------
// Meta-function: addressType
//------------------------------------------------------------------------------

template <typename DimensionType>
struct extractDimensionAddressType
{
    typedef typename DimensionType::AddressType type;
};

//-----------------------------------------------------------------------------
// Meta-function buildAddressType
//-----------------------------------------------------------------------------

template <typename DimensionTypes>
struct buildAddressType {

    typedef typename mpl::transform< DimensionTypes, extractDimensionAddressType<mpl::_> >::type ReverseOfDimensionAddressTypeList;

    typedef typename mpl::reverse_fold< ReverseOfDimensionAddressTypeList, mpl::list<>, mpl::push_front<mpl::_1, mpl::_2> >::type DimensionAddressTypeList;

    typedef tuple::Tuple<DimensionAddressTypeList> type;

};

//------------------------------------------------------------------------------
// NanoCubeTemplate
//------------------------------------------------------------------------------

template <typename dim_names, typename var_types>
struct NanoCubeTemplate {

public: // subtypes

    typedef NanoCubeTemplate<dim_names, var_types> nanocube_type;

    typedef var_types variable_types;

    typedef TimeSeriesEntryType<variable_types> entry_type;

    typedef timeseries::TimeSeries<entry_type> time_series_type;

    typedef dim_names dimension_names;

    typedef typename translateDimensionNamesToDimensionsTypes<dim_names, time_series_type>::type dimension_types;

    typedef typename buildAddressType<dimension_types>::type address_type;

    typedef typename mpl::front<dimension_types>::type first_dimension_type;

    static const int DIMENSION = mpl::size<dimension_types>::type::value;

public: // STATIC a single object of this type per process is expected

    static uint64_t keys;

    static int flags; // used when inserting a new entry

public: // methods

    NanoCubeTemplate(Schema &schema);

    void add(address_type address, entry_type entry);

    bool add(std::istream &is);

    bool mountAddressFromStream(address_type &a, std::istream &is);

    void mountReport(report::Report &report);

    void query(const ::query::QueryDescription  &query_description,
               ::query::result::Result    &result);

    void timeQuery(::query::QueryDescription &query_description,
                   ::query::result::Result &result);

public:

    first_dimension_type root; // root of the nanocube

    Schema &schema;

};

template <typename A, typename B>
uint64_t NanoCubeTemplate<A, B>::keys = 0;

template <typename A, typename B>
int NanoCubeTemplate<A, B>::flags = 0;




//------------------------------------------------------------------------------
// Impl. NanoCubeTemplate
//------------------------------------------------------------------------------

template <typename List, bool Done=false>
struct Aux {

    typedef typename mpl::front<List>::type dimension_type;

    typedef typename dimension_type::AddressType dimension_address_type;

    typedef typename mpl::pop_front<List>::type next_list;

    static const bool next_list_is_empty = mpl::empty<next_list>::value;

    template <typename NanoCubeAddress>
    static bool process(NanoCubeAddress &a, std::istream &is) {
        dimension_address_type dim_address;
        bool ok = dim_address.read(is);
        if (!ok) {
            return false;
        }
        a.set(dim_address);
        return Aux<next_list, next_list_is_empty>::process(a,is);
        // set address for dimension dim
        // Aux::process<next_list,
    }

};

template <typename List>
struct Aux<List, true> {
    template <typename NanoCubeAddress>
    static bool process(NanoCubeAddress &a, std::istream &is) {
        return true;
    }
};

template <typename dim_names, typename var_types>
bool NanoCubeTemplate<dim_names, var_types>::mountAddressFromStream(address_type &a, std::istream &is) {
    static const bool empty = mpl::empty<dimension_types>::type::value;
    return Aux<dimension_types, empty>::process(a, is);
}

template <typename dim_names, typename var_types>
NanoCubeTemplate<dim_names, var_types>::NanoCubeTemplate(Schema &schema):
    schema(schema)
{}

template <typename dim_names, typename var_types>
void NanoCubeTemplate<dim_names, var_types>::add(address_type address, entry_type entry) {
    // debug
    // std::cout << a << " entry " << e << std::endl;

    typedef NanoCubeTemplate<dim_names, var_types> nanocube_type;
    typedef typename mpl::begin<dimension_types>::type iterator;

    static std::vector<void*> updated_content(1024);
    static std::vector<void*> replaced_nodes(1024);

    updated_content.clear();
    replaced_nodes.clear();

    insert::Insert<nanocube_type, iterator> insert_trigger(address, root, nullptr, entry, updated_content, replaced_nodes);

    // std::cout << "Added point" << std::endl;

}

template <typename dim_names, typename var_types>
bool NanoCubeTemplate<dim_names, var_types>::add(std::istream &is) {

    // loop through Address types reading
    // address raw data and building corresponding
    // adddress of the right kind
    address_type a;

    bool valid_address = mountAddressFromStream(a, is);

    if (!valid_address) {
        return false;
    }

    entry_type e;

    is.read(e.data, entry_type::total_size);
    if (!is) {
        return false;
    }

    this->add(a, e);

    return true;
}

template <typename dim_names, typename var_types>
void NanoCubeTemplate<dim_names, var_types>::mountReport(report::Report &report)
{

    typedef NanoCubeTemplate<dim_names, var_types> nanocube_type;
    //
    // iterator through all nodes of a dimension
    // callback visit every find we can find a
    // new proper path.
    //
    // at this point we can add the new node
    // check if its content is proper or simply link
    //
    // scan_dimension(0, root, visitor);

    // starting with the root structure

    ReportBuilder<nanocube_type, 0> report_builder(root, report);

    report.updateToHumanReadableKeys();
    report.updateLayerIndices();

}

template <typename dim_names, typename var_types>
void NanoCubeTemplate<dim_names, var_types>::query(
        const ::query::QueryDescription  &query_description,
        ::query::result::Result    &result)
{
    Cache cache; // caches only within a single query
    query::Query<nanocube_type> query(root, query_description, result, cache);
}

template <typename dim_names, typename var_types>
void NanoCubeTemplate<dim_names, var_types>::timeQuery(
        ::query::QueryDescription  &query_description,
        ::query::result::Result    &result)
{
    timequery::TimeQuery<nanocube_type> tquery(root, query_description, result);
}


} // end of report namespace
