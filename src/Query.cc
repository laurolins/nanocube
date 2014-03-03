#include<cstddef>
#include <stdexcept>

#include"Query.hh"

namespace query {

//-----------------------------------------------------------------------------
// Target: addresses that we should try to find when traversing a dimension
//-----------------------------------------------------------------------------

Target * const Target::root = new Target();

Target::Target():
    type(ROOT)
{}

Target::Target(Type type):
    type(type)
{}

Target::~Target()
{}

const ListTarget* Target::asListTarget()
{
    throw std::exception();
}

RangeTarget* Target::asRangeTarget()
{
    throw std::exception();
}

SequenceTarget* Target::asSequenceTarget()
{
    throw std::exception();
}

FindAndDiveTarget* Target::asFindAndDiveTarget()
{
    throw std::exception();
}

BaseWidthCountTarget* Target::asBaseWidthCountTarget()
{
    throw std::exception();
}

void Target::replace(RawAddress , RawAddress )
{
    // throw std::exception();
}

//-----------------------------------------------------------------------------
// ListTarget
//-----------------------------------------------------------------------------

ListTarget::ListTarget():
    Target(LIST)
{}

ListTarget::ListTarget(std::vector<RawAddress> list):
    Target(LIST),
    list(list)
{}

void ListTarget::add(RawAddress addr) {
    list.push_back(addr);
}

ListTarget* ListTarget::asListTarget() {
    return this;
}

void ListTarget::replace(RawAddress addr_old, RawAddress addr_new)
{
    for (auto &v: list) {
        if (v == addr_old) {
            v = addr_new;
        }
    }
}

//-----------------------------------------------------------------------------
// FindAndDiveTarget
//-----------------------------------------------------------------------------

FindAndDiveTarget::FindAndDiveTarget(RawAddress base, int offset):
    Target(FIND_AND_DIVE),
    base(base),
    offset(offset)
{}

FindAndDiveTarget* FindAndDiveTarget::asFindAndDiveTarget() {
    return this;
}

void FindAndDiveTarget::replace(RawAddress addr_old, RawAddress addr_new) {
    if (base == addr_old) {
        base = addr_new;
    }
}


//-----------------------------------------------------------------------------
// RangeTarget
//-----------------------------------------------------------------------------

RangeTarget::RangeTarget(RawAddress min_address, RawAddress max_address):
    Target(RANGE),
    min_address(min_address),
    max_address(max_address)
{}

RangeTarget* RangeTarget::asRangeTarget() {
    return this;
}

void RangeTarget::replace(RawAddress addr_old, RawAddress addr_new)
{
    if (min_address == addr_old)
        min_address = addr_new;
    if (max_address == addr_old)
        max_address = addr_new;
}

//-----------------------------------------------------------------------------
// SequenceTarget
//-----------------------------------------------------------------------------

SequenceTarget::SequenceTarget(const std::vector<RawAddress>& addresses):
    Target(SEQUENCE),
    addresses{addresses}
{}

SequenceTarget* SequenceTarget::asSequenceTarget() {
    return this;
}

void SequenceTarget::replace(RawAddress addr_old, RawAddress addr_new)
{
    for (auto &v: addresses) {
        if (v == addr_old) {
            v = addr_new;
        }
    }
}


//-----------------------------------------------------------------------------
// BaseWidthCountTarget
//-----------------------------------------------------------------------------

BaseWidthCountTarget::BaseWidthCountTarget(RawAddress base, int width, int count):
    Target(BASE_WIDTH_COUNT),
    base(base),
    width(width),
    count(count)
{}

BaseWidthCountTarget* BaseWidthCountTarget::asBaseWidthCountTarget() {
    return this;
}

void BaseWidthCountTarget::replace(RawAddress addr_old, RawAddress addr_new)
{
    if (base == addr_old) {
        base = addr_new;
    }
}

//-----------------------------------------------------------------------------
// Query Description
//-----------------------------------------------------------------------------

QueryDescription::QueryDescription():
    anchors(MAX_DIMENSIONS, false),
    targets(MAX_DIMENSIONS, Target::root)
{}

void QueryDescription::setAnchor(int dimension, bool flag) {
    anchors[dimension] = flag;
}

void QueryDescription::setFindAndDiveTarget(int dimension, RawAddress base_address, int dive_depth) {
    if (targets[dimension]->type != Target::ROOT) {
        delete targets[dimension];
    }
    targets[dimension] = new FindAndDiveTarget(base_address, dive_depth);
}

void QueryDescription::setRangeTarget(int dimension, RawAddress min_address, RawAddress max_address) {
    if (targets[dimension]->type != Target::ROOT) {
        delete targets[dimension];
    }
    targets[dimension] = new RangeTarget(min_address, max_address);
}

void QueryDescription::setSequenceTarget(int dimension, const std::vector<RawAddress> addresses)
{
    if (targets[dimension]->type != Target::ROOT) {
        delete targets[dimension];
    }
    targets[dimension] = new SequenceTarget(addresses);
}

void QueryDescription::setBaseWidthCountTarget(int dimension, RawAddress base_address, int width, int count)
{
    if (targets[dimension]->type != Target::ROOT) {
        delete targets[dimension];
    }
    targets[dimension] = new BaseWidthCountTarget(base_address,width,count);
}

Target *QueryDescription::getFirstAnchoredTarget()
{
    std::size_t i = 0;
    for (auto b: this->anchors) {
        if (b) {
            break;
        }
        else {
            ++i;
        }
    }
    if (i < this->targets.size()) {
        return this->targets[i];
    }
    else return nullptr;
}

} // query namespace
