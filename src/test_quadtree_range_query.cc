#include <iostream>
#include <datatiles/QuadTree.hh>

using namespace quadtree;

struct NullContent {};



typedef QuadTree<3, NullContent>        QTree;
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

    for (int i=0;i<8;i++) {
        for (int j=0;j<8;j++) {
            QAddr qaddr;
            qaddr.setLevelCoords(i,j,3);
            qtree.trailProperPath(qaddr, stack);
        }
    }

    QAddr min_addr, max_addr;
    min_addr.setLevelCoords(2, 4, 3);
    max_addr.setLevelCoords(5, 6, 3);

    Visitor visitor;

    qtree.visitMaximalNodesInRange(min_addr, max_addr, visitor);

}





