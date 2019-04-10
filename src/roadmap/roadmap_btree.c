/*
 * rbt_ btree module
 */

/*
 *
 * Declarations
 *
 */

//
// A btree for variable sized keys. We store a
// hash of the key memory in the btree nodes.
// Collision might happen and are disambiguated
// by a linear scan.
//

//
// if a node $is_leaf$ field is set, then it has $e$ entries
// and all its $children$ pointers are null. If a node is not
// a leaf and has $e$ entries, than it also has $e+1$ children.
//
// hand made calculation:
// let s be the target node size
// let d be the min degree
// let f(d) = sizeof(u64) + (2*d - 1) * ( sizeof(rbt_Hash) + sizeof(Ptr_rbt_KeyValue) ) + 2*d * sizeof(Ptr_rbt_Node)
// min_degree = argmin_{d} ( [ f(d) <= s ] * f(d) )
//
// keep it fixed for now:
//
// note that MAX_DEGREE = 2 * MIN_DEGREE
//
// cases:   ORDER/MAX_DEGREE      MIN_DEGREE (= t)     MIN_ENTRIES   MAX_ENTRIES
//          3                     1   (= floor(1.5))   0             2   <--- degenerate case
//          4                     2   (= floor(2.0))   1             3
//          5                     2   (= floor(2.5))   1             4
//          6                     3   (= floor(3.0))   2             5
//          7                     3   (= floor(3.5))   2             6
//          8                     4   (= floor(4.0))   3             7
//
// 2 * max(floor(MAX_DEGREE / 2), 2)
//
//
// static const u64 rbt_Node_SIZE   = 4096;
// static const u64 rbt_u64_SIZE    = 8;
// static const u64 rbt_Hash_SIZE   = 8;
// static const u64 rbt_Ptr_SIZE    = 6;
// static const u64 rbt_MIN_DEGREE  = (rbt_NODE_SIZE - sizeof(u64) + sizeof(rbt_Hash) + sizeof(Ptr_rbt_KeyValue)) / (2 * (sizeof(rbt_Hash) + sizeof(Ptr_rbt_KeyValue) + sizeof(Ptr_rbt_Node)));
// static const u64 rbt_MIN_DEGREE  = (rbt_Node_SIZE - rbt_u64_SIZE + rbt_Hash_SIZE + rbt_Ptr_SIZE) / (2 * (rbt_Hash_SIZE + 2 * rbt_Ptr_SIZE));
// > (4096 - 8 + 8 + 6) / (2 * (8 + 6 + 6))
// [1] 102.55
//

// prefix rbt_ for btree related concepts

typedef struct rbt_KeyValue rbt_KeyValue;
typedef struct rbt_Node     rbt_Node;

PTR_SPECIALIZED_TYPE(rbt_Ptr_KeyValue);
PTR_SPECIALIZED_TYPE(rbt_Ptr_Node);
PTR_SPECIALIZED_TYPE(rbt_Ptr_BlockKV);

//     // the entries need to fit into a single page
//     using NumEntries  = uint64_t;
//     using NumChildren = uint64_t;
//     using ChildIndex  = uint64_t;
//     using EntryIndex  = uint64_t;

/* user knows what the content of this value represents */
struct rbt_KeyValue {
    u64             key;
    al_Ptr_char     value;
};

/* revise these sizes */
#define rbt_MIN_DEGREE  102
#define rbt_MAX_DEGREE  204 // rbt_MIN_DEGREE;
#define rbt_ORDER       204 // == rbt_MAX_DEGREE;
#define rbt_MIN_ENTRIES 101 // == rbt_MIN_DEGREE-1;
#define rbt_MAX_ENTRIES 203 // == rbt_MAX_DEGREE-1;

// make this the size of a page (eg. 4k)
struct rbt_Node {
    u64             num_entries:63;
    u64             is_leaf:1;
    rbt_KeyValue     data[rbt_MAX_ENTRIES]; // data[i] corresponds to hashes[i]
    rbt_Ptr_Node     children[rbt_MAX_DEGREE];
};

typedef struct {
    u64              size; // number of key value pairs
    rbt_Ptr_Node      root;
    al_Ptr_Allocator allocator; // kv memory is allocated through this
    al_Ptr_Cache     cache_nodes;
} rbt_BTree;

