#pragma once

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

#include "Report.hh"

#define xNANOCUBE_REPORT_BUILDER

namespace mpl = boost::mpl;

namespace nanocube {

template <typename NanoCube, int DimensionIndex>
struct ReportBuilder;


//-----------------------------------------------------------------------------
// AuxReportBuilder<R,L>
//-----------------------------------------------------------------------------

template <typename RepBuilder, bool LastDimension = false>
struct AuxReportBuilder {

public: // Subtypes & Constants

    typedef RepBuilder                                     report_builder_type;
    typedef typename report_builder_type::nanocube_type    nanocube_type;
    typedef typename report_builder_type::dimension_type   dimension_type;

    typedef typename report_builder_type::node_type        node_type;
    typedef typename report_builder_type::content_type     content_type;

    typedef typename content_type::NodeType                content_node_type;

    static const int DIMENSION_INDEX = report_builder_type::DIMENSION_INDEX;

public: // Static Methods

    static void process_content(const node_type *node, report::Report &report);

};

//-----------------------------------------------------------------------------
// AuxReportBuilder<R,true>: special treatment for last dimension
//-----------------------------------------------------------------------------

template <typename ReportBuilder>
struct AuxReportBuilder<ReportBuilder, true> {

public: // Subtypes & Constants

    typedef ReportBuilder                                  report_builder_type;
    typedef typename report_builder_type::nanocube_type    nanocube_type;
    typedef typename report_builder_type::dimension_type   dimension_type;

    typedef typename report_builder_type::content_type     content_type;
    typedef typename report_builder_type::node_type        node_type;

    typedef typename nanocube_type::entry_type             entry_type;

    static const int DIMENSION_INDEX = report_builder_type::DIMENSION_INDEX;

public: // Static Methods

    static void process_content(const node_type *node, report::Report &report);

};

//-----------------------------------------------------------------------------
// ReportBuilder
//-----------------------------------------------------------------------------

template <typename NanoCube, int DimensionIndex>
struct ReportBuilder {

public: // subtypes

    typedef ReportBuilder                                              report_builder_type;

    typedef NanoCube                                                   nanocube_type;
    typedef typename nanocube_type::dimension_types                    dimension_types;
    typedef typename mpl::at_c<dimension_types, DimensionIndex>::type  dimension_type;
    typedef typename dimension_type::IteratorType                      iterator_type;
    typedef typename dimension_type::NodeType                          node_type;
    typedef typename dimension_type::ContentType                       content_type;

    static const int DIMENSION       = nanocube_type::DIMENSION;
    static const int DIMENSION_INDEX = DimensionIndex;

public: // memebrs

