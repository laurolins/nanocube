#pragma once
#include <stdint.h>

//-----------------------------------------------------------------------------
// CountRecord
//-----------------------------------------------------------------------------

struct CountRecord {

    CountRecord();

    uint64_t actual_num_nodes;
    uint64_t expanded_num_nodes;

    uint64_t num_proper_parent_child_arcs;
    uint64_t num_shared_parent_child_arcs;

    uint64_t num_proper_content_arcs;
    uint64_t num_shared_content_arcs;

    uint64_t actual_used_memory;
    uint64_t expanded_used_memory;
};

