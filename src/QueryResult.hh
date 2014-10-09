#pragma once

#include <iostream>
#include <vector>
#include <stack>
#include <stdint.h>

#include "Query.hh"

#include "tree_store_nanocube.hh"

namespace query {

namespace result {

//TODO Cleanup these names

    using Result        = ::nanocube::TreeValueBuilder;
    using ResultNode    = ::nanocube::TreeValue::node_type;

// using namespace vector;

} // namespace result

} // namespace query
