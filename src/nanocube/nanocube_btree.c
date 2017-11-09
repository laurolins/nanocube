/*
 * bt_ btree module
 */

#ifdef nanocube_btree_UNIT_TEST
#include "../base/platform.c"
#include "../base/alloc.c"
#endif

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
// let f(d) = sizeof(u64) + (2*d - 1) * ( sizeof(bt_Hash) + sizeof(Ptr_bt_KeyValue) ) + 2*d * sizeof(Ptr_bt_Node)
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
// static const u64 bt_Node_SIZE   = 4096;
// static const u64 bt_u64_SIZE    = 8;
// static const u64 bt_Hash_SIZE   = 8;
// static const u64 bt_Ptr_SIZE    = 6;
// static const u64 bt_MIN_DEGREE  = (bt_NODE_SIZE - sizeof(u64) + sizeof(bt_Hash) + sizeof(Ptr_bt_KeyValue)) / (2 * (sizeof(bt_Hash) + sizeof(Ptr_bt_KeyValue) + sizeof(Ptr_bt_Node)));
// static const u64 bt_MIN_DEGREE  = (bt_Node_SIZE - bt_u64_SIZE + bt_Hash_SIZE + bt_Ptr_SIZE) / (2 * (bt_Hash_SIZE + 2 * bt_Ptr_SIZE));
// > (4096 - 8 + 8 + 6) / (2 * (8 + 6 + 6))
// [1] 102.55
//

// prefix bt_ for btree related concepts

// key hash
typedef u64 bt_Hash;

typedef struct bt_KeyValue bt_KeyValue;
typedef struct bt_Node     bt_Node;

PTR_SPECIALIZED_TYPE(bt_Ptr_KeyValue);
PTR_SPECIALIZED_TYPE(bt_Ptr_Node);
PTR_SPECIALIZED_TYPE(bt_Ptr_BlockKV);

//     // the entries need to fit into a single page
//     using NumEntries  = uint64_t;
//     using NumChildren = uint64_t;
//     using ChildIndex  = uint64_t;
//     using EntryIndex  = uint64_t;

struct bt_KeyValue {
    bt_Ptr_KeyValue next; // singly linked list of key values with the same key hash
    al_Ptr_char     key;
    al_Ptr_char     value;
    u32             key_size;   // up to 4GB keys
    u32             value_size; // up to 4GB values
};

#define bt_MIN_DEGREE  102
#define bt_MAX_DEGREE  204 // bt_MIN_DEGREE;
#define bt_ORDER       204 // == bt_MAX_DEGREE;
#define bt_MIN_ENTRIES 101 // == bt_MIN_DEGREE-1;
#define bt_MAX_ENTRIES 203 // == bt_MAX_DEGREE-1;

// make this the size of a page (eg. 4k)
struct bt_Node {
    u64             num_entries:63;
    u64             is_leaf:1;
    bt_Hash         hashes[bt_MAX_ENTRIES];
    bt_Ptr_KeyValue data[bt_MAX_ENTRIES]; // data[i] corresponds to hashes[i]
    bt_Ptr_Node     children[bt_MAX_DEGREE];
};

//
// key value blocks should grow by al_PAGE multiples
// and double every time we need an extra block
// it starts with one page or the min number pages
// to fit the initial record and grow from there
//
typedef struct {
    al_Ptr_char    begin; // where the data for this block starts
    bt_Ptr_BlockKV next;  //
    u64            bytes_used;
    u64            capacity;
} bt_BlockKV;

typedef struct {
    bt_Hash     hash;
    MemoryBlock key;
    MemoryBlock value;
    struct {
        u32 header;
        u32 header_aligned;
        u32 key;
        u32 key_aligned;
        u32 value;
        u32 value_aligned;
        u32 total_aligned;
    } sizes;
} bt_Record;

typedef struct {
    u64              size; // number of key value pairs
    u64              num_hashes; // if no collision, size==num_hashes
    bt_Ptr_Node      root;
    al_Ptr_Allocator allocator; // kv memory is allocated through this
    al_Ptr_Cache     cache_nodes;
    al_Ptr_Cache     cache_blocks_kv; // key value blocks
    bt_Ptr_BlockKV   head_block_kv;
} bt_BTree;

