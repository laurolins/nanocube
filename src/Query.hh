#pragma once

#include <vector>
#include <stack>
#include <stdint.h>

#include "polycover/labeled_tree.hh"

namespace query {

using RawAddress = uint64_t;
    
using Mask = polycover::labeled_tree::Node;

struct ListTarget;
struct RangeTarget;
struct MaskTarget;
struct SequenceTarget;
struct FindAndDiveTarget;
struct BaseWidthCountTarget;

//-----------------------------------------------------------------------------
// Target: addresses that we should try to find when traversing a dimension
//-----------------------------------------------------------------------------

struct Target {
public: // subtypes

    enum Type { ROOT, LIST, FIND_AND_DIVE, RANGE, BASE_WIDTH_COUNT, SEQUENCE, MASK };

public:

    static Target * const root;

public:

    Target();
    Target(Type type);
    virtual ~Target();

public: // data members

    virtual const ListTarget*     asListTarget();

    virtual RangeTarget*          asRangeTarget();
    virtual SequenceTarget*       asSequenceTarget();
    virtual FindAndDiveTarget*    asFindAndDiveTarget();
    virtual BaseWidthCountTarget* asBaseWidthCountTarget();
    virtual MaskTarget*           asMaskTarget();

    virtual void replace(RawAddress addr_old, RawAddress add_new);
    
public: // data memebrs

    Type type;

};

//-----------------------------------------------------------------------------
// ListTarget
//-----------------------------------------------------------------------------

struct ListTarget: public Target {

public: // constructors

    ListTarget();

    ListTarget(std::vector<RawAddress> list);

public: // methods

    void add(RawAddress addr);

    ListTarget* asListTarget();

    void replace(RawAddress addr_old, RawAddress add_new);
    
public: // data memebers

    std::vector<RawAddress> list;

};

//-----------------------------------------------------------------------------
// FindAndDiveTarget
//-----------------------------------------------------------------------------

struct FindAndDiveTarget: public Target {

public: // constructors

    FindAndDiveTarget(RawAddress base, int offset);

public: // methods

    FindAndDiveTarget* asFindAndDiveTarget();

    void replace(RawAddress addr_old, RawAddress addr_new);

public: // data memebers

    RawAddress base;

    int     offset;

};

//-----------------------------------------------------------------------------
// BaseWidthCountTarget
//-----------------------------------------------------------------------------

struct BaseWidthCountTarget: public Target {

public: // constructors
    BaseWidthCountTarget(RawAddress base, int width, int count);

public: // methods
    BaseWidthCountTarget* asBaseWidthCountTarget();


    void replace(RawAddress addr_old, RawAddress addr_new);


public: // data memebers
    RawAddress base;
    int        width;
    int        count;
};

//-----------------------------------------------------------------------------
// RangeTarget
//-----------------------------------------------------------------------------

struct RangeTarget: public Target {

public: // constructors

    RangeTarget(RawAddress min_address, RawAddress max_address);

public: // methods

    RangeTarget* asRangeTarget();

    void replace(RawAddress addr_old, RawAddress addr_new);

public: // data memebers

    RawAddress min_address;

    RawAddress max_address;

};

//-----------------------------------------------------------------------------
// SequenceTarget
//-----------------------------------------------------------------------------

struct SequenceTarget: public Target {

public: // constructors

    SequenceTarget(const std::vector<RawAddress>& addresses);

public: // methods

    SequenceTarget* asSequenceTarget();

    void replace(RawAddress addr_old, RawAddress addr_new);

public: // data memebers

    std::vector<RawAddress>addresses;

};

    //-----------------------------------------------------------------------------
    // MaskTarget
    //-----------------------------------------------------------------------------
    
    struct MaskTarget: public Target {
        
    public: // constructors
        
        MaskTarget(const Mask* root);
        
    public: // methods
        
        MaskTarget* asMaskTarget();
        
        void replace(RawAddress addr_old, RawAddress addr_new);
        
    public: // data memebers
        
        const Mask* root;
        
    };
    
//-----------------------------------------------------------------------------
// Query Description
//-----------------------------------------------------------------------------

struct QueryDescription {
public: // subtypes
    
    static const int MAX_DIMENSIONS = 30;

    //    typedef NanoCube nanocube_type;
//    typedef typename nanocube_type::entry_type entry_type;
//    static const int VARIABLE_INDEX = VarIndex;
//    static const int DIMENSION      = nanocube_type::dimension;
//    //
//    static_assert((0 < VARIABLE_INDEX) && (VARIABLE_INDEX < entry_type::dimension),
//                  "Invalid variable index on QueryDescription");

    //
    // Assume that each address in a dimension has a common
    // interpretation across dimensions. This interpretation
    // could be used here as a way to describe the target
    // set of each dimension.
    //
    // Essentially, a query description is simply a description
    // of which dimensions are anchored: Anchor Set, and a description
    // of a target set for each dimension.
    //
    // There is more than one way to describe the target set of
    // a dimension:
    //
    //     1 - Single address
    //     2 - Multiple addresses (disjoint)
    //     3 - Children that are k-levels deeper than a base address
    //     4 - Range (min address, max address)
    //     5 - Sequence (list of addresses)
    //     6 - Base/Width/Count for time
    //

public: // Constructor

    QueryDescription();

public: // Methods

    void setImgHint(int dimension, bool flag);
    void setAnchor(int dimension, bool flag);
    void setFindAndDiveTarget(int dimension, RawAddress base_address, int dive_depth);
    void setRangeTarget(int dimension, RawAddress min_address, RawAddress max_address);
    void setSequenceTarget(int dimension, const std::vector<RawAddress> addresses);
    void setMaskTarget(int dimension, const Mask *mask);

    // this is used for the time dimension which is special
    void setBaseWidthCountTarget(int dimension, RawAddress base_address, int width, int count);

    Target* getFirstAnchoredTarget();

public: // Data Members
    // time range description first/size/count
    std::vector<bool>    anchors;
    std::vector<Target*> targets;
    std::vector<bool>    img_hint;
};

} // query namespace
