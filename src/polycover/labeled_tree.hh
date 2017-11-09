#pragma once

#include <memory>
#include <vector>
#include <iostream>

#include "signal.hh"

#include "maps.hh"

namespace polycover {

	//
	// Labels are numbers from 0 to 255
	//

	//-----------------------------------------------------------------------------
	// tree
	//-----------------------------------------------------------------------------

	namespace labeled_tree {

		using ChildLabel = uint8_t;

		using Code = std::string;

		const uint8_t NONE = 255;

		struct Summary;

		struct Path;

		//-----------------------------------------------------------------------
		// Tag
		//-----------------------------------------------------------------------

		struct Tag {
		public:
			enum Type { NOT_FIXED, FIXED, FIXED_ANCESTOR };
		public:
			Tag() = default;
			Tag(Type type, int iteration);
			void update(const Tag& tag);
		public:
			Type tag_type { NOT_FIXED };
			int  iteration { 0 };
		};

		std::ostream &operator<<(std::ostream& os, const Tag::Type& ttype);
		std::ostream &operator<<(std::ostream& os, const Tag& tag);

		//-----------------------------------------------------------------------
		// Node
		//-----------------------------------------------------------------------

		struct Node {
		public:
			Node() = default;
			~Node();

			Node(Node* parent, ChildLabel label_to_parent);

			Node* advance(ChildLabel child_label);

			Path path() const;

			void deleteChild(ChildLabel child_label);

			void deleteAllChildren();

			void trim(int layers_to_go);

			bool split();

			int getNumChildren() const;

			int optimize();

			int pack();

			Summary getSummary() const;

			bool isFixed() const;

			bool isLeaf() const;

			void updateTag(Tag tag); // make is a fixed node

		public:
			Node*                 parent            { nullptr   };
			ChildLabel            label_to_parent   { NONE      };
			Tag                   tag;
			std::unique_ptr<Node> children[4];
		};

		//-----------------------------------------------------------------------
		// TileCoverage
		//-----------------------------------------------------------------------

		struct TileCoverage {
			TileCoverage() = default;
			TileCoverage(int tilex, int tiley, int level, float coverage);
			int tilex;
			int tiley;
			int level;
			float coverage;
		};

		//-----------------------------------------------------------------------
		// LabeledTree
		//-----------------------------------------------------------------------

		struct LabeledTree {
		public:
			using node_type = Node;
		public:
			LabeledTree() = default; // empty labeled tree

			LabeledTree(const Code& code);

			LabeledTree(int x, int y, int level);

			LabeledTree(std::unique_ptr<Node>&& root);

			LabeledTree(const LabeledTree& other);

			LabeledTree& operator=(const LabeledTree& other);

			LabeledTree operator-() const;

			Code code() const;

			void clear();

			void swap(LabeledTree& other);

			bool empty() const;

			Node* root();

			const Node* root() const;

			void optimize();

			void pack(); // TODO: revise what is the difference from optimize to pack

			std::vector<TileCoverage> coverage_report(int level);

		public:
			std::unique_ptr<Node> _root;
		};


		//-----------------------------------------------------------------------
		// free functions
		//-----------------------------------------------------------------------

		enum Operation { A_MINUS_B, B_MINUS_A, A_AND_B, A_OR_B, A_SYMMDIFF_B };

		LabeledTree combine(const LabeledTree& a, const LabeledTree& b, Operation op);

		LabeledTree operator+(const LabeledTree& a, const LabeledTree& b); // or

		LabeledTree operator-(const LabeledTree& a, const LabeledTree& b); // minus

		LabeledTree operator*(const LabeledTree& a, const LabeledTree& b); // and

		// combine list
		template <typename Iter>
			LabeledTree combine(Iter begin,
					    Iter end,
					    std::function<LabeledTree(Iter it)> extract,
					    Operation op);


		//-----------------------------------------------------------------------
		// Summary
		//-----------------------------------------------------------------------

		struct Summary {
			Summary(const Node& tree);

			std::size_t getNumLevels() const;
			std::size_t getNumLeaves() const;
			std::size_t getNumNodes()  const;

			std::size_t getFirstLevelWithTwoOrMoreNodes() const;

			std::size_t num_nodes { 0 };
			std::vector<std::size_t> nodes_per_level;
			std::vector<std::size_t> nodes_per_num_children;
		};


		std::ostream& operator<<(std::ostream& os, const Summary& summary);