    ReportBuilder(const dimension_type &tree, report::Report &report) ;

};



//-----------------------------------------------------------------------------
// AuxReportBuilder<R,L> Impl.
//-----------------------------------------------------------------------------

template <typename RepBuilder, bool LastDimension>
void AuxReportBuilder<RepBuilder, LastDimension>::process_content(const node_type *node, report::Report &report) {

    content_type       *content = node->getContent();
    content_node_type  *content_root = content->getRoot();

    // we associate a report to the root of the content tree
    // (there is a content tree because this is not the last dimension)
    report::Node *content_report_node = report.getNode((uint64_t) content_root);
    if (!content_report_node) {
        content_report_node =
                report.insertNode((uint64_t) content_root, DIMENSION_INDEX+1, 0);
    }

    // report node
    report::Node *report_node = report.getNode((uint64_t) node);

    // update content link on report_node
    report_node->setContent(content_report_node, node->contentIsShared());

    // recurse if the content is proper
    if (node->contentIsProper()) {
        content_type &content_tree = *content;

#ifdef NANOCUBE_REPORT_BUILDER
        std::cout << "Recursing Dimension: " << report_builder_type::DIMENSION_INDEX << std::endl;
#endif

        ReportBuilder<nanocube_type, DIMENSION_INDEX+1> x(content_tree, report); // don't need an object
    }
}

//-----------------------------------------------------------------------------
// AuxReportBuilder<R,true> Impl.
//-----------------------------------------------------------------------------

template <typename ReportBuilder>
void AuxReportBuilder<ReportBuilder, true>::process_content(const node_type *node, report::Report &report) {

    // Note that here we don't use the content->getRoot():
    //    the content of the the last dimension is not a tree
    //    it is a summary (e.g. time series)
    content_type       *content = node->getContent();

    // we associate a report to the root of the content tree
    // (there is a content tree because this is not the last dimension)
    report::Node *content_report_node = report.getNode((uint64_t) content);
    if (!content_report_node) {
        content_report_node =
                report.insertNode((uint64_t) content, DIMENSION_INDEX+1, 0);
    }

    std::stringstream ss;
    bool first = true;
    for (entry_type &e: content->entries) {
        if (!first) {
            ss << " ";
        }
        uint64_t time = e.template get<0>();
        ss << "t" << time;
        first = false;
    }
    content_report_node->setInfo(ss.str());

    // report node
    report::Node *report_node = report.getNode((uint64_t) node);

    // update content link on report_node
    report_node->setContent(content_report_node, node->contentIsShared());

}


//-----------------------------------------------------------------------------
// ReportBuilder Impl.
//-----------------------------------------------------------------------------

template <typename NanoCube, int DimensionIndex>
ReportBuilder<NanoCube, DimensionIndex>::ReportBuilder(const dimension_type &tree, report::Report &report) {

#ifdef NANOCUBE_REPORT_BUILDER
    std::cout << "Processing tree of dimension: " << DIMENSION_INDEX << std::endl;
#endif
    iterator_type it(tree);
    while (it.next()) {

        //
        // iterate over all parent-child links of the current dimension
        // (including a fake one where the parent is null and the child
        // is the root of "tree").
        //

        const node_type *node        = it.getCurrentNode();
        const node_type *parent_node = it.getCurrentParentNode();

        // std::cout << "--> node" << node << "  (parent: " << parent_node << ")" << std::endl;

        report::Node *report_node = nullptr;
        if (parent_node != 0) {

            report::Node *parent_report_node = report.getNode((uint64_t) parent_node);

            assert(parent_report_node != nullptr);

            // insert node
            report_node = report.getNode((uint64_t) node);

#ifdef NANOCUBE_REPORT_BUILDER
            std::cout << "case (A) there exists a parent (non-root)" << std::endl;
            std::cout << "    parent: " << *parent_report_node << std::endl;
#endif

            // first time child node was found?
            if (!report_node) {

#ifdef NANOCUBE_REPORT_BUILDER
                std::cout << "    creating child node..." << std::endl;
#endif

                report_node = report.insertNode((uint64_t) node, DIMENSION_INDEX, parent_report_node->layer->layer_id.level+1);

            }

#ifdef NANOCUBE_REPORT_BUILDER
            std::cout << "    child:  " << *report_node << std::endl;
#endif

            // insert child link
            parent_report_node->insertChildLink(report_node, it.isShared(), it.getLabel());

        }
        else {

#ifdef NANOCUBE_REPORT_BUILDER
            std::cout << "case (B) there is no parent (it is a root)" << std::endl;
#endif

            report_node = report.getNode((uint64_t) node);

            if (!report_node) {
                report_node = report.insertNode((uint64_t) node, DIMENSION_INDEX, 0);

#ifdef NANOCUBE_REPORT_BUILDER
                std::cout << "    creating new child (root in this case, parent==0)" << std::endl;
#endif

            }

#ifdef NANOCUBE_REPORT_BUILDER
            std::cout << "    child: " << *report_node << std::endl;
#endif

        }

        //
        // if parent-child is proper (when parent==0 it is considered proper)
        // process content of child node.
        //
        if (it.isProper()) {

#ifdef NANOCUBE_REPORT_BUILDER
            std::cout << "(Content Processing)" << std::endl;
#endif

            // recursively solve the next dimension
            AuxReportBuilder<report_builder_type, DIMENSION_INDEX == DIMENSION-1>::process_content(node, report);

            // const content_type *content = node->getContent();

//                report::Node *content_report_node = report.getNode((uint64_t) content);
//                if (!content_report_node) {
//                    content_report_node = report.insertNode((uint64_t) content, dim+1, 0);
//                    std::cout << "    creating content..." << std::endl;
//                }
//                std::cout << "    content: " << *content_report_node << std::endl;

//                report_node->setContent(content_report_node, node->contentIsShared());

//                // set content link
//                // report.setContentLink(report_node, (uint64_t) content, node->contentIsShared());

//                // if content is proper recurse
//                if (node->contentIsProper()) {

//                    std::cout << "-------------{ BEGIN RECURSION " << (dim+1) << std::endl;

//                    AuxReportBuilder<!last_dimension>::template recurse<nanocube_type, content_type, dim+1>(*content, report);

//                    std::cout << "-------------} END RECURSION " << (dim+1) << std::endl;
//                }
        }


        // get content and recurse if it is proper

        // report.insert
    }
}

} // nanocube namespace
