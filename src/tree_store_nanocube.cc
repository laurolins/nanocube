#include "tree_store_nanocube.hh"

#include <algorithm>

namespace nanocube {

//-----------------------------------------------------------------
// SimpleConfig Impl.
//-----------------------------------------------------------------

const double SimpleConfig::default_value = 0.0f;

std::size_t SimpleConfig::operator()(const label_type &label) const {
    std::size_t hash_value = 0;
    std::for_each(label.begin(), label.end(), [&hash_value](label_item_type v) {
        std::size_t vv = (std::size_t) v;
        hash_value ^= vv + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
    });
    return hash_value;
}

std::ostream& SimpleConfig::print_label(std::ostream& os, const label_type &label) const {
    os << "[";
    bool first = true;
    for (auto l: label) {
        if (!first)
            os << ",";
        os << l;
        first = false;
    }
    os << "]";
    return os;
}

std::ostream& SimpleConfig::print_value(std::ostream& os, const value_type &value, const parameter_type& parameter) const {
    os << value;
    return os;
}

std::ostream& SimpleConfig::serialize_label(std::ostream& os, const label_type &label) {
    uint16_t n = static_cast<uint16_t>(label.size());
    os.write(reinterpret_cast<char*>(&n), sizeof(uint16_t));
    for (auto l: label) {
        uint16_t li = static_cast<uint16_t>(l);
        os.write(reinterpret_cast<char*>(&li), sizeof(uint16_t));
    }
    return os;
}

std::istream& SimpleConfig::deserialize_label(std::istream& is, label_type &label) {
    uint16_t n;
    is.read(reinterpret_cast<char*>(&n), sizeof(uint16_t));
    label.reserve(n);
    for (auto i=0;i<n;++i) {
        uint16_t li;
        is.read(reinterpret_cast<char*>(&li), sizeof(uint16_t));
        label.push_back(li);
    }
    return is;
}

std::ostream& SimpleConfig::serialize_value(std::ostream& os, const value_type &value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(double));
    return os;
}

std::istream& SimpleConfig::deserialize_value(std::istream& is, value_type &value) {
    is.read(reinterpret_cast<char*>(&value), sizeof(double));
    return is;
}

} 