		//-----------------------------------------------------------------------
		// Node
		//-----------------------------------------------------------------------

		using Depth = int;

		struct Iterator {
		public:
			struct Action {
			public:
				enum Type { PUSH, POP };
			public:
				Action();

				Action(const Node* parent,
				       const Node* child);

				Action(const Node* parent,
				       const Node* child,
				       ChildLabel  label,
				       Depth       depth);
			public:
				Type         type;
				const Node* parent { nullptr };
				const Node* child  { nullptr };
				ChildLabel  label  { NONE    };
				Depth       depth  { 0       };
			};

		public:
			Iterator(const Node& node);
			Action *next(); // repeat until null
		public:
			Action last_action;
			std::vector<Action> stack;
		};


		//-----------------------------------------------------------------------
		// Write down the code of a tree
		//-----------------------------------------------------------------------

		std::ostream& operator<<(std::ostream& os, const Node& root);


		std::ostream& json(std::ostream& os, const Node& root);
		std::ostream& text(std::ostream& os, const Node& root);


		//-----------------------------------------------------------------------
		// Path
		//-----------------------------------------------------------------------

		struct Path {
			Path() = default;
			Path(const maps::Tile& tile);
			Path(const std::vector<ChildLabel> &labels);

			void setTile(const maps::Tile& tile);

			std::size_t length() const;
			void pop();
			void push(ChildLabel child);

			ChildLabel& operator[](std::size_t index);
			const ChildLabel& operator[](std::size_t index) const;

			void reverse();
			bool equalTo(const Path& path) const;

			std::string string_0123() const;

		public:
			std::size_t getLengthOfCommonPrefix(const Path& other) const;
			std::vector<ChildLabel> data;
		};


		std::ostream& operator<<(std::ostream& os, const Path& path);

		//-----------------------------------------------------------------------
		// CoverTreeEngine
		//-----------------------------------------------------------------------

		struct CoverTreeEngine {
		public:
			CoverTreeEngine();
			void goTo(const Path &path);
			void advance(ChildLabel child);
			void rewind();
			void consolidate();
			void swap(std::unique_ptr<Node>& other_root);
		public:
			int                   iteration_tag { 0 }; // when consolidating and rewinding use this tag
			std::unique_ptr<Node> root;
			Node*                 current_node { nullptr };
			Path                  current_path;
		};

		//-----------------------------------------------------------------
		// Parser
		//-----------------------------------------------------------------

		struct Parser { // .ttt files (tile tree tiles)

			enum State { EMPTY, WITH_DESCRIPTION };

			// format:
			//     (
			//     code_string_description <line_feed>
			//     code_string <line_feed>
			//     )*

			// true if stream is finished, false if "max" interruption occurred
			bool run(std::istream& is, std::size_t max);

			void push(std::string st);   // start a new area

			State       state { EMPTY };
			std::string description;

			std::size_t trigger_count { 0 };

			sig::Signal<const std::string&, const labeled_tree::Node&> signal;
		};






		//
		//
		// Implementation
		//
		//











		//
		// Utility function to combine trees using the same operator
		// (keeps merging pairs in order until one remains)
		//

		//------------------------------------------------------------------------------
		// Aux. Function to Combine Cell Decompositions from a list
		//------------------------------------------------------------------------------

		template <typename Iter>
			LabeledTree combine(Iter begin, Iter end, std::function<LabeledTree(Iter it)> extract, Operation op) {

				auto n = end - begin;

				std::vector<LabeledTree> nodes;
				for (auto i=0;i<n;i+=2) {
					LabeledTree cover1 { extract(begin + i) };
					if (i+1 < n) {
						LabeledTree cover2 { extract(begin + i + 1) };
						nodes.push_back(combine(cover1, cover2, op));
					}
					else {
						nodes.push_back(std::move(cover1));
					}
				}

				// keep combining covers until one is remaining
				while (nodes.size() > 1) {
					n = nodes.size();
					for (auto i=0;i<n;i+=2) {
						if (i+1 < n) {
							nodes[i/2] = combine(nodes[i], nodes[i+1], op);
						}
						else {
							nodes[i/2] = std::move(nodes[i]);
						}
					}
					auto unused_index = n/2 + ((n % 2 == 0) ? 0 : 1);
					nodes.erase(nodes.begin()+unused_index, nodes.end());
				}

				LabeledTree result;

				if (nodes.size())
					result.swap(nodes.back());

				return std::move(result);

			}


	} // labeled_tree

} // polycover
