#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <numeric>
#include <cassert>

#include "infix_iterator.hh"
#include "labeled_tree.hh"

#include "tokenizer.hh"

namespace polycover {

	//-----------------------------------------------------------------------------
	// tree
	//-----------------------------------------------------------------------------

	namespace labeled_tree {

		//-----------------------------------------------------------------------
		// Iterator::Action
		//-----------------------------------------------------------------------

		Iterator::Action::Action():
			type(POP)
		{}

		Iterator::Action::Action(const Node* parent,
					 const Node* child,
					 const ChildLabel label,
					 Depth depth):
			type(PUSH),
			parent(parent),
			child(child),
			label(label),
			depth(depth)
		{}

		Iterator::Iterator(const Node& node) {
			// stack.push_back(Action()); // pop action
			stack.push_back(Action(nullptr, &node, NONE, 0));
		}

		Iterator::Action* Iterator::next() {
			if (stack.size()) {
				Action a = stack.back();
				stack.pop_back();
				auto node = a.child;
				if (a.type == Action::PUSH) {
					for (int i=3;i>=0;--i) {
						ChildLabel child_label = (ChildLabel) i;
						Node* child = node->children[i].get();
						if (child) {
							stack.push_back(Action()); // pop action
							stack.push_back(Action(node, child, child_label, a.depth+1));
						}
					}
				}
				last_action = a;
				return &last_action;
			}
			else {
				return nullptr;
			}
		}

		//-----------------------------------------------------------------------
		// io
		//-----------------------------------------------------------------------

		std::ostream& operator<<(std::ostream& os, const Path& path);


		std::ostream& operator<<(std::ostream& os, const Node& root) {
			// std::stringstream ss;
			Iterator it(root);
			//            Iterator::Action::Type previous_type;// = Iterator::Action::POP;
			Iterator::Action *a;
			it.next(); // get rid of root
			while ((a = it.next())) {
				if (a->type == Iterator::Action::POP) {
					os << "<";
				}
				else {
					os << (int) a->label;
				}
				//previous_type = a->type;
			}
			return os; // .str();
		}

		std::ostream& json(std::ostream& os, const Node& root) {
			os << "{ \"type\":\"decomposition\", \"code\":\"";
			// std::stringstream ss;
			Iterator it(root);
			//            Iterator::Action::Type previous_type;// = Iterator::Action::POP;
			Iterator::Action *a;
			it.next(); // get rid of root
			while ((a = it.next())) {
				if (a->type == Iterator::Action::POP) {
					os << "<";
				}
				else {
					os << (int) a->label;
				}
				//                previous_type = a->type;
			}
			os << "\"}";
			return os; // .str();
		}

		std::ostream& text(std::ostream& os, const Node& root) {
			int level = 0;
			std::function<void(const Node&)> print = [&os, &level, &print](const Node& u) {
				std::string prefix(4 * level, '.');
				os << prefix << u.path() << ", tag:" << u.tag << std::endl;
				++level;
				for (auto &c: u.children) {
					if (c.get()) {
						print(*c.get());
					}
				}
				--level;
			};

			print(root);
			return os;
		}

		//-----------------------------------------------------------------------
		// io
		//-----------------------------------------------------------------------

		std::istream& operator>>(std::istream& is, Node& root) {
			std::vector<Node*> stack;
			stack.push_back(&root);
			while (true) {
				char ch;
				is >> ch;
				if (!is.good())
					break;
				if (ch == '0' || ch == '1' || ch == '2' || ch == '3') {
					ChildLabel lbl = (int) ch - (int) '0';
					stack.push_back(stack.back()->advance(lbl));
				}
				else if (ch == '<') {
					stack.pop_back();
				}
				else {
					break;
				}
			}
			return is;
		}

		//-----------------------------------------------------------------------
		// Tag Impl.
		//-----------------------------------------------------------------------

		Tag::Tag(Type type, int iteration):
			tag_type(type), iteration(iteration)
		{}

		void Tag::update(const Tag& tag) {
			*this = tag;

			//        if (tag_type == NOT_FIXED) {
			//            *this = tag;
			//        }
			//        else if (iteration < tag.iteration) {
			//            *this = tag;
			//        }
			//        else if (iteration == tag.iteration) {
			//            if (tag_type == FIXED || tag.tag_type == FIXED) {
			//                tag_type = FIXED;
			//            }
			//        }
			//        else if (iteration < tag.iteration) {
			//            // throw std::runtime_error("ooops");
			//        }
		}