//
// if index == -1 and kv == 0, then it is the first
// time a node is seen should start iterating left
// most child, then entry, then next child, then entry...
//
typedef struct {
    rbt_Node     *node;
    u64          index:63;
    u64          first:1;
} rbt_IterItem;

#define rbt_ITER_STACK_CAPACITY 32

//
// iterate through btree in the order of
// the hashes
//
typedef struct {
    rbt_IterItem stack[rbt_ITER_STACK_CAPACITY];
    u64         stack_size;
} rbt_Iter;







/*
 *
 * Definitions
 *
 */

//------------------------------------------------------------------------------
// specialize some pointers
//------------------------------------------------------------------------------

PTR_SPECIALIZED_SERVICES(rbt_Ptr_KeyValue, rbt_KeyValue)
PTR_SPECIALIZED_SERVICES(rbt_Ptr_Node, rbt_Node)

//------------------------------------------------------------------------------
// specialize a rotate procedure
//------------------------------------------------------------------------------

static void
rbt_KeyValue_copy(rbt_KeyValue *self, rbt_KeyValue *other)
{
	self->key = other->key;
	al_Ptr_char_set(&self->value, al_Ptr_char_get(&other->value));
}

static void
rbt_KeyValue_swap(rbt_KeyValue *a, rbt_KeyValue *b)
{
	u64 k = a->key;
	a->key = b->key;
	b->key = k;
	char *v = al_Ptr_char_get(&a->value);
	al_Ptr_char_set(&a->value, al_Ptr_char_get(&b->value));
	al_Ptr_char_set(&b->value, v);
}

static void
rbt_rotate_key_values(rbt_KeyValue *begin, rbt_KeyValue *middle, rbt_KeyValue *end)
{
	rbt_KeyValue *next = middle;
	while (begin != next) {
		rbt_KeyValue_swap(begin,next);
		++begin;
		++next;
		if (next == end) {
			next = middle;
		} else if (begin == middle) {
			middle = next;
		}
	}
}


//------------------------------------------------------------------------------
// rbt_Node
//------------------------------------------------------------------------------

static void
rbt_Node_init(rbt_Node *self)
{
	// make sure everything is zeroed
	pt_fill((char*) self, (char*) self + sizeof(rbt_Node), 0);
	self->num_entries = 0;
	self->is_leaf     = 1;
}

static rbt_Node*
rbt_Node_child(rbt_Node *self, u64 index)
{
	rbt_Node *child = rbt_Ptr_Node_get(&self->children[index]);
	return child;
}

static rbt_KeyValue*
rbt_Node_key_value(rbt_Node *self, u64 index)
{
	Assert(index < self->num_entries);
	return self->data + index;
}

static u64
rbt_Node_key(rbt_Node *self, u64 index)
{
	Assert(index < self->num_entries);
	return (self->data + index)->key;
}

static b8
rbt_Node_full(rbt_Node* self)
{
	return self->num_entries == rbt_MAX_ENTRIES;
}


//
// returns non-negative number corresponding
// to an existing entry with the same hash
//
// return -1-(insertion index) if key not found
// eg.
//     -1 if new entry should be inserted at position index 0
//     -2 if new entry should be inserted at position index 1
//     ...
//     -k if new entry should be inserted at position index k-1
//
static s64
rbt_Node_key_insert_index(rbt_Node* self, u64 key)
{
	// [l,r)
	rbt_KeyValue *l = self->data;
	rbt_KeyValue *r = self->data + self->num_entries;

	if (l == r) return -1;

	/* make sure l and r are always changing */
	while (r - l > 2) {
		/* m and m+1 are both different from l and r */
		rbt_KeyValue *m = l + (r-l)/2;
		if (key <= m->key) {
			r = m+1;
		} else {
			l = m;
		}
	}

	if (r - l == 2) {
		if (key < l->key) {
			return -1 - (l - self->data);
		} else if (key == l->key) {
			return l - self->data;
		} else if (key < (l + 1)->key) {
			return -1 - (l + 1 - self->data);
		} else if (key == (l + 1)->key) {
			return l + 1 - self->data;
		} else {
			return -1 - (r - self->data);
		}
	} else { /* r - l == 1 */
		if (key < l->key) {
			return -1 - (l - self->data);
		} else if (key == l->key) {
			return l - self->data;
		} else {
			return -1 - (r - self->data);
		}
	}
}

