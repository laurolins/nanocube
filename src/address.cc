#include "address.hh"

#include <iterator>

namespace nanocube {

//-----------------------------------------------------------------------------
// Address
//-----------------------------------------------------------------------------

Address::Address(RawAddress data):
    data(data)
{}

Address& Address::appendDimension()
{
    data.push_back(DimAddress());
    return *this;
}

Address& Address::appendLabel(Label label)
{
    data.back().push_back(label);
    return *this;
}

size_t Address::dimensions() const
{
    return data.size();
}

size_t Address::levels(int index) const
{
    return data[index].size();
}

DimAddress &Address::operator[](size_t index)
{
    return data[index];
}

const DimAddress &Address::operator[](size_t index) const
{
    return data[index];
}

//-----------------------------------------------------------------------------
// Address IO
//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    os << "Addr[";
    for (size_t i=0;i<addr.dimensions();++i) {
        if (i > 0) {
            os << ",";
        }
        os << "{";
        if (addr.levels((int) i) > 0) {
            std::ostream_iterator<int> out_it (os,",");
            std::copy ( addr[i].begin(), addr[i].end()-1, out_it );
            if (addr[i].begin() != addr[i].end()) {
                os << addr[i].back();
            }
        }
        os << "}";
    }
    os << "]";
    return os;
}

}