		std::ostream &operator<<(std::ostream& os, const Tag::Type& ttype) {
			switch (ttype) {
			case Tag::FIXED:
				os << "\"FIXED\"";
				break;
			case Tag::FIXED_ANCESTOR:
				os << "\"FIXED_ANCESTOR\"";
				break;
			case Tag::NOT_FIXED:
				os << "\"NOT_FIXED\"";
				break;
			default:
				break;
			}
			return os;
		}


		std::ostream &operator<<(std::ostream& os, const Tag& tag) {
			os << "{ \"type\":" << tag.tag_type << ", \"iter\":" << tag.iteration << "}";
			return os;
		}


		//-----------------------------------------------------------------------
		// Node Impl.
		//-----------------------------------------------------------------------

		Node::Node(Node* parent, ChildLabel label_to_parent):
			parent(parent),
			label_to_parent(label_to_parent)
		{}

		Node::~Node() {
			children[0].reset();
			children[1].reset();
			children[2].reset();
			children[3].reset();
		}

		Path Node::path() const {
			Path node_path;
			auto u = this;
			while (u->parent) {
				node_path.data.push_back(u->label_to_parent);
				u = u->parent;
			}
			node_path.reverse();
			return node_path;
		}

		void Node::trim(int layers_to_go) {
			if (layers_to_go <= 0)
				throw std::runtime_error("Cannot trim to 0 layers");

			if (layers_to_go == 1) { // this will be the last layer
				children[0].reset();
				children[1].reset();
				children[2].reset();
				children[3].reset();
			}
			else {
				for (int i=0;i<4;++i) {
					if (children[i].get()) {
						children[i].get()->trim(layers_to_go-1);
					}
				}
			}
		}

		Node *Node::advance(ChildLabel child_label)
		{
			if (child_label == 1 && path().equalTo(Path({1,2})))
				child_label = (child_label + 1) - 1;


			Node* child = nullptr;
			if (child_label == NONE)
				throw std::runtime_error("oops");
			child = children[child_label].get();
			if (!child) {
				children[child_label].reset((child = new Node(this, child_label)));
			}
			return child;
		}

		void Node::deleteChild(ChildLabel child_label)
		{
			if (child_label == NONE)
				throw std::runtime_error("oops");
			children[child_label].reset();
		}

		void Node::deleteAllChildren()
		{
			children[0].reset();
			children[1].reset();
			children[2].reset();
			children[3].reset();
		}

		bool Node::split() {

			//        if (parent && parent->path().equalTo(Path({1,2})))
			//            tag.iteration += 1 - 1;

			// make current node be a leaf on a tree
			std::vector<Node*> path;
			auto n = this;
			while (n && n->tag.tag_type == Tag::NOT_FIXED) {
				path.push_back(n);
				n = n->parent;
			}

			if (path.size()) {
				auto iter = tag.iteration;
				// assume p is fixed
				for (auto it = path.rbegin(); it != path.rend(); ++it) {
					auto p = (*it)->parent;
					if (p->tag.tag_type == Tag::FIXED_ANCESTOR)
						break;

					if (p->tag.tag_type != Tag::FIXED)
						throw std::runtime_error("Needs to be a fixed vertex!");

					if (p->getNumChildren() > 1)
						throw std::runtime_error("Can only split fixed vertex if no children exist");

					p->advance(0)->updateTag(Tag(Tag::FIXED,iter));
					p->advance(1)->updateTag(Tag(Tag::FIXED,iter));
					p->advance(2)->updateTag(Tag(Tag::FIXED,iter));
					p->advance(3)->updateTag(Tag(Tag::FIXED,iter));
					p->updateTag(Tag(Tag::FIXED_ANCESTOR,iter));
				}
				return true;
			}
			else {
				return false;
			}

			//        if (tag.tag_type == Tag::FIXED) {
			//            if (this->getNumChildren() > 0)
			//                throw std::runtime_error("Can only split fixed vertex if no children exist");
			//            auto iter = tag.iteration;
			//            this->advance(0)->updateTag(Tag(Tag::FIXED,iter));
			//            this->advance(1)->updateTag(Tag(Tag::FIXED,iter));
			//            this->advance(2)->updateTag(Tag(Tag::FIXED,iter));
			//            this->advance(3)->updateTag(Tag(Tag::FIXED,iter));
			//            this->tag = Tag(Tag::FIXED_ANCESTOR,iter);
			//            return true;
			//        }
			//        else {
			//            return false;
			//        }
		}

