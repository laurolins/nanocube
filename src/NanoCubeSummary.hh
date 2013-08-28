#pragma once

#include <cstdint>

#include <boost/mpl/if.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>

// #include <boost/mpl/reverse.hpp>
// #include <boost/mpl/begin_end.hpp>
// #include <boost/type_traits/is_same.hpp>

#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/list.hpp>

#include <boost/type_traits/is_same.hpp>

#include <vector>
#include <memory>

namespace mpl = boost::mpl;

namespace nanocube {

//-----------------------------------------------------------------------------
// LayerKey
//-----------------------------------------------------------------------------

struct LayerKey {
public:
    LayerKey() = default;
    LayerKey(int dim, int level);

    bool operator==(const LayerKey& other) const;
    bool operator<(const LayerKey& other) const;

public:
    int dimension { -1 };
    int level     { -1 };
};

//-----------------------------------------------------------------------------
// LayerSummary
//-----------------------------------------------------------------------------

struct LayerSummary {
public:
    LayerSummary() = default;
    LayerSummary(LayerKey layer_key);
public:
    void addNode();
    // void add
public:
    LayerKey layer_key;
    uint64_t num_nodes           { 0 };
    uint64_t num_proper_content  { 0 };
    uint64_t num_shared_content  { 0 };
    uint64_t num_proper_children { 0 };
    uint64_t num_shared_children { 0 };
};

//-----------------------------------------------------------------------------
// Summary
//-----------------------------------------------------------------------------

struct Summary {

    Summary() = default;

    Summary(const Summary &other) = delete;
    Summary& operator=(const Summary &other) = delete;

    Summary(Summary &&other) = default;
    Summary& operator=(Summary &&other) = default;

    void incrementNodeCount  ( int dim, int level );
    void incrementContent    ( int dim, int level, bool shared );
    void incrementChildCount ( int dim, int level, bool shared );

    LayerSummary& getOrCreate(const LayerKey& layer_key);

public:

    std::vector<std::unique_ptr<LayerSummary>> layers;

};


//-----------------------------------------------------------------------------
// summary io
//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const LayerSummary& layer);
std::ostream& operator<<(std::ostream &os, const Summary& summary);

//-----------------------------------------------------------------------------
// Function to mount a summary
//-----------------------------------------------------------------------------

template <typename NanoCubeType>
Summary mountSummary(NanoCubeType &nanocube);


//-----------------------------------------------------------------------------
// Forward Decl.
//-----------------------------------------------------------------------------

template <typename NanoCube, int DimensionIndex>
struct SummaryBuilder;

//-----------------------------------------------------------------------------
// AuxSummaryBuilder<R,L>
//-----------------------------------------------------------------------------

template <typename RepBuilder, bool LastDimension = false>
struct AuxSummaryBuilder {

public: // Subtypes & Constants

    typedef RepBuilder                                     summary_builder_type;
    typedef typename summary_builder_type::nanocube_type    nanocube_type;
    typedef typename summary_builder_type::dimension_type   dimension_type;

    typedef typename summary_builder_type::node_type        node_type;
    typedef typename summary_builder_type::content_type     content_type;

    typedef typename content_type::NodeType                content_node_type;

    static const int DIMENSION_INDEX = summary_builder_type::DIMENSION_INDEX;

public: // Static Methods

    static void process_content(const node_type *node, Summary &summary);

};

//-----------------------------------------------------------------------------
// AuxSummaryBuilder<R,true>: special treatment for last dimension
//-----------------------------------------------------------------------------

template <typename SummaryBuilder>
struct AuxSummaryBuilder<SummaryBuilder, true> {

public: // Subtypes & Constants

    typedef SummaryBuilder                                  summary_builder_type;
    typedef typename summary_builder_type::nanocube_type    nanocube_type;
    typedef typename summary_builder_type::dimension_type   dimension_type;

    typedef typename summary_builder_type::content_type     content_type;
    typedef typename summary_builder_type::node_type        node_type;

    typedef typename nanocube_type::entry_type             entry_type;

