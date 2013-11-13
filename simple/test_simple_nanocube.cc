#include <simple_nanocube.hh>
#include <simple_nanocube_iterator.hh>
#include <report.hh>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>

#include <algorithm>
#include <iterator>
#include <sstream>

#include <random>

#include <log.hh>

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
// TestInstance
//-----------------------------------------------------------------------------

struct TestInstance {

    TestInstance() = default;

    TestInstance(const TestInstance &other) = delete;
    TestInstance& operator=(const TestInstance &other) = delete;

    TestInstance(TestInstance &&other) = default;
    TestInstance& operator=(TestInstance &&other) = default;

    void addAddress(const Address &addr);

    Nanocube nc;
    std::vector<Address> data;

};

//-----------------------------------------------------------------------------
// Test
//-----------------------------------------------------------------------------

struct Test {
public:

    struct DimensionSpec {
        DimensionSpec(int num_levels, int num_labels);

        void random(Address &addr) const;

        int num_levels; // at least one
        int num_labels; // every level has the same possibility of labels
    };

public:

    Test(const std::vector<DimensionSpec> &spec);
    TestInstance randomTestInstance(int num_points) const;
    std::vector<int> getLevelsByDimension() const;

public:

    std::vector<DimensionSpec> spec;

};

static Label random_label(int n) {
    static std::mt19937 gen(17);
    std::uniform_int_distribution<> dis(0,n-1);
    return dis(gen);
}

//-----------------------------------------------------------------------------
// Test::DimensionSpec
//-----------------------------------------------------------------------------

Test::DimensionSpec::DimensionSpec(int num_levels, int num_labels):
    num_levels (num_levels),
    num_labels (num_labels)
{}

void Test::DimensionSpec::random(Address &addr) const
{
    addr.appendDimension();
    for (int i=0;i<num_levels;++i) {
        addr.appendLabel(random_label(num_labels));
    }
}


//-----------------------------------------------------------------------------
// Test
//-----------------------------------------------------------------------------

std::vector<int> Test::getLevelsByDimension() const {
    std::vector<int> result(this->spec.size());
    std::transform(spec.begin(),
                   spec.end(),
                   result.begin(),
                   [](const DimensionSpec& dim_spec) -> int { return dim_spec.num_levels; } );

    std::stringstream ss;
    std::copy(result.begin(), result.end(), std::ostream_iterator<Object>(ss, " "));
    std::cout << ss.str() << std::endl;

    return result;
}

Test::Test(const std::vector<Test::DimensionSpec> &spec):
    spec(spec)
{}

TestInstance Test::randomTestInstance(int num_points) const {
    TestInstance test_instance;

    // generate addresses
    for (int i=0;i<num_points;++i) {
        Address addr;
        for (const DimensionSpec &dim_spec: this->spec) {
            dim_spec.random(addr);
        }
        std::cout << "Address " << i << " = " << addr << std::endl;
        test_instance.addAddress(addr);
    }

    //
    test_instance.nc = Nanocube(getLevelsByDimension());
    int obj = 0;
    for (Address& addr: test_instance.data) {
        std::cout << "Inserting " << addr << std::endl;
        test_instance.nc.insert(addr, ++obj);
    }

    return test_instance;
}

//-----------------------------------------------------------------------------
// TestInstance
//-----------------------------------------------------------------------------

void TestInstance::addAddress(const Address &addr)
{
    this->data.push_back(addr);
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------

int main() {
    for (int i=0;i<1;++i) {
        Test test({ {2,16}, {3,16}, {1,16}, {5,16} });
        std::cout << "Test " << i << std::endl;
        auto instance = test.randomTestInstance(7);


        {
            report::Report rep = buildReport(instance.nc);

            //
            std::string dot_filename = "/tmp/g.dot";
            std::string pdf_filename = "/tmp/g.pdf";

            std::ofstream of(dot_filename);
            report::report_graphviz(of, rep);
            of.close();

            system(("dot -Tpdf " + dot_filename + " > " + pdf_filename).c_str());
            system(("open " + pdf_filename).c_str());
        }

    }
}