		int Node::getNumChildren() const {
			return
				(children[0] ? 1 : 0) +
				(children[1] ? 1 : 0) +
				(children[2] ? 1 : 0) +
				(children[3] ? 1 : 0);
		}

		struct NodeOptimizeException: public std::exception {
			NodeOptimizeException() = default;
		};

		int Node::optimize() { // returns 1 if it was optimized to be a leaf, 0 otherwise
			// if node has a FIXED tag, then get rid
			// of all its children. Assuming a node
			// that was fixed and then updated with
			// a larger iteration number as an FIXED_ANCESTOR
			// becomes FIXED_ANCESTOR
			if (tag.tag_type == Tag::FIXED) {
				for (auto &ptr: children)
					ptr.reset();
				return 1; // it is a leaf now
			}
			else if (tag.tag_type == Tag::FIXED_ANCESTOR) {
				int children_count  = 0;
				int children_leaves = 0;
				for (int i=0;i<4;++i) {
					if (children[i].get()) {
						++children_count;
						children_leaves += children[i].get()->optimize();
					}
				}
				if (children_leaves == 4) {
					for (int i=0;i<4;++i)
						children[i].reset(); // get rid of children
					children_count = 0;
				}
				return children_count == 0 ? 1 : 0;
			}
			//            else if (tag.tag_type == Tag::NOT_FIXED && !parent && getNumChildren() == 0) {
			//                // single NOT FIXED node
			//                return 1;
			//            }
			else {
				//                auto u = this;
				//                int level = 0;
				//                while (u->parent) {
				//                    ++level;
				//                    u = u->parent;
				//                }
				//                std::cerr << "level of the problem: " << level << std::endl;
				// std::runtime_error("Optimizing a node that is NOT_FIXED"); // expecting all nodes to be NOT_FIXED
				throw NodeOptimizeException(); //
			}
		}


		int Node::pack() { // returns 1 if it was optimized to be a leaf, 0 otherwise
			int children_count  = 0;
			int children_leaves = 0;
			for (int i=0;i<4;++i) {
				if (children[i].get()) {
					++children_count;
					children_leaves += children[i]->pack();
				}
			}
			if (children_leaves == 4) {
				for (int i=0;i<4;++i)
					children[i].reset(); // get rid of children
				children_count = 0;
			}
			return children_count == 0 ? 1 : 0;
		}


		Summary Node::getSummary() const {
			return Summary(*this);
		}

		void Node::updateTag(Tag tag) { // make is a fixed node

#if 0
			std::deque<int> labels;
			auto u = this;
			while (u->parent != nullptr) {
				labels.push_front((int) u->label_to_parent);
				u = u->parent;
			}
			std::stringstream ss;
			ss << "...update tag of path [ ";
			std::copy(labels.begin(),
				  labels.end(),
				  infix_ostream_iterator<int>(ss, ","));
			ss << " ] from " << this->tag;
#endif
			this->tag.update(tag);
#if 0
			ss << " to " << this->tag;
			ss << std::endl;
			std::cerr << ss.str();
#endif
		}

		bool Node::isFixed() const {
			return this->tag.tag_type != Tag::NOT_FIXED;
		}


		bool Node::isLeaf() const {
			return getNumChildren() == 0;
		}

		//    void Node::fix(int iteration) {
		//
		//        //
		//        // iteration is positive when it is a proper
		//        // fix (meaning whole subtree is contained)
		//        //
		//        // iteration is negative when it is being
		//        // fixed because of a child node
		//        //
		//        // Note that a positive fix has precedence
		//
		//        auto current_iteration = abs(fix_iteration);
		//        auto new_iteration = abs(iteration);
		//        if (new_iteration > current_iteration) {
		//            fix_iteration = new_iteration;
		//        }
		//        else if (new_iteration == current_iteration && iteration > 0) {
		//            fix_iteration = iteration; // a proper fix operation has priority
		//        }
		//        else {
		//            throw std::runtime_error("ooops");
		//        }
		//
		//    }


