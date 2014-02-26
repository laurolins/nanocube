#include "QueryResult.hh"
#include "NanoCubeSchema.hh"

#include <iostream>

namespace nanocube {

struct QueryResult {

public: // constructor

    QueryResult(::query::result::Vector &result, Schema &schema);

public: // Methods

    void json(std::ostream& os);

public: // Data Members

    ::query::result::Vector &result;
    ::nanocube::Schema      &schema;

};

} // nanocube namespace
