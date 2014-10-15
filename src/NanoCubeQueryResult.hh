#include "QueryResult.hh"
#include "NanoCubeSchema.hh"

#include "tree_store_nanocube.hh"


#include <iostream>

namespace nanocube {

struct QueryResult {

public: // constructor

    QueryResult(::nanocube::TreeValue &result, Schema &schema);

public: // Methods

    void json(std::ostream& os);

public: // Data Members

    ::nanocube::TreeValue  &result;
    ::nanocube::Schema      &schema;

};

} // nanocube namespace
