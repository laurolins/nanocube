#include <datatiles/QuadTree.hh>
#include <datatiles/QuadTreeUtil.hh>
#include <datatiles/TimeSeries.hh>
#include <datatiles/FlatTree.hh>
#include <datatiles/STree.hh>

typedef uint32_t TimeBin; // timeseries time bin type
typedef uint32_t TSCount; // timeseries count type

typedef timeseries::TimeSeries<TimeBin, TSCount> EventTimeSeries;

typedef flattree::FlatTree<EventTimeSeries> Dim2;
typedef flattree::FlatTree<Dim2>            Dim1;
typedef quadtree::QuadTree<1,Dim1>          Dim0;

typedef boost::mpl::vector<Dim0, Dim1, Dim2> PathListType;

typedef typename Dim0::AddressType QAddr;
typedef typename Dim1::AddressType WAddr;
typedef typename Dim2::AddressType HAddr;

typedef stree::STree<PathListType, TimeBin> STreeType;

struct AddPoint {
    QAddr a0;
    WAddr a1;
    HAddr a2;
    AddPoint(QAddr a0, WAddr a1, HAddr a2, STreeType &stree, TimeBin t):
        a0(a0), a1(a1), a2(a2)
    {
        stree.setAddress(a0);
        stree.setAddress(a1);
        stree.setAddress(a2);
        stree.add(t);
    }
};

std::ostream& operator<<(std::ostream &os, const QAddr& addr)
{
    os << "Addr[x: "  << addr.x
       << ", y: "     << addr.y
       << ", level: " << addr.level
       << "]";
    return os;
}

struct AccumulateVisitor
{
    template <typename Query>
    void changeBaseAddress(int dimension, Query &query)
    {}

    template <typename Query>
    void visit(EventTimeSeries &timeseries, Query &query)
    {
        QAddr addr = query.template getCurrentAddress<QAddr>();


        std::cout << "Address: " << addr << std::endl;
        std::cout << "Visiting timeseries: " << std::endl;
        timeseries.dump(std::cout);
    }
};

int main()
{
    STreeType stree;

    // this is adding points to stree
    AddPoint(QAddr(0,0,1), WAddr(1), HAddr(10), stree, 0);
    AddPoint(QAddr(0,0,1), WAddr(1), HAddr(11), stree, 0);
    AddPoint(QAddr(0,0,1), WAddr(2), HAddr(10), stree, 0);
    AddPoint(QAddr(0,0,1), WAddr(2), HAddr(11), stree, 0);
    AddPoint(QAddr(0,1,1), WAddr(2), HAddr(11), stree, 0);

    // computing statistics
//    quadtree::Stats<Dim0> stats;
//    stats.initialize(stree.root);
//    stats.dumpReport(std::clog);

    // dump report on timeseries
//    Dim1::dump_ftlist(std::clog);
//    Dim2::dump_ftlist(std::clog);
//    EventTimeSeries::dump_tslist(std::clog);

    //
    QAddr a_s1;
    WAddr a_w1(1);
    WAddr a_w2(2);
    HAddr a_h1(11);

    // create a query object
    stree::Query<STreeType> query;
    query.add(a_s1, 1); // root
    query.add(a_w1, 0); // monday
    query.add(a_w2, 0); // tuesday
    query.add(a_h1, 0); // 10

    //
    AccumulateVisitor a;
    stree.visit(query, a);

    return 0;

}




























#if 0


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


#endif