		//-----------------------------------------------------------------------
		// LabeledTree
		//-----------------------------------------------------------------------

		LabeledTree::LabeledTree(std::unique_ptr<Node>&& root):
			_root(std::move(root))
		{}

		LabeledTree::LabeledTree(const LabeledTree& other) {
			*this = other;
		}

		void LabeledTree::clear() {
			_root.reset();
		}

		void LabeledTree::swap(LabeledTree& other) {
			_root.swap(other._root);
		}

		LabeledTree::LabeledTree(int x, int y, int level) {
			CoverTreeEngine engine;
			engine.goTo(Path{ maps::Tile{x, y, level} });
			engine.consolidate();
			engine.swap(_root);
		}

		LabeledTree::LabeledTree(const Code& code) {
			// code for empty tree is anything that starts with a pop '<'
			if (code.size() > 0 && code[0] == '<') {
				// empty tree!
				return;
			}

			CoverTreeEngine engine;
			for (auto ch: code) {
				switch (ch) {
				case '<':
					engine.rewind();
					break;
				case '0':
					engine.advance(0);
					engine.consolidate();
					break;
				case '1':
					engine.advance(1);
					engine.consolidate();
					break;
				case '2':
					engine.advance(2);
					engine.consolidate();
					break;
				case '3':
					engine.advance(3);
					engine.consolidate();
					break;
				default:
					throw std::runtime_error("ooops");
				}
			}

			engine.swap(_root);
		}

		LabeledTree& LabeledTree::operator=(const LabeledTree& other) {
			// TODO: add a more elegant way to do this
			if (!other.empty()) {
				LabeledTree copy { other.code() };
				copy.swap(*this);
			}
			return *this;
		}

		LabeledTree LabeledTree::operator-() const {
			LabeledTree all(""); // empty code is everything
			return all - *this;
		}

		Code LabeledTree::code() const {
			if (empty()) {
				return std::string("<");
			}
			else {
				std::stringstream ss;
				ss << *_root.get();
				return ss.str();
			}
		}

		bool LabeledTree::empty() const {
			return _root.get() == nullptr;
		}

		Node* LabeledTree::root() {
			return _root.get();
		}

		const Node* LabeledTree::root() const {
			return _root.get();
		}

		void LabeledTree::optimize() {
			if (_root) {
				try {
					_root->optimize();
				}
				catch(NodeOptimizeException &) {
					// corner case where root node is NOT_FIXED
					// after advance/rewind/consolidate process
					_root.reset();
				}
			}
		}

		void LabeledTree::pack() {
			if (_root) {
				_root->pack();
			}
		}


		/* @TODO(llins): continue from here */
		struct CoverageReportItem {

			CoverageReportItem() = default;
			CoverageReportItem(Node *node, int tilex, int tiley, int level)
			{
				this->node   = node;
				this->tilex  = tilex;
				this->tiley  = tiley;
				this->level  = level;
				this->cursor = 0;
			}

			Node          *node;
			/*  0 is first encounter of a node */
			/*  1 is the last encounter of a node */
			int           cursor;
			int           tilex;
			int           tiley;
			int           level;
		};

		//-----------------------------------------------------------------------
		// TileCoverage
		//-----------------------------------------------------------------------
		TileCoverage::TileCoverage(int tilex, int tiley, int level, float coverage):
			tilex(tilex), tiley(tiley), level(level), coverage(coverage)
		{}

		void
			coverage_report_fully_covered_tiles(int tilex, int tiley, int level, int target_level, std::vector<TileCoverage> *report)
			{
				assert(level <= target_level);
				if (level == target_level) {
					report->push_back(TileCoverage(tilex, tiley, level, 1.0f));
				} else {
					for (int i=0;i<4;++i) {
						int tx = tilex * 2 + ((i & 1) ? 1 : 0);
						int ty = tiley * 2 + ((i & 2) ? 1 : 0);
						coverage_report_fully_covered_tiles(tx, ty, level+1, target_level, report);
					}
				}
			}