    static const int DIMENSION_INDEX = summary_builder_type::DIMENSION_INDEX;

public: // Static Methods

    static void process_content(const node_type *node, Summary &summary);

};

//-----------------------------------------------------------------------------
// SummaryBuilder
//-----------------------------------------------------------------------------

template <typename NanoCube, int DimensionIndex>
struct SummaryBuilder {

public: // subtypes

    typedef SummaryBuilder                                              summary_builder_type;

    typedef NanoCube                                                   nanocube_type;
    typedef typename nanocube_type::dimension_types                    dimension_types;
    typedef typename mpl::at_c<dimension_types, DimensionIndex>::type  dimension_type;
    typedef typename dimension_type::IteratorType                      iterator_type;
    typedef typename dimension_type::NodeType                          node_type;
    typedef typename dimension_type::ContentType                       content_type;

    static const int DIMENSION       = nanocube_type::DIMENSION;
    static const int DIMENSION_INDEX = DimensionIndex;

public: // memebrs

    SummaryBuilder(const dimension_type &tree, Summary &summary) ;

};



//-----------------------------------------------------------------------------
// AuxSummaryBuilder<R,L> Impl.
//-----------------------------------------------------------------------------

template <typename RepBuilder, bool LastDimension>
void AuxSummaryBuilder<RepBuilder, LastDimension>::process_content(const node_type *node, Summary &summary) {

    content_type       *content      = node->getContent();
    // content_node_type  *content_root = content->getRoot();

    content_type &content_tree = *content;

    SummaryBuilder<nanocube_type, DIMENSION_INDEX+1>(content_tree, summary); // don't need an object

}

//-----------------------------------------------------------------------------
// AuxSummaryBuilder<R,true> Impl.
//-----------------------------------------------------------------------------

template <typename SummaryBuilder>
void AuxSummaryBuilder<SummaryBuilder, true>::process_content(const node_type *node, Summary &summary)
{

    // Note that here we don't use the content->getRoot():
    //    the content of the the last dimension is not a tree
    //    it is a summary (e.g. time series)
    // content_type       *content = node->getContent();

    // add simple counts here please
    summary.incrementNodeCount(DIMENSION_INDEX+1, 0);

//    // we associate a summary to the root of the content tree
//    // (there is a content tree because this is not the last dimension)
//    summary::Node *content_summary_node = summary.getNode((uint64_t) content);
//    if (!content_summary_node) {
//        content_summary_node =
//                summary.insertNode((uint64_t) content, DIMENSION_INDEX+1, 0);
//    }

}


//-----------------------------------------------------------------------------
// SummaryBuilder Impl.
//-----------------------------------------------------------------------------

template <typename NanoCube, int DimensionIndex>
SummaryBuilder<NanoCube, DimensionIndex>::SummaryBuilder(const dimension_type &tree, Summary &summary) {

    iterator_type it(tree);
    while (it.next()) {

        //
        // iterate over all parent-child links of the current dimension
        // (including a fake one where the parent is null and the child
        // is the root of "tree").
        //

        const node_type *node        = it.getCurrentNode();
        const node_type *parent_node = it.getCurrentParentNode();

        int level = it.getCurrentLevel();

        if (parent_node) { // there is a parent node
            summary.incrementChildCount(DimensionIndex, level-1, it.isShared());
        }

        if (it.isProper()) {
            summary.incrementNodeCount(DimensionIndex, level);
            summary.incrementContent(DimensionIndex, level, node->contentIsShared());
            if (node->contentIsProper()) {
                AuxSummaryBuilder<summary_builder_type, DIMENSION_INDEX == DIMENSION-1>::process_content(node, summary);
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Function to mount a summary. Implementation.
//-----------------------------------------------------------------------------

template <typename NanoCubeType>
Summary mountSummary(NanoCubeType &nanocube)
{
    Summary summary;
    SummaryBuilder<NanoCubeType, 0> summary_builder(nanocube.root, summary);
    return summary;
}

} // nanocube namespace
