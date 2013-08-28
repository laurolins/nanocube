#typedef int32_t TimeTreeID

static const int32_t UNDEFINED = -1;

struct QuadTreeNode
{
TimeTreeNodeID time_tree_node;
};

struct NodeLinkInfo
{
    unsigned char bits; // control bits encoding category and key of the link

    int  getKey();      // if WEEKDAY then 0 = Sunday, 1 = Monday ...
                        // else if HOUR then 0 to 24

    int  getCategory(); // Link category: WEEKDAY or HOUR

    bool isOwner();     // this link is owned by the current context
}


template<Content>
struct TimeTreeNode
{
    // Note that for the purpose of saving storage space we keep separate
    // vectors of childInfo and childID

    // Replace this vector by a compact vector: that stored in maximum
    // 255 children. (maybe define two bytes for capacity and count)
    // for now k
    std::vector<NodeLinkInfo> childInfo;   
    std::vector<NodeID>       childID;     // maybe an unordered map
};
 
//
void updateParentTimeTree(QuadTreeNode &parent, 
                              QuadTreeNode &child, 
                              Point &point, 
                              TimeTreeNode &childHourUpdatedNode,
                              TimeTreeNode &childWeekdayUpdatedNode)
{

if (parent.timeTree == UNDEFINED)
{
parent.timeTreeID =  child.timeTreeID; // copy reference
parent.setOwnerOfTimeTree(false);
return;
}

else if (parent.timeTreeID == child.timeTreeID)
{
return;
}

//
TimeTreeNode parentHourUpdateNode;
TimeTreeNode parentHourUpdateNode;

}







// // Quadtree simultaneous address reduction in x and y.

// struct Node 
// {

// }

// // address mechanism is uniform


// 25 || 25 -> 3 -> 5


// Parallel(2 x 25) -> Series(3,5)


// Address


// Address needs 25 + 25 + 3 + 5 = 58 bits


// Quadtree -> 7-ary tree -> 24-ary tree






// // 3 would be an Octree
// ParallelAddressTree<2, 25, [SeriesTree<3,5,TimeSeries>, TimeSeries] >


// // Addr: level, 25 bits x, 25 bits y


// Nested address spaces