		std::vector<TileCoverage> LabeledTree::coverage_report(int level)
		{
			std::vector<TileCoverage> result;

			// std::uint64_t leaf_area;

			/*
			 * navigate tree and for each node at level "level"
			 * start counting how many leaves of each different
			 * size is covered
			 */
			Node *node = this->root();

			if (node == 0) {
				return result;
			}

			std::vector<CoverageReportItem> stack;
			stack.push_back(CoverageReportItem(node, 0, 0, 0));

			static const int HARD_CODED_MAX_LEVEL = 25;

			std::uint64_t item_to_report_total_area   = 1ull << (2 * (HARD_CODED_MAX_LEVEL - level));
			std::uint64_t item_to_report_covered_area = 0;
			CoverageReportItem item_to_report_storage;
			CoverageReportItem *item_to_report = 0;

			while(stack.size()) {
				auto top = stack.back();
				auto top_index = stack.size()-1;
				// std::cout << "top node is " << top.node << std::endl;
				if (top.cursor == 0) {
					/* first encounter with a node */
					int num_children = 0;
					for (auto i=0;i<4;++i) {
						Node *child = top.node->children[i].get();
						// std::cout << "child " << i << " is " << child << std::endl;
						if (child) {
							stack.push_back(CoverageReportItem(child,
											   top.tilex * 2 + ((i & 1) ? 1 : 0),
											   top.tiley * 2 + ((i & 2) ? 1 : 0),
											   top.level + 1));
							++num_children;
						}
					}
					if (num_children == 0) {
						/* leaf node */
						if (top.level == level) {
							assert(item_to_report == 0);
							result.push_back(TileCoverage(top.tilex, top.tiley, top.level, 1.0f));
							stack.pop_back();
						} else if (top.level < level) {
							assert(item_to_report == 0);
							// artificially loop through all the tiles x levels deeper
							coverage_report_fully_covered_tiles(top.tilex, top.tiley, top.level, level, &result);
							stack.pop_back();
						} else {
							assert(item_to_report != 0);
							/* accumulate on current item to report */
							item_to_report_covered_area += 1ull << 2*(HARD_CODED_MAX_LEVEL - top.level);
							stack.pop_back();
						}
					} else {
						stack[top_index].cursor = 1;
						if (top.level == level) {
							assert(item_to_report == 0);
							item_to_report_storage = top;
							item_to_report = &item_to_report_storage;
							item_to_report_covered_area = 0;
						}
					}
				} else {
					if (top.level == level) {
						// assert(item_to_report == top.node);
						double coverage = (double) item_to_report_covered_area / (double) item_to_report_total_area;
						result.push_back(TileCoverage(top.tilex, top.tiley, top.level, (float) coverage));
						item_to_report = 0;
						item_to_report_covered_area = 0;
					}
					stack.pop_back();
				}
			}

			return result;
		}

		//-----------------------------------------------------------------------
		// Summary Impl.
		//-----------------------------------------------------------------------

		struct Item {
			Item() = default;
			Item(int level, const Node* node):
				level(level), node(node)
			{}
			int level;
			const Node *node;
		};

		Summary::Summary(const Node& tree) {
			this->nodes_per_num_children.resize(5,0);

			std::vector<Item> stack;
			stack.push_back({0, &tree});
			while (!stack.empty()) {
				Item item = stack.back();
				stack.pop_back();

				++num_nodes;
				nodes_per_num_children[item.node->getNumChildren()]++;

				if (static_cast<int>(nodes_per_level.size()) <= item.level) {
					nodes_per_level.resize(item.level+1, 0);
				}
				++nodes_per_level[item.level];

				for (int i=3;i>=0;--i) {
					const Node* child = item.node->children[i].get();
					if (child)
						stack.push_back({item.level+1, child});
				}
			}
		}

		std::size_t Summary::getNumLevels() const {
			return nodes_per_level.size();
		}

		std::size_t Summary::getNumLeaves() const {
			return nodes_per_num_children[0];
		}

		std::size_t Summary::getNumNodes()  const {
			return num_nodes;
		}

		std::size_t Summary::getFirstLevelWithTwoOrMoreNodes() const {
			for (std::size_t i=0; i<nodes_per_level.size(); ++i) {
				if (nodes_per_level[i] > 1)
					return i;
			}
			return nodes_per_level.size();
		}


