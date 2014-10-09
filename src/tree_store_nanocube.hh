#pragma once

#include "address.hh"
#include "tree_store.hh"

#include <iostream>

namespace nanocube {

//-----------------------------------------------------------------
// SimpleConfig
//-----------------------------------------------------------------

struct SimpleConfig {
    
    using label_type       = ::nanocube::DimAddress;
    using label_item_type  = typename label_type::value_type;
    using value_type       = double;
    using parameter_type   = int; // dummy parameter
    
    static const double default_value;
    
    std::size_t operator()(const label_type &label) const;
    
    std::ostream& print_label(std::ostream& os, const label_type &label) const;
    
    std::ostream& print_value(std::ostream& os, const value_type &value, const parameter_type& parameter) const;
    
    std::ostream& serialize_label(std::ostream& os, const label_type &label);
    
    std::istream& deserialize_label(std::istream& is, label_type &label);
    
    std::ostream& serialize_value(std::ostream& os, const value_type &value);
    
    std::istream& deserialize_value(std::istream& is, value_type &value);
    
};

using TreeValue = tree_store::TreeStore<SimpleConfig>;
using TreeValueBuilder = tree_store::TreeStoreBuilder<TreeValue>;

using TreeValueIterator = tree_store::TreeStoreIterator<TreeValue>;

    
}