//
// if index == -1 and kv == 0, then it is the first
// time a node is seen should start iterating left
// most child, then entry, then next child, then entry...
//
typedef struct {
    bt_Node     *node;
    u64          index:63; // next child to process
    u64          node_index:8; // this node's index in its parent node (0 for root)
    u64          depth:7;
    u64          first:1;
    bt_KeyValue *kv;
} bt_IterItem;

#define bt_ITER_STACK_CAPACITY 32

//
// iterate through btree in the order of
// the hashes
//
typedef struct {
    bt_IterItem stack[bt_ITER_STACK_CAPACITY];
    u64         stack_size;
} bt_Iter;







/*
 *
 * Definitions
 *
 */

//------------------------------------------------------------------------------
// specialize some pointers
//------------------------------------------------------------------------------

PTR_SPECIALIZED_SERVICES(bt_Ptr_KeyValue, bt_KeyValue)
PTR_SPECIALIZED_SERVICES(bt_Ptr_Node, bt_Node)
PTR_SPECIALIZED_SERVICES(bt_Ptr_BlockKV, bt_BlockKV)

//------------------------------------------------------------------------------
// specialize a rotate procedure
//------------------------------------------------------------------------------

ROTATE_SPECIALIZED(bt_rotate_hashes, bt_Hash)

//------------------------------------------------------------------------------
//
// multiply rotate xor multiply rotate
// https://sites.google.com/site/murmurhash/MurmurHash2_64.cpp?attredirects=0
//
//------------------------------------------------------------------------------

#define bt_MURMUR_SEED 0x5dee7792