static rbt_KeyValue*
rbt_Node_find(rbt_Node* self, u64 key)
{
	s64 index = rbt_Node_key_insert_index(self, key);
	if (index >= 0) {
		return self->data + index;
	} else if (!self->is_leaf) {
		index = -index - 1;
		return rbt_Node_find(rbt_Node_child(self, index), key);
	} else {
		return 0;
	}
}

//------------------------------------------------------------------------------
// rbt_BTree
//------------------------------------------------------------------------------

static u16 rbt_g_btree_index = 0;

static void
rbt_BTree_init(rbt_BTree *self, al_Allocator *allocator)
{
	/* @NOTE thread unsafe */
	u16 rbt_id = rbt_g_btree_index++;

	self->size = 0;
	rbt_Ptr_Node_set_null(&self->root);

	char  name[al_MAX_CACHE_NAME_LENGTH+1];
	Print print;
	print_init(&print, name, al_MAX_CACHE_NAME_LENGTH);

	{  // prepare name of cach
		print_clear(&print);
		print_cstr(&print, "rbt_Node_");
		print_u64(&print,(u64) rbt_id);
		print_char(&print, 0);
		name[al_MAX_CACHE_NAME_LENGTH] = 0;
	}

	al_Ptr_Cache_set(&self->cache_nodes, al_Allocator_create_cache(allocator, name, sizeof(rbt_Node)));

}

