#pragma once

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

#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/list.hpp>

#include <boost/type_traits/is_same.hpp>

namespace mpl = boost::mpl;

namespace nanocube {

//------------------------------------------------------------------------------
// TimeSeries
//------------------------------------------------------------------------------

//template <typename TimeSeriesEntryType>
//struct TimeSeries {};

//------------------------------------------------------------------------------
// Sum
//------------------------------------------------------------------------------

template <typename ListVarTypes, int PreviousSum, bool Done=false>
struct Sum {

    typedef typename mpl::front<ListVarTypes>::type current_var_type;

    static const int current_var_size = current_var_type::size;

    static const int current_sum_of_sizes = PreviousSum + current_var_size;

    typedef typename mpl::pop_front<ListVarTypes>::type next_list_of_var_types;

    static const bool next_list_of_var_types_is_empty = mpl::empty<next_list_of_var_types>::type::value;

    static const int value = Sum<next_list_of_var_types,
                                 current_sum_of_sizes,
                                 next_list_of_var_types_is_empty>::value;
};

template <typename ListVarTypes, int CurrentSum>
struct Sum<ListVarTypes, CurrentSum, true> {
    static const int value = CurrentSum;
};

//------------------------------------------------------------------------------
// Offset
//------------------------------------------------------------------------------

template <typename ListVarTypes, typename CurrentCumulativeSum, int PreviousSum, bool Done=false>
struct Offset {

    typedef typename mpl::front<ListVarTypes>::type current_var_type;

    static const int current_var_size = current_var_type::size;

    static const int current_sum_of_sizes = PreviousSum + current_var_size;

    typedef typename mpl::pop_front<ListVarTypes>::type next_list_of_var_types;

    typedef typename mpl::push_back<CurrentCumulativeSum, mpl::int_<PreviousSum>>::type current_offsets;

    static const bool next_list_of_var_types_is_empty = mpl::empty<next_list_of_var_types>::type::value;

    typedef typename Offset< next_list_of_var_types,
                             current_offsets,
                             current_sum_of_sizes,
                             next_list_of_var_types_is_empty>::type type;

};

template <typename ListVarTypes, typename CurrentCumulativeSum, int PreviousSum>
struct Offset<ListVarTypes, CurrentCumulativeSum, PreviousSum, true> {

    typedef typename mpl::push_back<CurrentCumulativeSum, mpl::int_<PreviousSum>>::type type;

};

//------------------------------------------------------------------------------
// Sizes
//------------------------------------------------------------------------------

template <typename ListVarTypes, typename CurrentSizes, bool Done=false>
struct Size {

    typedef typename mpl::front<ListVarTypes>::type current_var_type;

    static const int current_var_size = current_var_type::size;

    typedef typename mpl::pop_front<ListVarTypes>::type next_list_of_var_types;

    typedef typename mpl::push_back<CurrentSizes, mpl::int_<current_var_size>>::type current_sizes;

    static const bool next_list_of_var_types_is_empty = mpl::empty<next_list_of_var_types>::type::value;

    typedef typename Size< next_list_of_var_types,
                           current_sizes,
                           next_list_of_var_types_is_empty>::type type;

};

template <typename ListVarTypes, typename CurrentSizes>
struct Size<ListVarTypes, CurrentSizes, true> {

    typedef CurrentSizes type;

};

//------------------------------------------------------------------------------
// TimeSeriesEntryType
//------------------------------------------------------------------------------

#define xLOG_TIME_SERIES_ENTRY_TYPE

template <typename var_types>
struct TimeSeriesEntryType {

public: // subtypes

    using variable_type = var_types;

    using offsets = typename Offset<var_types, mpl::vector<>, 0, false>::type;

    using sizes   = typename Size<var_types, mpl::vector<>, false>::type;

    static const int front_size = mpl::front<variable_type>::type::size;

public: // constants

    // a vector of dimension = "dimension"
    // first dimension is assumed to be time.
    static const int dimension = mpl::size<var_types>::type::value;

    static const int total_size = Sum<var_types, 0, false>::value;

public: // data members

    char data[total_size];

public: // methods

    TimeSeriesEntryType operator+(const TimeSeriesEntryType &b) const;

    void accum(const TimeSeriesEntryType &a);

    template <int index>
    uint64_t get() const;

    template <int index>
    void set(uint64_t value);

};



//----------------------------------------------------------------
// TimeSeriesEntryType Impl.
//----------------------------------------------------------------

template <typename tsentry, int n>
struct Add {
    static void add_tsentry(const tsentry &a, const tsentry&b, tsentry &result) {
        static const int index        = tsentry::dimension - n;

        uint64_t va = a.template get<index>();
        if (index == 0) {
            result.template set<index>(va);
        }
        else {
            uint64_t va = a.template get<index>();
            uint64_t vb = b.template get<index>();
            result.template set<index>(va + vb);
        }

        Add<tsentry,n-1>::add_tsentry(a, b , result);
    }
};

template <typename tsentry>
struct Add<tsentry,0> {
    static void add_tsentry(const tsentry &a, const tsentry&b, tsentry &result) {
    }
};

//template<typename var_types>
//TimeSeriesEntryType<var_types> TimeSeriesEntryType<var_types>::operator+(const TimeSeriesEntryType &b) const {
//    typedef TimeSeriesEntryType<var_types> tsentry;
//    const tsentry &a = *this;
//    // keep first coord
//    tsentry result;
//    // runtime check
//    assert(a.template get<0>() ==  b.template get<0>());
//    //
//    add_tsentry<tsentry, tsentry::dimension>(a, b, result);
//    return result;
//}

template<typename var_types>
void TimeSeriesEntryType<var_types>::accum(const TimeSeriesEntryType &b)
{
    typedef TimeSeriesEntryType<var_types> tsentry;
    Add<tsentry, tsentry::dimension>::add_tsentry(*this, b, *this);
}

template<typename var_types>
template <int index>
uint64_t TimeSeriesEntryType<var_types>::get() const {

    static const int num_bytes    = mpl::at_c<sizes,   index>::type::value;
    static const int offset       = mpl::at_c<offsets, index>::type::value;

    uint64_t result = 0;

    std::copy(&data[offset], &data[offset + num_bytes], (char*) &result);

    return result;
}

template<typename var_types>
template <int index>
void TimeSeriesEntryType<var_types>::set(uint64_t value) {

    static const int num_bytes = mpl::at_c<sizes,   index>::type::value;
    static const int offset    = mpl::at_c<offsets, index>::type::value;

    char *ptr = (char*) &value;

    std::copy(ptr, ptr + num_bytes, (char*) &data[offset]);

}

//-----------------------------------------------------------------------------
// ostream operators
//-----------------------------------------------------------------------------

template <typename Vars>
std::ostream& operator<<(std::ostream& os, const TimeSeriesEntryType<Vars> &e)
{
    os << "entry" << std::endl;
    return os;
}

} // end namespace nanocube