internal u64
bt_murmur_hash2_64A(const void *key, s32 len, s32 seed)
{
	u64  m = 0xc6a4a7935bd1e995;
	s32  r = 47;

	u64 h = seed ^ (len * m);

	const u64 *data = (u64*) key;
	const u64 *end  = data + (len/8);

	while(data != end)
	{
		u64 k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const u8* data2 = (const u8*) data;

	switch(len & 7)
	{
	case 7: h ^= (u64) data2[6] << 48;
	case 6: h ^= (u64) data2[5] << 40;
	case 5: h ^= (u64) data2[4] << 32;
	case 4: h ^= (u64) data2[3] << 24;
	case 3: h ^= (u64) data2[2] << 16;
	case 2: h ^= (u64) data2[1] << 8;
	case 1: h ^= (u64) data2[0];
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}

//------------------------------------------------------------------------------
// bt_Node
//------------------------------------------------------------------------------

internal void
bt_Node_init(bt_Node *self)
{
	// make sure everything is zeroed
	pt_fill((char*) self, (char*) self + sizeof(bt_Node), 0);
	self->num_entries = 0;
	self->is_leaf     = 1;
}

internal bt_Node*
bt_Node_child(bt_Node *self, u64 index)
{
	bt_Node *child = bt_Ptr_Node_get(&self->children[index]);
	return child;
}

internal bt_KeyValue*
bt_Node_key_value(bt_Node *self, u64 index)
{
	Assert(index < self->num_entries);
	return bt_Ptr_KeyValue_get(self->data + index);
}

internal bt_Hash
bt_Node_hash(bt_Node *self, u64 index)
{
	Assert(index < self->num_entries);
	return *(self->hashes + index);
}

internal b8
bt_Node_full(bt_Node* self)
{
	return self->num_entries == bt_MAX_ENTRIES;
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
internal s64
bt_Node_hash_insert_index(bt_Node* self, bt_Hash hash)
{
	Assert(self->num_entries <= bt_MAX_ENTRIES);

	// [l,r)
	bt_Hash *l = self->hashes;
	bt_Hash *r = self->hashes + self->num_entries;

	if (l == r) return -1;

	/* make sure l and r are always changing */
	while (r - l > 2) {
		/* m and m+1 are both different from l and r */
		bt_Hash *m = l + (r-l)/2;
		if (hash <= *m) {
			r = m+1;
		} else {
			l = m;
		}
	}

	if (r - l == 2) {
		if (hash < *l) {
			return -1 - (l - self->hashes);
		} else if (hash == *l) {
			return l - self->hashes;
		} else if (hash < *(l + 1)) {
			return -1 - (l + 1 - self->hashes);
		} else if (hash == *(l + 1)) {
			return (l + 1) - self->hashes; // this without parenthesis is weird
		} else {
			return -1 - (r - self->hashes);
		}
	} else { /* r - l == 1 */
		if (hash < *l) {
			return -1 - (l - self->hashes);
		} else if (hash == *l) {
			return l - self->hashes;
		} else {
			return -1 - (r - self->hashes);
		}
	}
}

internal bt_KeyValue*
bt_Node_find_hash(bt_Node* self, bt_Hash hash)
{
	s64 index = bt_Node_hash_insert_index(self, hash);
	if (index >= 0) {
		return bt_Node_key_value(self, pt_safe_s64_u64(index));
	} else if (!self->is_leaf) {
		index = -index - 1;
		return bt_Node_find_hash(bt_Node_child(self, index), hash);
	} else {
		return 0;
	}
}

//------------------------------------------------------------------------------
// bt_KeyValue
//------------------------------------------------------------------------------

internal MemoryBlock
bt_KeyValue_key(bt_KeyValue* self)
{
	MemoryBlock result;
	result.begin = al_Ptr_char_get(&self->key);
	result.end   = result.begin + self->key_size;
	return result;
}

internal MemoryBlock
bt_KeyValue_value(bt_KeyValue* self)
{
	MemoryBlock result;
	result.begin = al_Ptr_char_get(&self->value);
	result.end   = result.begin + self->value_size;
	return result;
}

internal bt_KeyValue*
bt_KeyValue_next(bt_KeyValue* self)
{
	return bt_Ptr_KeyValue_get(&self->next);
}

//------------------------------------------------------------------------------
// bt_BlockKV
//------------------------------------------------------------------------------

internal void
bt_BlockKV_init(bt_BlockKV *self, char *begin, u64 capacity)
{
	al_Ptr_char_set(&self->begin, begin);
	self->bytes_used = 0;
	self->capacity = capacity;
	bt_Ptr_BlockKV_set_null(&self->next);
}

//------------------------------------------------------------------------------
// bt_Record
//------------------------------------------------------------------------------

internal void
bt_Record_init(bt_Record *self, char *key_begin, char *key_end, char *value_begin, char *value_end)
{
	self->hash = bt_murmur_hash2_64A(key_begin, (s32) (key_end-key_begin), bt_MURMUR_SEED);
	self->key.begin = key_begin;
	self->key.end   = key_end;
	self->value.begin = value_begin;
	self->value.end   = value_end;
	self->sizes.header           = (u32) sizeof(bt_KeyValue);
	self->sizes.header_aligned   = (u32) pt_next_multiple(self->sizes.header,8);
	self->sizes.key              = pt_safe_s64_u32(key_end - key_begin);
	self->sizes.key_aligned      = (u32)pt_next_multiple(self->sizes.key,8);
	self->sizes.value            = pt_safe_s64_u32(value_end - value_begin);
	self->sizes.value_aligned    = (u32)pt_next_multiple(self->sizes.value,8);
	self->sizes.total_aligned    = self->sizes.header_aligned +
		self->sizes.key_aligned + self->sizes.value_aligned;
}

//------------------------------------------------------------------------------
// bt_BlockKV
//------------------------------------------------------------------------------


internal inline u64
bt_BlockKV_bytes_available(bt_BlockKV *self)
{
	return self->capacity - self->bytes_used;
}

internal inline MemoryBlock
bt_BlockKV_reserve(bt_BlockKV *self, u64 size)
{
	Assert(self->bytes_used + size <= self->capacity);
	MemoryBlock result;
	result.begin = al_Ptr_char_get(&self->begin) + self->bytes_used;
	result.end   = result.begin + size;
	self->bytes_used += size;
	return result;
}


//------------------------------------------------------------------------------
// bt_Iter
//------------------------------------------------------------------------------

internal void
bt_Iter_init(bt_Iter *self, bt_BTree *tree)
{
	bt_Node* node = bt_Ptr_Node_get(&tree->root);
	if (node) {
		self->stack_size = 1;
		bt_IterItem *item = &self->stack[0];
		item->node  =  node;
		item->depth =  0;
		item->node_index = 0;
		item->index =  0;
		item->first =  1;
		item->kv    =  0;
	} else {
		self->stack_size = 0;
	}
}

internal bt_Node*
bt_Iter_next_node(bt_Iter *self, u32 *out_index, u32 *out_depth)
{
	if (self->stack_size == 0) return 0;
	while (self->stack_size > 0) {
		bt_IterItem *item = &self->stack[self->stack_size-1];
		if (item->first) {
			/* first time arriving at node */
			item->first = 0;
			item->index = 0; // go to first entry next time we pop
			if (!item->node->is_leaf) {
				Assert(self->stack_size < bt_ITER_STACK_CAPACITY);
				bt_Node *child = bt_Node_child(item->node, 0);
				Assert(child);
				bt_IterItem *next_item = &self->stack[self->stack_size];
				next_item->node  = child;
				next_item->depth = item->depth + 1;
				next_item->node_index = 0;
				next_item->first = 1;
				next_item->index = 0;
				next_item->kv    = 0;
				++self->stack_size;
			}
			if (out_depth) {
				*out_depth = item->depth;
				*out_index = item->node_index;
			}
			return item->node;
		} else if (item->index < item->node->num_entries) {
			++item->index;
			item->kv = 0;
			if (!item->node->is_leaf) {
				Assert(self->stack_size < bt_ITER_STACK_CAPACITY);
				bt_Node *child = bt_Node_child(item->node, item->index);
				Assert(child);
				bt_IterItem *next_item = &self->stack[self->stack_size];
				next_item->node  = child;
				next_item->depth = item->depth + 1;
				next_item->node_index = item->index;
				next_item->index = 0;
				next_item->first = 1;
				next_item->kv    = 0;
				++self->stack_size;
			}
		} else {
			--self->stack_size;
		}
	}
	return 0;
}

internal b8
bt_Iter_next(bt_Iter *self, bt_Hash *hash, MemoryBlock *key, MemoryBlock *value)
{
	if (self->stack_size == 0) return 0;
	while (self->stack_size > 0) {
		bt_IterItem *item = &self->stack[self->stack_size-1];
		if (item->first) {
			/* first time arriving at node */
			item->first = 0;
			item->index = 0; // go to first entry next time we pop
			if (!item->node->is_leaf) {
				Assert(self->stack_size < bt_ITER_STACK_CAPACITY);
				bt_Node *child = bt_Node_child(item->node, 0);
				Assert(child);
				bt_IterItem *next_item = &self->stack[self->stack_size];
				next_item->node  = child;
				next_item->depth = item->depth + 1;
				next_item->node_index = 0;
				next_item->first =  1;
				next_item->index =  0;
				next_item->kv    =  0;
				++self->stack_size;
			}
		} else if (item->index < item->node->num_entries) {
			if (!item->kv) {
				/* go to next kv */
				item->kv = bt_Node_key_value(item->node,item->index);
				Assert(item->kv); // should exist
				*hash  = bt_Node_hash(item->node,item->index);
				*key   = bt_KeyValue_key(item->kv);
				*value = bt_KeyValue_value(item->kv);
				// the fact that we advanced kv means we are
				// in position for the next call
				return 1;
			} else {
				bt_KeyValue *next_kv = bt_KeyValue_next(item->kv);
				if (next_kv) {
					item->kv = next_kv;
					*hash  = bt_Node_hash(item->node,item->index);
					*key   = bt_KeyValue_key(item->kv);
					*value = bt_KeyValue_value(item->kv);
					return 1;
				} else {
					++item->index;
					item->kv = 0;
					if (!item->node->is_leaf) {
						// note that the child index here also includes
						// the num_entries index. In other words we
						// stack all the num_entries+1 children of a
						// non leaf node. The first child is stacked
						// at first hit and then everytime we finish
						// scanning the kv pairs at entry item->index
						// we stack the next child
						Assert(self->stack_size < bt_ITER_STACK_CAPACITY);
						bt_Node *child = bt_Node_child(item->node, item->index);
						Assert(child);
						bt_IterItem *next_item = &self->stack[self->stack_size];
						next_item->node  = child;
						next_item->depth = item->depth + 1;
						next_item->index = 0;
						next_item->node_index = item->index;
						next_item->first = 1;
						next_item->kv    = 0;
						++self->stack_size;
					}
				}
			}
		} else {
			--self->stack_size;
		}
	}
	return 0;
}


//------------------------------------------------------------------------------
// bt_BTree
//------------------------------------------------------------------------------

static u16 bt_g_btree_index = 0;

internal void
bt_BTree_init(bt_BTree *self, al_Allocator *allocator)
{
//     u64              size; // number of key value pairs
//     u64              num_hashes; // if no collision, size==num_hashes
//     bt_Ptr_Node      root;
//     al_Ptr_Allocator allocator; // kv memory is allocated through this
//     al_Ptr_Cache     cache_nodes;     // cache nodes
//     al_Ptr_Cache     cache_blocks_kv; // initialize cache
//     bt_Ptr_BlockKV   head_block_kv;   // initialize as null

	pt_fill((char*) self, (char*) self + sizeof(bt_BTree), 0);

	/* @NOTE thread unsafe */
	u16 bt_id = bt_g_btree_index++;

	self->size       = 0;
	self->num_hashes = 0;
	bt_Ptr_Node_set_null(&self->root);

	char  name[al_MAX_CACHE_NAME_LENGTH+1];
	Print print;
	Print_init(&print, name, name + al_MAX_CACHE_NAME_LENGTH);

	{  // prepare name of cache
		Print_clear(&print);
		Print_cstr(&print, "bt_Node_");
		Print_u64(&print,(u64) bt_id);
		Print_char(&print, 0);
		name[al_MAX_CACHE_NAME_LENGTH] = 0;
	}
	al_Cache *cache_nodes = al_Allocator_create_cache(allocator, name, sizeof(bt_Node));
	al_Ptr_Cache_set(&self->cache_nodes, cache_nodes);

	{  // prepare name of cache
		Print_clear(&print);
		Print_cstr(&print, "bt_BlockKV_");
		Print_u64(&print,(u64) bt_id);
		Print_char(&print, 0);
		name[al_MAX_CACHE_NAME_LENGTH] = 0;
	}
	al_Cache *cache_blocks_kv = al_Allocator_create_cache(allocator, name, sizeof(bt_BlockKV));
	al_Ptr_Cache_set(&self->cache_blocks_kv, cache_blocks_kv);

	bt_Ptr_BlockKV_set_null(&self->head_block_kv);

	al_Ptr_Allocator_set(&self->allocator, allocator);
}

internal bt_KeyValue*
bt_prepare_keyvalue_from_record(char *base, bt_Record *record)
{
	// @PRE
	// - memory from base to base + record->sizes.total_aligned is reserved
	char *ptr  = base;

	bt_KeyValue *key_value = (bt_KeyValue*) ptr;
	bt_Ptr_KeyValue_set_null(&key_value->next);
	key_value->key_size   = record->sizes.key;
	key_value->value_size = record->sizes.value;
	ptr += record->sizes.header_aligned;

	al_Ptr_char_set(&key_value->key, ptr);
	pt_copy_bytes(record->key.begin, record->key.end, ptr, ptr + record->sizes.key);
	pt_fill(ptr + record->sizes.key, ptr + record->sizes.key_aligned, 0);
	ptr += record->sizes.key_aligned;

	al_Ptr_char_set(&key_value->value, ptr);
	pt_copy_bytes(record->value.begin, record->value.end, ptr, ptr + record->sizes.value);
	pt_fill(ptr + record->sizes.value, ptr + record->sizes.value_aligned, 0);
	// ptr += value_size_aligned;

	return key_value;
}

internal bt_BlockKV*
bt_BTree_new_block(bt_BTree *self, u64 min_capacity)
{
	u32 new_block_pages = (u32)  pt_next_multiple(min_capacity, al_PAGE_SIZE)/al_PAGE_SIZE;

	al_Allocator *allocator = al_Ptr_Allocator_get(&self->allocator);
	al_PageBlock page_block = al_Allocator_back_reserve(allocator, new_block_pages);

	Assert(page_block.page_size>0);

	char *base = (char*) al_Allocator_page_to_ptr(allocator, page_block.page_index);

	// init head block
	al_Cache *cache_blocks_kv = al_Ptr_Cache_get(&self->cache_blocks_kv);
	bt_BlockKV *block_kv = al_Cache_alloc(cache_blocks_kv);

	block_kv->capacity   = al_PAGE_SIZE * new_block_pages;
	block_kv->bytes_used = 0;
	al_Ptr_char_set(&block_kv->begin, base);
	bt_Ptr_BlockKV_set_null(&block_kv->next);

	return block_kv;
}


internal bt_KeyValue*
bt_BTree_new_key_value(bt_BTree *self, bt_Record* record)
{
	bt_BlockKV *head = bt_Ptr_BlockKV_get(&self->head_block_kv);

	if (!head) {
		// first block
		bt_BlockKV *new_block = bt_BTree_new_block(self,record->sizes.total_aligned);

		// set head_block_kv
		bt_Ptr_BlockKV_set(&self->head_block_kv, new_block);
		MemoryBlock mem = bt_BlockKV_reserve(new_block, record->sizes.total_aligned);
		return bt_prepare_keyvalue_from_record(mem.begin, record);
	} else if (record->sizes.total_aligned <= bt_BlockKV_bytes_available(head)) {
		MemoryBlock mem = bt_BlockKV_reserve(head, record->sizes.total_aligned);
		return bt_prepare_keyvalue_from_record(mem.begin, record);
	} else {
		// head exists but new block will be needed
		// create new block with double capacity or enough to fit record.
		u64 next_block_size = 2 * head->capacity;
		if (next_block_size < record->sizes.total_aligned) {
			next_block_size = record->sizes.total_aligned;
		}

		bt_BlockKV *new_block = bt_BTree_new_block(self,record->sizes.total_aligned);
		bt_Ptr_BlockKV_set(&new_block->next, head);
		bt_Ptr_BlockKV_set(&self->head_block_kv, new_block);
		MemoryBlock mem = bt_BlockKV_reserve(new_block, record->sizes.total_aligned);
		return bt_prepare_keyvalue_from_record(mem.begin, record);
	}
}

internal void
bt_BTree_split_child(bt_BTree* self, bt_Node *node, u64 index)
{
	// assumes node is not full
	Assert(!bt_Node_full(node));

	bt_Node *child = bt_Node_child(node, index);

	Assert(bt_Node_full(child));

	bt_Node *new_child = (bt_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
	bt_Node_init(new_child);

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

	// append median to parent node (will rotate later)
	node->hashes[node->num_entries] = child->hashes[bt_MIN_ENTRIES];
	bt_Ptr_KeyValue_copy(node->data + node->num_entries, child->data + bt_MIN_ENTRIES);

	// copy new node as last child (will rotate later)
	bt_Ptr_Node_set(node->children + node->num_entries + 1, new_child);

	// copy second half of child keys into new child
	for (u64 j=0;j<bt_MIN_ENTRIES;++j) {
		new_child->hashes[j] = child->hashes[bt_MIN_ENTRIES + 1 + j];
	}
	for (u64 j=0;j<bt_MIN_ENTRIES;++j) {
		bt_Ptr_KeyValue_copy(new_child->data + j, child->data + bt_MIN_ENTRIES + 1 + j);
	}
	if (!child->is_leaf) {
		for (u64 j=0;j<bt_MIN_DEGREE;++j) {
			bt_Ptr_Node_copy(new_child->children + j, child->children + bt_MIN_ENTRIES + 1 + j);
		}
	}

	child->num_entries     = bt_MIN_ENTRIES;
	new_child->num_entries = bt_MIN_ENTRIES;
	new_child->is_leaf     = child->is_leaf;


	if (index < node->num_entries) {
		// rotate hashes
		bt_rotate_hashes(node->hashes + index, node->hashes + node->num_entries,
				 node->hashes + node->num_entries + 1);

		// rotate keyvalue pointers
		Ptr *begin;

		begin = (Ptr*) node->data;
		pt_rotate_Ptr(begin + index, begin + node->num_entries, begin + node->num_entries + 1);

		// rotate children
		begin = (Ptr*) node->children;
		pt_rotate_Ptr(begin + index + 1, begin + node->num_entries + 1, begin + node->num_entries + 2);
	}

	++node->num_entries;

}

internal void
bt_BTree_insert_nonfull(bt_BTree *self, bt_Node* node, bt_Record *record)
{
	Assert(!bt_Node_full(node));

	s64 index = bt_Node_hash_insert_index(node, record->hash);

	if (index >= 0) {
		bt_KeyValue *existing_kv = bt_Node_key_value(node, index);
		bt_KeyValue *new_kv = bt_BTree_new_key_value(self, record);
		bt_KeyValue *existing_next_kv = bt_Ptr_KeyValue_get(&existing_kv->next);
		bt_Ptr_KeyValue_set(&existing_kv->next, new_kv);
		bt_Ptr_KeyValue_set(&new_kv->next, existing_next_kv);
		++self->size;
	} else if (node->is_leaf) {
		node->hashes[node->num_entries] = record->hash;
		// create new key value region and point to that region
		bt_KeyValue *key_value = bt_BTree_new_key_value(self, record);
		bt_Ptr_KeyValue_set(node->data + node->num_entries, key_value);

		// convert to insertion index rotations
		index = -index - 1;

		bt_rotate_hashes(node->hashes + index,
				 node->hashes + node->num_entries,
				 node->hashes + node->num_entries + 1);

		Ptr* begin = (Ptr*) node->data;
		pt_rotate_Ptr(begin + index,
			      begin + node->num_entries,
			      begin + node->num_entries + 1);

		++node->num_entries;

		++self->size;
		++self->num_hashes;
	} else {
		index = -index - 1;
		bt_Node *child = bt_Node_child(node, index);
		if (bt_Node_full(child)) {
			bt_BTree_split_child(self, node, index);
			// compare
			if (bt_Node_hash(node, index) < record->hash) {
				++index;
			}
		}
		bt_BTree_insert_nonfull(self, bt_Node_child(node,index), record);
	}
}

internal void
bt_BTree_check(bt_BTree *self)
{
	bt_Iter it;
	bt_Iter_init(&it, self);

	bt_Node *node = 0;
	u32 index = 0;
	u32 depth = 0;
	for (;;) {
		node = bt_Iter_next_node(&it, &index, &depth);
		if (!node) {
			break;
		}
		// check consistency of node
		Assert(node->num_entries <= bt_MAX_ENTRIES);
	}
}

#if 0
internal void
bt_BTree_print(bt_BTree *self)
{
	bt_Iter it;
	bt_Iter_init(&it, self);

	printf("-----------------------------------------------------------------------------------------\n");

	u64 sum_entries = 0;
	bt_Node *node = 0;
	u32 index = 0;
	u32 depth = 0;
	for (;;) {
		node = bt_Iter_next_node(&it, &index, &depth);
		if (!node) {
			break;
		}
		for (s32 i=0;i<depth;++i) {
			printf("    ");
		}
		printf("[%03u] node: %20p   leaf: %d   num_entries: %lu\n", index, node, (s32) node->is_leaf, (u64) node->num_entries);
		sum_entries += node->num_entries;
		// check consistency of node
		Assert(node->num_entries <= bt_MAX_ENTRIES);
	}

	printf("^^^^^ btree: %19p   #kv: %lu   num_hashes: %lu  sum(node->num_entries): %lu\n", self, self->size, self->num_hashes, sum_entries);
}
#endif

internal void
bt_BTree_insert(bt_BTree* self, char *key_begin, char *key_end, char *value_begin, char *value_end)
{
#if 0
	{
		static s32 open = 0;
		static FILE *file;
		if (!open) {
			file = fopen("/tmp/btree.txt","w");
			open = 1;
		}
		char tab = '\t';
		char newline = '\n';
		fwrite(key_begin, 1, key_end - key_begin, file);
		fwrite(&tab, 1, 1, file);
		fwrite(value_begin, 1, value_end - value_begin, file);
		fwrite(&newline, 1, 1, file);
		fflush(file);
		// printf("insert key size %ld value size %ld\n", key_end - key_begin, value_end - value_begin);
	}
#endif

#if 0
	// bt_BTree_print(self);
	bt_BTree_check(self);
#endif


	bt_Node *root = bt_Ptr_Node_get(&self->root);

	if (!root) {
		root = (bt_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
		bt_Node_init(root);
		bt_Ptr_Node_set(&self->root, root);
	} else if (bt_Node_full(root)) {
		// special case if the root is full
		bt_Node *new_root = (bt_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
		bt_Node_init(new_root);
		new_root->is_leaf = 0;
		bt_Ptr_Node_set(new_root->children, root);
		bt_Ptr_Node_set(&self->root, new_root);
		root = new_root;
		bt_BTree_split_child(self, root, 0);
	}

	bt_Record record;
	bt_Record_init(&record, key_begin, key_end, value_begin, value_end);
	bt_BTree_insert_nonfull(self, root, &record);


#if 0
	bt_BTree_check(self);
	// bt_BTree_print(self);
	// bt_BTree_check(self);
#endif

}

internal b8
bt_BTree_get_value(bt_BTree *self, char *key_begin, char *key_end, MemoryBlock *output)
{
	output->begin = 0;
	output->end = 0;

	bt_Node *root = bt_Ptr_Node_get(&self->root);
	if (!root) {
		return 0;
	}

	bt_Hash hash = bt_murmur_hash2_64A(key_begin, (s32) (key_end-key_begin), bt_MURMUR_SEED);

	bt_KeyValue *kv = bt_Node_find_hash(root, hash);

	if (!kv) {
		return 0;
	} else {
		// search through linked list
		bt_KeyValue *it = kv;
		while (it) {
			MemoryBlock kv_key = bt_KeyValue_key(it);
			if (pt_compare_memory(key_begin, key_end, kv_key.begin, kv_key.end) == 0) {
				*output = bt_KeyValue_value(it);
				return 1;
			} else {
				it = bt_Ptr_KeyValue_get(&it->next);
			}
		}
		return 0;
	}
}






#ifdef nanocube_btree_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	u64 size = Megabytes(64);
	char *storage = (char*) malloc(size+4095);
	char *buffer  = (char*) (RALIGN( (u64) storage, 4096 ));
	al_Allocator *allocator = al_Allocator_new(buffer, buffer + size);

	bt_BTree btree;
	bt_BTree_init(&btree, allocator);

	char filename[] = "/home/llins/pproj/CMCF_20170101_20170331_K1/bug/bug2.txt";
	u64 file_size = 0;
	FILE *fp = fopen(filename, "r");
	if (fp) {
		fseek(fp, 0L, SEEK_END);
		file_size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
	}

	char *text = (char*) malloc(file_size);
	fread(text, 1, file_size, fp);
	fclose(fp);

	// parse lines and insert them into the btree
	char *it = text;
	char *end = text + file_size;

	// find new line
	while (it < end) {
		// TODO(llins): make it more robust to support \r\n
		char *stop1 = pt_find_char(it,      end, '\t');
		char *stop2 = pt_find_char(stop1+1, end, '\n');

		bt_BTree_insert(&btree, it, stop1-1, stop1+1, stop2);
		it = stop2 + 1;
	}
}

#endif




