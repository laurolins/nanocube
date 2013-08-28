#include <QuadTree.hh>
#include <QuadTreeUtil.hh>
#include <TimeSeries.hh>
#include <FlatTree.hh>
#include <STree.hh>


typedef uint32_t TimeBin; // timeseries time bin type
typedef uint32_t TSCount; // timeseries count type


typedef timeseries::TimeSeries<TimeBin, TSCount> EventTimeSeries;

typedef flattree::FlatTree<EventTimeSeries> Dim2;
typedef flattree::FlatTree<Dim2>            Dim1;
typedef quadtree::QuadTree<1,Dim1>          Dim0;

typedef boost::mpl::vector<Dim0, Dim1, Dim2> PathListType;

typedef quadtree::Address<1> QAddr;
typedef flattree::Address    FAddr;

typedef stree::STree<PathListType, TimeBin> STreeType;


struct AddPoint {
    QAddr a0;
    FAddr a1;
    FAddr a2;
    AddPoint(QAddr a0, FAddr a1, FAddr a2, STreeType &stree, TimeBin t):
        a0(a0), a1(a1), a2(a2)
    {
        stree.setAddress(0, &a0);
        stree.setAddress(1, &a1);
        stree.setAddress(2, &a2);
        stree.add(t);
    }
};

int main()
{


    STreeType stree;

    AddPoint(QAddr(0,0,1), FAddr(weekday::Mon), FAddr(10), stree, 0);
    AddPoint(QAddr(0,0,1), FAddr(weekday::Mon), FAddr(11), stree, 0);
    AddPoint(QAddr(0,0,1), FAddr(weekday::Tue), FAddr(10), stree, 0);
    AddPoint(QAddr(0,0,1), FAddr(weekday::Tue), FAddr(11), stree, 0);
    AddPoint(QAddr(0,1,1), FAddr(weekday::Tue), FAddr(11), stree, 0);

    // computing statistics
    quadtree::Stats<1> stats;
    stats.initialize(stree.root);
    stats.dumpReport(std::clog);

    // dump report on timeseries
    Dim1::dump_ftlist(std::clog);
    Dim2::dump_ftlist(std::clog);
    EventTimeSeries::dump_tslist(std::clog);


    // find timeseries of everything
    auto n0 = stree.root.find(quadtree::Address<1>());

    flattree::Address emptyPath;

    {
        auto n1 = n0->getContent()->find(emptyPath);
        auto n2 = n1->getContent()->find(emptyPath);
        EventTimeSeries &et = *n2->getContent();
        std::clog << "===================================================" << std::endl;
        std::clog << "No Weekday Constraint: " << std::endl << "   ";
        et.dump(std::clog);
    }

    for (flattree::PathElement weekday=0;weekday<7;weekday++)
    {
        flattree::Address weekdayPath(weekday);
        auto n1 = n0->getContent()->find(weekdayPath);
        if (!n1)
            continue;
        auto n2 = n1->getContent()->find(emptyPath);
        EventTimeSeries &et = *n2->getContent();
        std::clog << "===================================================" << std::endl;
        std::clog << "Weekday: " << (int) weekday << std::endl << "   ";
        et.dump(std::clog);
    }

    for (flattree::PathElement hour=0;hour<24;hour++)
    {
        flattree::Address hourPath(hour);
        auto n1 = n0->getContent()->find(emptyPath);
        if (!n1)
            continue;
        auto n2 = n1->getContent()->find(hourPath);
        if (!n2)
            continue;
        EventTimeSeries &et = *n2->getContent();
        std::clog << "===================================================" << std::endl;
        std::clog << "Hour: " << (int) hour << std::endl << "   ";
        et.dump(std::clog);
    }


    for (flattree::PathElement weekday=0;weekday<7;weekday++)
    {
        flattree::Address weekdayPath(weekday);

        auto n1 = n0->getContent()->find(weekdayPath);
        if (!n1)
            continue;

        for (flattree::PathElement hour=0;hour<24;hour++)
        {
            flattree::Address hourPath(hour);

            auto n2 = n1->getContent()->find(hourPath);
            if (!n2)
                continue;

            EventTimeSeries &et = *n2->getContent();
            std::clog << "===================================================" << std::endl;
            std::clog << "Weekday: " << (int) weekday << " Hour: " << (int) hour << std::endl << "   ";
            et.dump(std::clog);
        }
    }


    // // computing statistics
    // quadtree::Stats<25> stats;
    // stats.initialize(sTree.root);
    // stats.dumpReport(std::clog);

    // dispatcher d;

    // std::cout << typeid(types).name() << std::endl;
    // std::cout << typeid(mpl::next<types>::type::item).name() << std::endl;
    // std::cout << typeid(mpl::next<mpl::next<types>::type>::type).name() << std::endl;

    return 0;
}
