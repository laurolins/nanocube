#include "STree.hh"

namespace stree
{

//-----------------------------------------------------------------------------
// QueryEntry
//-----------------------------------------------------------------------------

Entry::Entry(void *address, LevelOffset target_level_offset):
    type(POINT_AND_OFFSET),
    address(address),
    target_level_offset(target_level_offset),
    min_address(nullptr),
    max_address(nullptr)
{}

Entry::Entry(void *min_address, void *max_address):
    type(RANGE),
    address(nullptr),
    target_level_offset(0),
    min_address(min_address),
    max_address(max_address)
{}

#if 0

Entry::~Entry()
{
    deleteAddresses();
}

void Entry::deleteAddresses() {
    if (type == RANGE) {
        delete min_address;
        delete max_address;
        min_address = max_address = nullptr;
    }
    else if (type == POINT_AND_OFFSET)
    {
        delete address;
        address = nullptr;
    }
}
#endif

}