		std::ostream& operator<<(std::ostream& os, const Summary& summary) {
			os << "...cover" << std::endl;
			os << "......size:       " << summary.getNumNodes()  << std::endl;
			os << "......leaves:     " << summary.getNumLeaves() << std::endl;
			os << "......levels:     "
				<< "[" << summary.getFirstLevelWithTwoOrMoreNodes()-1
				<< "," << summary.getNumLevels()-1 << "]" << std::endl;
			os << "......num levels: " << summary.getNumLevels();
			return os;
		}

		//-----------------------------------------------------------------------
		// Path Impl.
		//-----------------------------------------------------------------------

		Path::Path(const maps::Tile& tile) {
			for (auto i=0;i<tile.getZoom();++i) {
				data.push_back(tile.getSubTileLabel(i));
			}
		}

		Path::Path(const std::vector<ChildLabel> &labels) {
			data = labels;
		}

		void Path::setTile(const maps::Tile& tile) {
			auto x = tile.x.quantity;
			auto y = tile.y.quantity;
			auto z = tile.getZoom();

			data.resize(z);

			auto mask = 1 << (z - 1);
			for (auto i=0;i<z;++i) {
				data[i] = ((x & mask) != 0 ? 1 : 0) + ((y & mask) != 0 ? 2 : 0);
				mask >>= 1; // shift right one bit
			}
			//        auto x = this->x.quantity;
			//        auto y = this->y.quantity;
			//        auto num_transitions = this->zoom.quantity;
			//        if (index >= num_transitions)
			//            throw MapsException("ooops");
			//        auto bit_x = (x >> (num_transitions - 1 - index)) & 0x1;
			//        auto bit_y = (y >> (num_transitions - 1 - index)) & 0x1;
			//        return (SubTileLabel) (bit_x | (bit_y << 1));
			//            data.push_back(tile.getSubTileLabel(i));
		}

		void Path::pop() {
			data.pop_back();
		}

		void Path::push(ChildLabel child) {
			data.push_back(child);
		}

		void Path::reverse() {
			std::reverse(data.begin(),data.end());
		}

		bool Path::equalTo(const Path& path) const {
			if (path.length() != length())
				return false;
			for (std::size_t i=0;i<length();++i) {
				if (path[i] != (*this)[i])
					return false;
			}
			return true;
		}


		std::size_t Path::length() const {
			return data.size();
		}

		std::size_t Path::getLengthOfCommonPrefix(const Path& other) const {
			std::size_t result = 0;
			auto it1 = data.cbegin();
			auto it2 = other.data.cbegin();
			while (it1 != data.cend() && it2 != other.data.cend() && *it1 == *it2) {
				++result;
				++it1;
				++it2;
			}
			return result;
		}

		ChildLabel& Path::operator[](std::size_t index) {
			return data[index];
		}

		const ChildLabel& Path::operator[](std::size_t index) const {
			return data[index];
		}

		std::string Path::string_0123() const
		{
			std::stringstream ss;
			for (auto i: data) {
				ss << (char) ('0' + i);
			}
			return ss.str();
		}

		//-----------------------------------------------------------------------
		// Path IO
		//-----------------------------------------------------------------------

		std::ostream& operator<<(std::ostream& os, const Path& path) {
			os << "[";
			for (auto i: path.data) {
				os << " " << (int) i;
			}
			os << " ]";
			return os;
		}

		//-----------------------------------------------------------------------
		// CoverTreeEngine Impl.
		//-----------------------------------------------------------------------

		CoverTreeEngine::CoverTreeEngine():
			root(new Node()),
			current_node(root.get())
		{}


#define xLOG_COVER_TREE_ENGINE


		void CoverTreeEngine::goTo(const Path &path) {
			auto common_prefix_size = current_path.getLengthOfCommonPrefix(path);

#ifdef LOG_COVER_TREE_ENGINE
			std::cout << "   goto path: " << path << "  from: " << current_path << "  common_prefix_size: " << common_prefix_size << std::endl;
#endif

			auto i = current_path.length() - common_prefix_size;
			while (i > 0) {
				rewind();
				--i;
			}
			for (auto i=common_prefix_size;i<path.length();++i) {
				advance(path[i]);
			}
		}

