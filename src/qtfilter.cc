#include "qtfilter.hh"

#include "geom2d/make_monotone.hh"

namespace qtfilter {

//-----------------------------------------------------------------------------
// Node Impl.
//-----------------------------------------------------------------------------

Node::Node() {
    for (int i=0;i<4;i++) {
        children[i] = nullptr;
    }
}

Node::~Node() {
    for (int i=0;i<4;i++) {
        if (children[i])
            delete children[i];
    }
}

void Node::clear() {
    for (int i=0;i<4;i++) {
        delete children[i];
        children[i] = nullptr;
    }
}

void Node::removeChildren(Flag flag) {
    for (int i=0;i<4;i++) {
        if (children[i] && children[i]->flag == flag) {
            delete children[i];
            children[i] = nullptr;
        }
    }
}

bool Node::isLeaf() const {
    return getNumChildren() == 0;
}

bool Node::isFull() const {
    return getNumChildren() == 4;
}

int Node::countChildren(Flag flag) const {
    int result = 0;
    for (int i=0;i<4;i++) {
        if (children[i] && children[i]->flag == flag) {
            ++result;
        }
    }
    return result;
}

auto Node::getChildren(int index) const -> Node* {
    return children[index];
}

auto Node::operator[](int index) const -> Node* {
    return children[index];
}

auto Node::getOrCreateChildren(int index) -> Node* {
    if (children[index] == nullptr)
        children[index] = new Node();
    return children[index];
}

auto Node::getNumChildren() const -> int {
    return (children[0] != nullptr ? 1 : 0) +
            (children[1] != nullptr ? 1 : 0) +
            (children[2] != nullptr ? 1 : 0) +
            (children[3] != nullptr ? 1 : 0);
}

//-----------------------------------------------------------------------------
// QuadTreeCreator::Action Impl.
//-----------------------------------------------------------------------------
QuadTreeCreator::Action::Action(QuadTreeCreator *creator,
       Node* parent,
       geom2d::Tile  parent_tile,
       int   child_index,
       ActionType type):
    creator { creator},
    parent{parent},
    parent_tile{parent_tile},
    child_index{child_index},
    type{type}
{}


auto QuadTreeCreator::Action::getTile() const -> geom2d::Tile {
    if (parent != nullptr) {
        return parent_tile + child_index;
    }
    else {
        return geom2d::Tile(0,0,0);
    }
}

auto QuadTreeCreator::Action::getNode() const -> Node* {
    if (parent == nullptr) {
        return creator->root;
    }
    else {
        return parent->getChildren(child_index);
    }
}



//-----------------------------------------------------------------------------
// QuadTreeCreator Impl.
//-----------------------------------------------------------------------------

bool QuadTreeCreator::next() {
    if (pending_actions.empty()) {
        pending_actions.push_back({this, nullptr, {0,0,-1}, 0, POST_PROCESS});
        pending_actions.push_back({this, nullptr, {0,0,-1}, 0, TEST});
        return true;
    }
    {
        // execute the scheduled ToDo action with
        // the top of the stack action
        if (next_todo == CONTINUE) {
            pending_actions.pop_back();
        }
        else if (next_todo == KEEP) {
            Action action = pending_actions.back();
            pending_actions.pop_back();
            if (action.parent == nullptr) {
                // root
                root = new Node();
                root->flag = Node::KEEP;
            }
            else {
                Node* child = action.parent->getOrCreateChildren(action.child_index);
                child->flag = Node::KEEP;
            }
        }
        else if (next_todo == SPLIT) {
            Action action = pending_actions.back();
            pending_actions.pop_back();

            Node* child = nullptr;
            if (action.parent == nullptr) {
                child = root = new Node();
            }
            else {
                child = action.parent->getOrCreateChildren(action.child_index);
            }

            // schedule a compression action
            // pending_actions.push_back({this, action.parent, action.parent_tile, action.child_index, POST_PROCESS});

            // pending
            for (int i=0;i<4;++i) {
                pending_actions.push_back({this, child, action.getTile(), i, POST_PROCESS});
                pending_actions.push_back({this, child, action.getTile(), i, TEST});
            }
        }

        next_todo = CONTINUE; // set default action for next

        // pending actions is not empty
        return !pending_actions.empty();

    }
}

QuadTreeCreator::Action &QuadTreeCreator::getAction()
{
    return pending_actions.back();
}

void QuadTreeCreator::split()
{
    this->next_todo = SPLIT;
}

void QuadTreeCreator::keep()
{
    this->next_todo = KEEP;
}

//-----------------------------------------------------------------------------
// QuadTreeIterator Impl.
//-----------------------------------------------------------------------------

QuadTreeIterator::QuadTreeIterator(Node *node, geom2d::Tile tile):
    root{node}, root_tile{tile}
{}

Node* QuadTreeIterator::getCurrentNode() {
    return stack.back().node;
}

geom2d::Tile  QuadTreeIterator::getCurrentTile() {
    return stack.back().tile;
}

bool QuadTreeIterator::next() {
    while (true) {
        if (stack.empty()) {
            stack.push_back({root,root_tile});
        }
        else {
            Item item = stack.back();
            Node*        parent = item.node;
            geom2d::Tile parent_tile = item.tile;

            stack.pop_back();
            for (int i=0;i<4;i++) {
                Node *child = parent->getChildren(i);
                geom2d::Tile child_tile { 2*parent_tile.x, 2*parent_tile.y, parent_tile.z+1 };
                if (child != nullptr) {
                    if (i == 0) {
                        stack.push_back( {child, child_tile} );
                    }
                    else if (i == 1) {
                        stack.push_back( {child, geom2d::Tile{child_tile.x+1, child_tile.y,   child_tile.z} });
                    }
                    else if (i == 2) {
                        stack.push_back( {child, geom2d::Tile{child_tile.x  , child_tile.y+1, child_tile.z} });
                    }
                    else if (i == 3) {
                        stack.push_back( {child, geom2d::Tile{child_tile.x+1, child_tile.y+1, child_tile.z} });
                    }
                }
            }
        }

        if (stack.empty())
            return false;
        if (!only_leaves || (only_leaves && stack.back().node->isLeaf()))
            return true;
    }
}


//
// Generate the quadtree filter
//

auto intersect(const geom2d::Polygon &original_polygon, int max_level, bool optimize) -> Node*
{

    // copy polygon
    geom2d::Polygon poly = original_polygon;

//    {
//        using namespace geom2d::io;
//        std::cout << poly << std::endl;
//    }


    poly.makeCCW(); // make sure it is oriented in counter-clock wise
    
    // poly.save("/tmp/ncserve-poly.poly");

    std::vector<geom2d::Polygon> convex_decomposition;
    geom2d::BoundingBox  convex_decomposition_bbox;

    for (geom2d::Polygon &p: geom2d::makeMonotone(poly)) {
        auto convex_partition = geom2d::convexPartition(p);
        for (geom2d::Polygon &convex_poly: convex_partition) {
            convex_decomposition.push_back(convex_poly);
            convex_decomposition_bbox.merge(convex_poly.getBoundingBox());
        }
    }

    // post-processing of a quadtree node (maybe compress full nodes)
    auto post_processing = [optimize](Node *node) {
        if (node != nullptr) {
            if (node->flag == Node::KEEP && node->isLeaf()) {
                node->flag = Node::KEEP_AND_FULL;
            }
            else {
                node->removeChildren(Node::NONE);
                bool compressed = false;
                if (optimize) {
                    if (node->countChildren(Node::KEEP_AND_FULL) == 4) {
                        node->clear(); // delete all children
                        node->flag = Node::KEEP_AND_FULL;
                        compressed = true;
                    }
                }
                if (!compressed && node->getNumChildren() > 0) {
                    node->flag = Node::KEEP;
                }
            }
        }
    };


    //
    bool intersect_borders = true;

    //
    QuadTreeCreator quadtree_creator;
    while (quadtree_creator.next()) {

        QuadTreeCreator::Action action = quadtree_creator.getAction();

        geom2d::Tile tile = action.getTile();

        if (action.type == QuadTreeCreator::POST_PROCESS) {

            post_processing(action.getNode());

        }
        else { // process action

            geom2d::BoundingBox tile_bbox = tile.getBoundingBox();

            bool can_split = tile.z < max_level;

            if (convex_decomposition_bbox.disjoint(tile_bbox)) {

                continue; //  node will be discarded
            }

            for (auto it=convex_decomposition.begin();it!=convex_decomposition.end();++it) {

                geom2d::Polygon& convex_poly = *it;

                const geom2d::BoundingBox& bbox = convex_poly.getBoundingBox();
                if (bbox.intersects(tile_bbox)) {

                    int count_inside = 0;

                    for (int i=0;i<4;i++) {
                        count_inside += convex_poly.inside(tile_bbox[i]);
                    }

                    if (count_inside == 4) { // tile_bbox is inside polygon
                        quadtree_creator.keep(); // flag next operation as a keep this won't be overriden
                        break;
                    }

                    else if (count_inside > 0 ||
                             tile_bbox.contains(bbox) || // tile_bbox might contain polygon and all bbox
                             convex_poly.boxCrossesBoundary(tile_bbox)) {
                        // there is an intersection between convex_poly and tile_bbox
                        if (!can_split && intersect_borders) {
                            quadtree_creator.keep(); // flag next operation as a keep
                            break;
                        }
                        else if (can_split){
                            quadtree_creator.split(); // flag next operation as a split this might be overriden
                        }
                    }
                    else {
                        // split
                    }
                }
            }
        } //
    }

//    QuadTreeIterator it(quadtree_creator.root, geom2d::Tile(0,0,0));
//    it.only_leaves = true;
//    while (it.next()) {
//        std::cout << "x:" << it.getCurrentTile().x
//                  << " y:" << it.getCurrentTile().y
//                  << " z:" << it.getCurrentTile().z
//                  << std::endl;
//    }

    return quadtree_creator.root;

}

}
