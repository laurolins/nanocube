#include "Common.hh"

//-----------------------------------------------------------------------------
// CountRecord
//-----------------------------------------------------------------------------

CountRecord::CountRecord():
    actual_num_nodes(0),
    expanded_num_nodes(0),
    num_proper_parent_child_arcs(0),
    num_shared_parent_child_arcs(0),
    num_proper_content_arcs(0),
    num_shared_content_arcs(0),
    actual_used_memory(0),
    expanded_used_memory(0)
{}