		void CoverTreeEngine::rewind() {
			if (current_path.length() == 0)
				throw std::runtime_error("ooops");

			// when rewinding from a NOT_FIXED node release memory
			Node* parent       = current_node->parent;
			auto  child_label  = current_node->label_to_parent;

			if (current_node->tag.tag_type == Tag::NOT_FIXED) {
				if (parent) {
					parent->deleteChild(child_label);
				}
			}
			else if (current_node->tag.tag_type == Tag::FIXED) {
				current_node->deleteAllChildren();
			}

			if (parent && parent->getNumChildren() > 0) {
				parent->updateTag(Tag(Tag::FIXED_ANCESTOR,this->iteration_tag)); // if there is one fixed child
				// then consolidate parent
			}

			current_node = parent;
			current_path.pop();
		}


		void CoverTreeEngine::advance(ChildLabel child_label) {
			current_node = current_node->advance(child_label);
			current_path.push(child_label);
		}

		void CoverTreeEngine::swap(std::unique_ptr<Node>& other_root) {
			root.swap(other_root);
		}

		void CoverTreeEngine::consolidate() {

#ifdef LOG_COVER_TREE_ENGINE
			std::cout << "...consolidate " << current_path << std::endl;
#endif

			current_node->updateTag(Tag(Tag::FIXED, this->iteration_tag));
			current_node->deleteAllChildren();
		}

		namespace {

			// trim from start
			inline std::string &ltrim(std::string &s) {
				s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
				return s;
			}

			// trim from end
			inline std::string &rtrim(std::string &s) {
				s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
				return s;
			}

			// trim from both ends
			inline std::string &trim(std::string &s) {
				return ltrim(rtrim(s));
			}

		}

		//-----------------------------------------------------------------
		// Parser
		//-----------------------------------------------------------------

		void Parser::push(std::string st) {
			// check if st starts with a pound #
			if (st.length() == 0 || st.at(0) == '#')
				return;

			if (state == EMPTY) {
				this->description = st;
				state = WITH_DESCRIPTION;
			}
			else if (state == WITH_DESCRIPTION) {
				// parse code from string
				Node node;
				std::stringstream ss(st);
				ss >> node;
				signal.trigger(description, node);
				state = EMPTY;
				++trigger_count;
			}
		}

		bool Parser::run(std::istream& is, std::size_t max) {

			std::size_t base = this->trigger_count;

			Parser &parser = *this;

			tokenizer::Tokenizer lines(is, '\n', true);
			for (auto it=lines.begin();it!=lines.end();++it) {
				std::string line(trim(*it));
				parser.push(line);
				if (max != 0 && (trigger_count - base) == max) {
					return false;
				}
			}
			return true;
		}


		//-----------------------------------------------------------------
		// combine
		//-----------------------------------------------------------------

