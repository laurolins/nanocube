#pragma once

#include <vector>
#include <ostream>

namespace nanocube {

//-----------------------------------------------------------------------------
// Label
//-----------------------------------------------------------------------------

using Label = int; // Label (or SmallLabel will be 15-bits long)

const Label NO_LABEL = -1;

//-----------------------------------------------------------------------------
// Address
//-----------------------------------------------------------------------------

using  DimAddress = std::vector<Label>;
using  RawAddress = std::vector<DimAddress>;

struct Address {
    Address() = default;
    Address(RawAddress data);

    Address(const Address& ) = default;
    Address& operator=(const Address& ) = default;
    Address(Address&& ) = default;
    Address& operator=(Address&& ) = default;

    Address& appendDimension();
    Address& appendLabel(Label label);

    size_t dimensions() const;
    size_t levels(int index) const;

    DimAddress& operator[](size_t index);
    const DimAddress& operator[](size_t index) const;

public:
    RawAddress data;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

}
