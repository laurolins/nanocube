#include <cstdint>

#include <algorithm>

#include "QuadTree.hh"
#include "QuadTreeUtil.hh"
#include "TimeSeries.hh"
#include "FlatTree.hh"
#include "STree.hh"

//
// Tile
//

typedef uint16_t PixelSize;
typedef uint16_t PixelCoord;
typedef uint32_t PixelType;

template <typename TileID, typename PixelType>
struct Tile
{
    Tile(TileID id, PixelSize sideSize):
        id(id),
        sideSize(sideSize)
    {
        data = new PixelType[sideSize*sideSize];
        std::fill(data, data + (sideSize*sideSize), 0);
    }

    ~Tile()
    {  delete [] data; }

    PixelType& operator()(PixelCoord i, PixelCoord j) const
    {
        return data[i * sideSize + j];
    }

    TileID      id;
    PixelSize   sideSize;
    PixelType  *data;
};

typedef uint32_t TSTime; // timeseries time bin type
typedef uint32_t TSCount; // timeseries count type

typedef timeseries::TimeSeries<TSTime, TSCount> TS;

typedef flattree::FlatTree<TS> Dim2;
typedef flattree::FlatTree<Dim2>            Dim1;
typedef quadtree::QuadTree<1,Dim1>          Dim0;

typedef boost::mpl::vector<Dim0, Dim1, Dim2> STreeTypeList;

typedef typename Dim0::AddressType QAddr;
typedef typename Dim1::AddressType WAddr;
typedef typename Dim2::AddressType HAddr;

typedef stree::STree<STreeTypeList, TSTime> STreeType;

// quadtree tile
typedef Tile<QAddr, TSCount> QAddrTile;

//
// print QAddrTile
//

std::ostream& operator<<(std::ostream &os, const QAddrTile& tile)
{
    for (int i=0;i<tile.sideSize;i++)
    {
        for (int j=0;j<tile.sideSize;j++)
            os << std::setw(4) << tile(i,j);
        os << std::endl;
    }
    return os;
}

std::ostream& operator<<(std::ostream &os, const QAddr& addr)
{
    os << "Addr[x: "  << addr.x
       << ", y: "     << addr.y
       << ", level: " << addr.level
       << "]";
    return os;
}

//
// TileBuilder
//

struct TileBuilder
{
    TileBuilder():
        new_base_address(true),
        current_tile(nullptr)
    {}

    // TODO make STree signal this
    template <typename Query>
    void changeBaseAddress(int dimension, Query &query)
    {
        if (dimension == 0)
            new_base_address = true;
    }

    template <typename Query>
    void visit(TS &timeseries, Query &query)
    {
        QAddr addr = query.template getCurrentAddress<QAddr>();

        if (new_base_address)
        {
            QAddr base_addr = query.template getCurrentBaseAddress<QAddr>();
            // tile size
            PixelSize sideSize = 1 << (addr.level - base_addr.level);
            current_tile = new QAddrTile(base_addr, sideSize);
            new_base_address = false;
            tiles.push_back(current_tile);
        }

        // calculate relative address i and j
        QAddrTile &t = *current_tile;

        PixelCoord i = addr.x - t.id.x;
        PixelCoord j = addr.y - t.id.y;

        t(i, j) += timeseries.entries.back().count;

        std::cout << "Address: " << addr << std::endl;
        std::cout << "Tile: "    << i << ", " << j << std::endl;
        std::cout << "Visiting timeseries: " << std::endl;
        timeseries.dump(std::cout);
    }

public:

    bool  new_base_address;
    QAddrTile *current_tile;
    std::vector<QAddrTile*> tiles;

};

























struct AddPoint {
    QAddr a0;
    WAddr a1;
    HAddr a2;
    AddPoint(QAddr a0, WAddr a1, HAddr a2, STreeType &stree, TSTime t):
        a0(a0), a1(a1), a2(a2)
    {
        // STreeType::Address a;
        // a.set(a0);
        // a.set(a1);
        // a.set(a2);
        // stree.add(a, t);

        stree.setAddress(a0);
        stree.setAddress(a1);
        stree.setAddress(a2);
        stree.add(t);
    }
};


int main()
{
    STreeType stree;

    // this is adding points to stree
    AddPoint(QAddr(0,0,1), WAddr(weekday::Mon), HAddr(10), stree, 0);
    AddPoint(QAddr(0,0,1), WAddr(weekday::Mon), HAddr(11), stree, 0);
    AddPoint(QAddr(0,0,1), WAddr(weekday::Tue), HAddr(10), stree, 0);
    AddPoint(QAddr(0,0,1), WAddr(weekday::Tue), HAddr(11), stree, 0);
    AddPoint(QAddr(0,1,1), WAddr(weekday::Tue), HAddr(11), stree, 0);

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
    WAddr a_w1(weekday::Mon);
    WAddr a_w2(weekday::Tue);
    HAddr a_h1(10);

    // create a query object
    stree::Query<STreeType> query;
    query.add(a_s1, 8); // root
    query.add(a_w1, 0); // monday
    query.add(a_w2, 0); // tuesday
    query.add(a_h1, 0); // 10

    //
    TileBuilder tileBuilder;
    stree.visit(query, tileBuilder);

    assert(tileBuilder.current_tile);

    std::cout << *tileBuilder.current_tile;

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