		LabeledTree combine(const LabeledTree& a, const LabeledTree& b, Operation op)
		{

			using ptr_t  = std::unique_ptr<Node>;
			using func_t = std::function<ptr_t(const Node*,const Node*,Operation)>;

			// used to refine a complete node while
			// we don't get to two leaves
			Node complete_node;

			enum ChildrenCase { CC_A_AND_B, CC_A_AND_COMPLETE_B, CC_COMPLETE_A_AND_B };

			//
			// give me the node that corresponds to combining
			// a and b using operator op
			//
			// note:
			// (1) a xor b can be null
			// (2) return can be a null ptr if it is not part of the combined tree
			//
			func_t process = [&](const Node* a, const Node* b, Operation op) -> ptr_t {

				ptr_t result;

				auto get_current_result_node = [&result]() -> Node& {
					if (!result)
						result.reset(new Node{});
					return *result.get();
				};

				auto process_children = [&](const Node *a, const Node* b, ChildrenCase ccase) {
					for (int i=0;i<4;++i) {

						auto aa = (ccase==CC_COMPLETE_A_AND_B) ? &complete_node :
							(a  ? a->children[i].get() : nullptr); // special node

						auto bb = (ccase==CC_A_AND_COMPLETE_B) ? &complete_node :
							(b ? b->children[i].get() : nullptr); // special node

						//                        std::cout << "complete_node" << &complete_node << std::endl;
						//                        std::cout << "a : " << a  << "   b : " << b << " a==b: " << (a==b) << std::endl;
						//                        std::cout << "aa: " << aa << "   bb: " << bb << " aa==bb: " << (aa==bb) << std::endl ;

						if (aa || bb) { // always true
							auto aux = process(aa, bb, op);
							if (aux) {
								auto &node = get_current_result_node();
								node.children[i].swap(aux);
							}
						}
					}
				};

				auto null_a = a ? 0 : 1;
				auto null_b = b ? 0 : 1;

				auto leaf_a = a ? (a->isLeaf()?1:0) : 0;
				auto leaf_b = b ? (b->isLeaf()?1:0) : 0;

				auto complete_a = a == &complete_node;
				auto complete_b = b == &complete_node;


				auto solve = [&]() {
					// covers the case of complete_a && complete_b (end real leaves and one of those)
					switch (op) {
					case A_SYMMDIFF_B:
						if ((null_a && !null_b) || (!null_a && null_b))
							result.reset(new Node());
						break;
					case A_AND_B:
						if (!null_a && !null_b)
							result.reset(new Node());
						break;
					case A_OR_B:
						if (!null_a || !null_b)
							result.reset(new Node());
						break;
					case A_MINUS_B:
						if (!null_a && null_b)
							result.reset(new Node());
						break;
					case B_MINUS_A:
						if (null_a && !null_b)
							result.reset(new Node());
						break;
					default:
						throw std::runtime_error("not implemented");
					}
				};


				if ( (complete_a || leaf_a || null_a) &&
				     (complete_b || leaf_b || null_b) ) {
					solve();
				}
				else if (complete_a || leaf_a) {
					process_children(a,b,CC_COMPLETE_A_AND_B);
				}
				else if (complete_b || leaf_b) {
					process_children(a,b,CC_A_AND_COMPLETE_B);
				}
				else /*if (null_a)*/ {
					process_children(a,b,CC_A_AND_B);
				}


				//                if ( (complete_a && complete_b) || (leaf_a && leaf_b) ) {
				//                    solve();
				//                }
				//                else if ( complete_a ) {
				//                    if ( leaf_b || null_b ) {
				//                        solve();
				//                    }
				//                    else {
				//                        process_children(a,b,CC_COMPLETE_A_AND_B);
				//                    }
				//                }
				//                else if ( complete_b ) {
				//                    if ( leaf_a || null_a ) {
				//                        solve();
				//                    }
				//                    else {
				//                        process_children(a,b,CC_A_AND_COMPLETE_B);
				//                    }
				//                }
				//                else if (leaf_a) {
				//                    if (null_b) {
				//                        solve();
				//                    }
				//                    else {
				//                        process_children(a,b,CC_COMPLETE_A_AND_B);
				//                    }
				//                }
				//                else if (leaf_b) {
				//                    if (null_a) {
				//                        solve();
				//                    }
				//                    else {
				//                        process_children(a,b,CC_A_AND_COMPLETE_B);
				//                    }
				//                }
				//                else if (null_a || null_b) {
				//                    solve();
				//                }
				//                else {
				//                    process_children(a,b,CC_A_AND_B);
				//                }
				return result;
			};

			// enum Operation { A_MINUS_B, B_MINUS_A, A_AND_B, A_OR_B, A_SYMMDIFF_B };

			if (!a.empty() && !b.empty()) {
				return LabeledTree { process(a.root(),b.root(),op) };
			}
			else if (a.empty() && b.empty()) {
				return LabeledTree {};
			}
			else if (a.empty()) {
				// one of the trees is empty
				if (op == A_AND_B || op == A_MINUS_B) {
					return LabeledTree {};
				}
				else {
					return LabeledTree { b };
				}
			}
			else { // if (b.empty()) {
				// one of the trees is empty
				if (op == A_AND_B || op == B_MINUS_A) {
					return LabeledTree {};
				}
				else {
					return LabeledTree { a };
				}
			}
			}

			LabeledTree operator+(const LabeledTree& a, const LabeledTree& b) {
				return combine(a,b,A_OR_B);
			}

			LabeledTree operator-(const LabeledTree& a, const LabeledTree& b) {
				return combine(a,b,A_MINUS_B);
			}

			LabeledTree operator*(const LabeledTree& a, const LabeledTree& b) {
				return combine(a,b,A_AND_B);
			}

		} // labeled_tree

	} // polycover




