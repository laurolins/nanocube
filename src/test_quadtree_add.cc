#include <iostream>
#include <datatiles/QuadTree.hh>
#include <datatiles/QuadTreeUtil.hh>

using namespace quadtree;

struct NullContent {};



typedef QuadTree<25, NullContent>        QTree;
typedef typename QTree::NodeType        QNode;
typedef typename QTree::AddressType     QAddr;
typedef typename QTree::NodeStackType   QStack;

struct Visitor {

    void visit(QNode*, QAddr &qaddr) {
        std::cout << "---------------------> " << qaddr << std::endl;
    }

};


int main()
{

    QTree  qtree;
    QStack stack;

    QAddr qaddr;
    qaddr.setLevelCoords(1<<12 | 1<<15,1<<15 | 1<<13,25);
    qtree.trailProperPath(qaddr, stack);

//    for (int i=0;i<4;i++) {
//        for (int j=0;j<4;j++) {
//            qaddr.setLevelCoords(i,j,2);
//            qtree.trailProperPath(qaddr, stack);
//        }
//    }

//    qaddr.setLevelCoords(0,0,1);
//    qtree.trailProperPath(qaddr, stack);
//    qaddr.setLevelCoords(1,0,1);
//    qtree.trailProperPath(qaddr, stack);
//    qaddr.setLevelCoords(0,1,1);
//    qtree.trailProperPath(qaddr, stack);
//    qaddr.setLevelCoords(1,1,1);
//    qtree.trailProperPath(qaddr, stack);

//    qaddr.setLevelCoords(2,2,2);
//    qtree.trailProperPath(qaddr, stack);

//    qaddr.setLevelCoords(0,0,2);
//    qtree.trailProperPath(qaddr, stack);

    Stats<QTree> stats;
    stats.initialize(qtree);

    stats.dumpReport(std::cout);

//    QAddr min_addr, max_addr;
//    min_addr.setLevelCoords(0, 0, 2);
//    max_addr.setLevelCoords(3, 3, 2);
    Visitor visitor;
    QAddr root_addr;
    qtree.visitSubnodes(root_addr, 25, visitor);

}





