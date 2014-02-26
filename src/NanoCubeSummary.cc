#include "NanoCubeSummary.hh"

#include <algorithm>
#include <iostream>

#include <iomanip>

namespace nanocube {

//-----------------------------------------------------------------------------
// LayerKey
//-----------------------------------------------------------------------------

LayerKey::LayerKey(int dim, int level):
    dimension{dim}, level{level}
{}

bool LayerKey::operator==(const LayerKey& other) const {
    return (dimension == other.dimension && level == other.level );
}

bool LayerKey::operator<(const LayerKey& other) const {
    return ( dimension <  other.dimension ||
            (dimension == other.dimension && level < other.level) );
}

//-----------------------------------------------------------------------------
// LayerSummary
//-----------------------------------------------------------------------------

LayerSummary::LayerSummary(LayerKey layer_key):
    layer_key { layer_key }
{}

//-----------------------------------------------------------------------------
// Summary
//-----------------------------------------------------------------------------

void Summary::incrementNodeCount(int dim, int level)
{
    LayerSummary &layer = this->getOrCreate({dim, level});
    ++layer.num_nodes;
}

void Summary::incrementContent(int dim, int level, bool shared)
{
    LayerSummary &layer = this->getOrCreate({dim, level});
    if (shared)
        ++layer.num_shared_content;
    else
        ++layer.num_proper_content;
}

void Summary::incrementChildCount(int dim, int level, bool shared)
{
    LayerSummary &layer = this->getOrCreate({dim, level});
    if (shared)
        ++layer.num_shared_children;
    else
        ++layer.num_proper_children;
}

LayerSummary& Summary::getOrCreate(const LayerKey& layer_key) {

    auto comp = [](const std::unique_ptr<LayerSummary> &a, const LayerKey& b) {
        return a.get()->layer_key < b;
    };

    auto it = std::lower_bound(layers.begin(), layers.end(), layer_key, comp);

    if (it == layers.end() || !(it->get()->layer_key == layer_key)) {
        auto it2 = layers.insert(it, std::unique_ptr<LayerSummary>(new LayerSummary(layer_key)));
        return *((*it2).get());
    }
    else {
        return *(*it).get();
    }
}

std::ostream& operator<<(std::ostream &os, const LayerSummary& layer) {
    char sep = ' ';
    os << std::setw(12) << layer.layer_key.dimension << sep
       << std::setw(12) << layer.layer_key.level << sep
       << std::setw(12) << layer.num_nodes << sep
       << std::setw(12) << layer.num_proper_content << sep
       << std::setw(12) << layer.num_shared_content << sep
       << std::setw(12) << layer.num_proper_children << sep
       << std::setw(12) << layer.num_shared_children;
    return os;
}

std::ostream& operator<<(std::ostream &os, const Summary& summary) {

    char sep = ' ';
    os << std::setw(12) << "dim"    << sep
       << std::setw(12) << "level"  << sep
       << std::setw(12) << "nodes"  << sep
       << std::setw(12) << "pcont"  << sep
       << std::setw(12) << "scont"  << sep
       << std::setw(12) << "pchild" << sep
       << std::setw(12) << "schild" << std::endl;
    for (auto &layer_ptr: summary.layers) {
        const LayerSummary &layer = *layer_ptr.get();
        os << layer << std::endl;
    }
    return os;
}

} // nanocube namespace
