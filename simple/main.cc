#include <simple_nanocube.hh>
#include <simple_nanocube_iterator.hh>
#include <report.hh>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>

//-----------------------------------------------------------------------------
// getLabel<T>(T *obj)
//-----------------------------------------------------------------------------

template <typename T>
std::string getLabel(T* obj, std::string prefix) {

    if (obj == nullptr) {
        return "null";
    }

    static int next_label = 0;
    static std::unordered_map<T*, int> map;
    auto it = map.find(obj);
    if (it == map.end()) {
        map[(obj)] = next_label;
        return prefix+std::to_string(next_label++);
    }
    else {
        return prefix+std::to_string(it->second);
    }
};

//-----------------------------------------------------------------------------
// Report Graphviz
//-----------------------------------------------------------------------------

report::Report buildReport(const Nanocube &nc) {

    report::Report report(nc.dimension + 1);

    Iterator it(nc);
    while (it.next()) {

        Iterator::Item &item = it.current_item;

        uint64_t parent_key = (uint64_t) (item.parent_node ? item.parent_node : nullptr);
        uint64_t child_key  = (uint64_t) item.node;

        auto report_parent = parent_key ? report.getNode(parent_key) : nullptr;
        auto report_child  = child_key  ? report.getNode(child_key) : nullptr;

        bool shared = item.link_type == SHARED;
        bool proper = item.link_type == PROPER;

        if (report_child == nullptr) {
            report_child = report.insertNode(child_key, item.dimension, item.level);
        }

        if (report_parent != nullptr) {
            if (item.level == 0) {
                report_parent->setContent(report_child, shared);
            }
            else {
                report_parent->insertChildLink(report_child, shared, std::to_string(item.label));
            }
        }

        if (it.lastDimension() && proper) {

            // use child as a parent of the summary node
            uint64_t summary_key = (uint64_t) item.node->getContentAsSummary();
            auto report_summary = report.getNode(summary_key);
            if (report_summary == nullptr) {
                report_summary = report.insertNode(summary_key, item.dimension+1, 0);
                report_summary->setInfo(item.node->getContentAsSummary()->info());

                std::cout << item.node->getContentAsSummary()->info() << std::endl;
            }


            bool summary_shared = item.node->getContentType() == SHARED;
            report_child->setContent(report_summary, summary_shared);

        }

    }

    return report;

}





void auxreport() {

#if 0
    Iterator it(nc);
    while (it.next()) {
        Iterator::Item &item = it.current_item;
        auto node_label = getLabel(item.node,"N");

        // report edge information
        if (item.level == 0) {
            std::cout << "Content Link: "
                      << getLabel(item.parent_node,"N")
                      << " -> "
                      << getLabel(item.node,"N")
                      << std::endl;
        }
        else {
            std::cout << "Parent-Child Link: "
                      << getLabel(item.parent_node,"N")
                      << " --(" << item.label << ")-- "
                      << getLabel(item.node,"N")
                      << " " << ((item.link_type == SHARED) ? "(SHARED)" : "(PROPER)")
                      << std::endl;
        }

        if (item.link_type == PROPER) {
            std::cout << "Node: " << node_label
                      << " dim: " << item.dimension
                      << " level: " << item.level << std::endl;
            if (it.lastDimension()) {
                auto summary_label = getLabel(item.node->getContentAsSummary(),"S");
                LinkType link_type = item.node->getContentType();
                std::cout << "Summary Content Link: " << node_label << " -> " << summary_label << " "
                          << ((link_type == SHARED) ? "(SHARED)" : "(PROPER)")
                                                     << std::endl;
            }
        }
//        if (item.level == 0) {
//            std::cout << "Content Link: " << node_label
//                      << " dim: " << item.dimension
//                      << " level: " << item.level << std::endl;
//        }
    }

//    nc.insert({{2},{3}}, 1);

//    Summary *summary = nc.query({{}, {}});

//    if (summary) {
//        std::cout << summary->count << std::endl;
//    }

#endif


}



inline constexpr int operator"" _c ( char c )
{
    return ((int)c) - ((int)'A') + 1;
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------

int main() {

#if 0
    Nanocube nc({2, 1});
    nc.insert({{'C'_c,2},{1}}, 1);
    nc.insert({{'C'_c,2},{2}}, 2);
    nc.insert({{'B'_c,3},{2}}, 3);
    nc.insert({{'D'_c,1},{1}}, 4);
    nc.insert({{'B'_c,4},{2}}, 5);
#else
    Nanocube nc({1, 1, 1, 1});
//    nc.insert({{2},{1},{1},{1}}, 1);
//    nc.insert({{0},{0},{1},{2}}, 2);
    nc.insert({{0},{1},{0},{0}}, 1);
//    nc.insert({{1},{1},{2},{1}}, 4);
    nc.insert({{2},{0},{2},{0}}, 2);
    nc.insert({{0},{2},{0},{0}}, 3);
#endif

//    Nanocube nc({2});
//    nc.insert({{1,2}}, 1);
//    nc.insert({{1,3}}, 2);

    report::Report rep = buildReport(nc);

    //
    std::string dot_filename = "/tmp/g.dot";
    std::string pdf_filename = "/tmp/g.pdf";

    std::ofstream of(dot_filename);
    report::report_graphviz(of, rep);
    of.close();

    system(("dot -Tpdf " + dot_filename + " > " + pdf_filename).c_str());
    system(("open " + pdf_filename).c_str());

    // report::report_graphviz(std::cout, rep);

}