static void
rbt_BTree_split_child(rbt_BTree* self, rbt_Node *node, u64 index)
{
	// assumes node is not full
	Assert(!rbt_Node_full(node));

	rbt_Node *child = rbt_Node_child(node, index);

	Assert(rbt_Node_full(child));

	rbt_Node *new_child = (rbt_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
	rbt_Node_init(new_child);

	//         // break child into
	//         //
	//         // 1-based
	//         // [1,2,...,MIN_ENTRIES] [MIN_ENTRY+1] [MIN_ENTRY+2,...,MAX_ENTRIES]
	//         //
	//         // 0-based
	//         // [0,2,...,MIN_ENTRIES-1] [MIN_ENTRY] [MIN_ENTRY+1,...,MAX_ENTRIES-1]
	//         //
	//         // cases:   ORDER/MAX_DEGREE      MIN_DEGREE (= t)            MIN_ENTRIES   MAX_ENTRIES
	//         //          4                     2   (= max(floor(2.0),2))   1             3
	//         //          6                     3   (= max(floor(3.0),2))   2             5
	//         //          8                     4   (= max(floor(4.0),2))   3             7
	//         //

	// append median to (non-full) parent node (will rotate later)
	rbt_KeyValue_copy(node->data + node->num_entries, child->data + rbt_MIN_ENTRIES);

	// copy new node as last child (will rotate later)
	rbt_Ptr_Node_set(node->children + node->num_entries + 1, new_child);

	// copy second half of child keys into new child
	for (u64 j=0;j<rbt_MIN_ENTRIES;++j) {
		rbt_KeyValue_copy(new_child->data + j, child->data + rbt_MIN_ENTRIES + 1 + j);
	}
	if (!child->is_leaf) {
		for (u64 j=0;j<rbt_MIN_DEGREE;++j) {
			rbt_Ptr_Node_copy(new_child->children + j, child->children + rbt_MIN_ENTRIES + 1 + j);
		}
	}

	child->num_entries     = rbt_MIN_ENTRIES;
	new_child->num_entries = rbt_MIN_ENTRIES;
	new_child->is_leaf     = child->is_leaf;


	if (index < node->num_entries) {
		// rotate hashes
		rbt_rotate_key_values(node->data + index,
				     node->data + node->num_entries,
				     node->data + node->num_entries + 1);

		// rotate children
		Ptr* begin = (Ptr*) node->children;
		pt_rotate_Ptr(begin + index + 1, begin + node->num_entries + 1, begin + node->num_entries + 2);
	}

	++node->num_entries;

}

static void
rbt_BTree_insert_nonfull(rbt_BTree *self, rbt_Node* node, u64 key, char *value)
{
	Assert(!rbt_Node_full(node));

	s64 index = rbt_Node_key_insert_index(node, key);

	if (index >= 0) {
		/* overwrite */
		rbt_KeyValue *kv = rbt_Node_key_value(node,index);
		al_Ptr_char_set(&kv->value, value);
	} else if (node->is_leaf) {

		rbt_KeyValue *kv = node->data + node->num_entries;
		kv->key = key;
		al_Ptr_char_set(&kv->value, value);

		// convert to insertion index rotations
		index = -index - 1;
		rbt_rotate_key_values(node->data + index,
				     node->data + node->num_entries,
				     node->data + node->num_entries + 1);
		++node->num_entries;
		++self->size;
	} else {
		index = -index - 1;
		rbt_Node *child = rbt_Node_child(node, index);
		if (rbt_Node_full(child)) {
			rbt_BTree_split_child(self, node, index);
			// compare
			if (rbt_Node_key(node, index) < key) {
				++index;
			}
		}
		rbt_BTree_insert_nonfull(self, rbt_Node_child(node,index), key, value);
	}
}

/* @TODO(llins): return error code with failure types */
static void
rbt_BTree_insert(rbt_BTree* self, u64 key, char *value)
{
	rbt_Node *root = rbt_Ptr_Node_get(&self->root);

	if (!root) {
		root = (rbt_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
		rbt_Node_init(root);
		rbt_Ptr_Node_set(&self->root, root);
	} else if (rbt_Node_full(root)) {
		// special case if the root is full
		rbt_Node *new_root = (rbt_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
		rbt_Node_init(new_root);
		new_root->is_leaf = 0;
		rbt_Ptr_Node_set(new_root->children, root);
		rbt_Ptr_Node_set(&self->root, new_root);
		root = new_root;
		rbt_BTree_split_child(self, root, 0);
	}

	rbt_BTree_insert_nonfull(self, root, key, value);
}

static b8
rbt_BTree_get_value(rbt_BTree *self, u64 key, char* *value)
{
	rbt_Node *root = rbt_Ptr_Node_get(&self->root);
	if (!root) {
		return 0;
	}

	rbt_KeyValue *kv = rbt_Node_find(root, key);
	if (!kv) {
		return 0;
	} else {
		// search through linked list
		*value = al_Ptr_char_get(&kv->value);
		return 1;
	}
}


//------------------------------------------------------------------------------
// rbt_Iter
//------------------------------------------------------------------------------

static void
rbt_Iter_init(rbt_Iter *self, rbt_BTree *tree)
{
	rbt_Node* node = rbt_Ptr_Node_get(&tree->root);
	if (node) {
		self->stack_size = 1;
		rbt_IterItem *item = &self->stack[0];
		item->node  =  node;
		item->index =  0;
		item->first =  1;
	} else {
		self->stack_size = 0;
	}
}

static void
rbt_Iter_push_child(rbt_Iter *self, rbt_Node *node, u32 index)
{
	Assert(self->stack_size < rbt_ITER_STACK_CAPACITY);
	rbt_Node *child = rbt_Node_child(node, index);
	Assert(child);
	rbt_IterItem *next_item = &self->stack[self->stack_size];
	next_item->node  = child;
	next_item->first =  1;
	next_item->index =  0;
	++self->stack_size;
}


static b8
rbt_Iter_next(rbt_Iter *self, u64 *output_key, char* *output_value)
{
	if (self->stack_size == 0) return 0;
	while (self->stack_size > 0) {
		rbt_IterItem *item = &self->stack[self->stack_size-1];
		if (item->first) {
			/* first time arriving at node */
			item->first = 0;
			item->index = 0; // go to first entry next time we pop
			if (!item->node->is_leaf) {
				rbt_Iter_push_child(self, item->node, 0);
			}
		} else if (item->index < item->node->num_entries) {
			rbt_KeyValue *kv = rbt_Node_key_value(item->node, item->index);
			*output_key   = kv->key;
			*output_value = al_Ptr_char_get(&kv->value);
			++item->index;
			if (!item->node->is_leaf) {
				rbt_Iter_push_child(self, item->node, item->index);
			}
			return 1;

		} else {
			--self->stack_size;
		}
	}
	return 0;
}
