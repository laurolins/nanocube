/*
 * ss_ btree module
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
// let f(d) = sizeof(u64) + (2*d - 1) * ( sizeof(ss_Hash) + sizeof(Ptr_ss_Entry) ) + 2*d * sizeof(Ptr_ss_Node)
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
// static const u64 ss_Node_SIZE   = 4096;
// static const u64 ss_u64_SIZE    = 8;
// static const u64 ss_Hash_SIZE   = 8;
// static const u64 ss_Ptr_SIZE    = 6;
// static const u64 ss_MIN_DEGREE  = (ss_NODE_SIZE - sizeof(u64) + sizeof(ss_Hash) + sizeof(Ptr_ss_Entry)) / (2 * (sizeof(ss_Hash) + sizeof(Ptr_ss_Entry) + sizeof(Ptr_ss_Node)));
// static const u64 ss_MIN_DEGREE  = (ss_Node_SIZE - ss_u64_SIZE + ss_Hash_SIZE + ss_Ptr_SIZE) / (2 * (ss_Hash_SIZE + 2 * ss_Ptr_SIZE));
// > (4096 - 8 + 8 + 6) / (2 * (8 + 6 + 6))
// [1] 102.55
//

// prefix ss_ for btree related concepts

// key hash
typedef u64 ss_Hash;

typedef struct ss_Entry   ss_Entry;
typedef struct ss_Node    ss_Node;
typedef struct ss_String  ss_String;

PTR_SPECIALIZED_TYPE(ss_Ptr_Entry);
PTR_SPECIALIZED_TYPE(ss_Ptr_Node);
PTR_SPECIALIZED_TYPE(ss_Ptr_String);
PTR_SPECIALIZED_TYPE(ss_Ptr_EntryBlock);

//     // the entries need to fit into a single page
//     using NumEntries  = uint64_t;
//     using NumChildren = uint64_t;
//     using ChildIndex  = uint64_t;
//     using EntryIndex  = uint64_t;

struct ss_String {
	u32  length;
	/* the space is reserved from [begin, begin + length) */
	char begin;
};

struct ss_Entry {
    ss_Ptr_Entry  next; // singly linked list of key values with the same key hash
    ss_String     string;
};

#define ss_MIN_DEGREE  102
#define ss_MAX_DEGREE  204 // ss_MIN_DEGREE;
#define ss_ORDER       204 // == ss_MAX_DEGREE;
#define ss_MIN_ENTRIES 101 // == ss_MIN_DEGREE-1;
#define ss_MAX_ENTRIES 203 // == ss_MAX_DEGREE-1;

// make this the size of a page (eg. 4k)
struct ss_Node {
    u64             num_entries:63;
    u64             is_leaf:1;
    ss_Hash         hashes[ss_MAX_ENTRIES];
    ss_Ptr_Entry    data[ss_MAX_ENTRIES];     // data[i] corresponds to hashes[i]
    ss_Ptr_Node     children[ss_MAX_DEGREE];
};

//
// key value blocks should grow by al_PAGE multiples
// and double every time we need an extra block
// it starts with one page or the min number pages
// to fit the initial record and grow from there
//
typedef struct {
    al_Ptr_char       begin; // where the data for this block starts
    ss_Ptr_EntryBlock next;  //
    u64               bytes_used;
    u64               capacity;
} ss_EntryBlock;

typedef struct {
    ss_Hash     hash;
    MemoryBlock key;
    u32         buffer_size;
} ss_Record;

typedef struct {
    u64                 size; // number of key value pairs
    u64                 num_hashes; // if no collision, size==num_hashes
    ss_Ptr_Node         root;
    al_Ptr_Allocator    allocator; // kv memory is allocated through this
    al_Ptr_Cache        cache_nodes;
    al_Ptr_Cache        cache_entry_blocks; // key value blocks
    ss_Ptr_EntryBlock   head_entry_block;
} ss_StringSet;

//
// if index == -1 and kv == 0, then it is the first
// time a node is seen should start iterating left
// most child, then entry, then next child, then entry...
//
typedef struct {
    ss_Node     *node;
    u64          index:63;
    u64          first:1;
    ss_Entry *kv;
} ss_IterItem;

#define ss_ITER_STACK_CAPACITY 32


//
// iterate through btree in the order of
// the hashes
//
typedef struct {
    ss_IterItem stack[ss_ITER_STACK_CAPACITY];
    u64         stack_size;
} ss_Iter;


/*
 *
 * Definitions
 *
 */

//------------------------------------------------------------------------------
// specialize some pointers
//------------------------------------------------------------------------------

PTR_SPECIALIZED_SERVICES(ss_Ptr_Entry, ss_Entry)
PTR_SPECIALIZED_SERVICES(ss_Ptr_Node, ss_Node)
PTR_SPECIALIZED_SERVICES(ss_Ptr_EntryBlock, ss_EntryBlock)

//------------------------------------------------------------------------------
// specialize a rotate procedure
//------------------------------------------------------------------------------

ROTATE_SPECIALIZED(ss_rotate_hashes, ss_Hash)

//------------------------------------------------------------------------------
//
// multiply rotate xor multiply rotate
// https://sites.google.com/site/murmurhash/MurmurHash2_64.cpp?attredirects=0
//
//------------------------------------------------------------------------------

#define ss_MURMUR_SEED 0x5dee7792

internal u64
ss_murmur_hash2_64A(const void *key, s32 len, s32 seed)
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
// ss_Node
//------------------------------------------------------------------------------

internal void
ss_Node_init(ss_Node *self)
{
	// make sure everything is zeroed
	pt_fill((char*) self, (char*) self + sizeof(ss_Node), 0);
	self->num_entries = 0;
	self->is_leaf     = 1;
}

internal ss_Node*
ss_Node_child(ss_Node *self, u64 index)
{
	ss_Node *child = ss_Ptr_Node_get(&self->children[index]);
	return child;
}

internal ss_Entry*
ss_Node_get_entry(ss_Node *self, u64 index)
{
	Assert(index < self->num_entries);
	return ss_Ptr_Entry_get(self->data + index);
}

internal void
ss_Node_push_entry(ss_Node *self, u64 index, ss_Entry *new)
{
	Assert(index < self->num_entries);
	ss_Entry *old = ss_Ptr_Entry_get(self->data + index);
	ss_Ptr_Entry_set(&new->next, old);
	ss_Ptr_Entry_set(self->data + index, new);
}

internal ss_Hash
ss_Node_hash(ss_Node *self, u64 index)
{
	Assert(index < self->num_entries);
	return *(self->hashes + index);
}

internal b8
ss_Node_full(ss_Node* self)
{
	return self->num_entries == ss_MAX_ENTRIES;
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
ss_Node_hash_insert_index(ss_Node* self, ss_Hash hash)
{
	// [l,r)
	ss_Hash *l = self->hashes;
	ss_Hash *r = self->hashes + self->num_entries;

	if (l == r) return -1;

	/* make sure l and r are always changing */
	while (r - l > 2) {
		/* m and m+1 are both different from l and r */
		ss_Hash *m = l + (r-l)/2;
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
			return l + 1 - self->hashes;
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

internal ss_Entry*
ss_Node_find_hash(ss_Node* self, ss_Hash hash)
{
	s64 index = ss_Node_hash_insert_index(self, hash);
	if (index >= 0) {
		return ss_Node_get_entry(self, pt_safe_s64_u64(index));
	} else if (!self->is_leaf) {
		index = -index - 1;
		return ss_Node_find_hash(ss_Node_child(self, index), hash);
	} else {
		return 0;
	}
}

//------------------------------------------------------------------------------
// ss_Entry
//------------------------------------------------------------------------------

internal ss_String*
ss_Entry_string(ss_Entry* self)
{
	return &self->string;
}

internal ss_Entry*
ss_Entry_next(ss_Entry* self)
{
	return ss_Ptr_Entry_get(&self->next);
}

//------------------------------------------------------------------------------
// ss_EntryBlock
//------------------------------------------------------------------------------

internal void
ss_EntryBlock_init(ss_EntryBlock *self, char *begin, u64 capacity)
{
	al_Ptr_char_set(&self->begin, begin);
	self->bytes_used = 0;
	self->capacity = capacity;
	ss_Ptr_EntryBlock_set_null(&self->next);
}

//------------------------------------------------------------------------------
// ss_Record
//------------------------------------------------------------------------------

internal void
ss_Record_init(ss_Record *self, char *key_begin, char *key_end)
{
	self->hash = ss_murmur_hash2_64A(key_begin, (s32) (key_end-key_begin), ss_MURMUR_SEED);
	self->key.begin              = key_begin;
	self->key.end                = key_end;
	//
	// [ss_Ptr_Entry next, u32 length, char begin]
	// + [ enough to fit key content in an 8 byte aligned manner ]
	//
	s64 buffer_length = sizeof(ss_Entry) + (key_end - key_begin) - 1;
	self->buffer_size = (u32) pt_next_multiple(buffer_length,8);
}

//------------------------------------------------------------------------------
// ss_EntryBlock
//------------------------------------------------------------------------------


internal inline u64
ss_EntryBlock_bytes_available(ss_EntryBlock *self)
{
	return self->capacity - self->bytes_used;
}

internal inline MemoryBlock
ss_EntryBlock_reserve(ss_EntryBlock *self, u64 size)
{
	Assert(self->bytes_used + size <= self->capacity);
	MemoryBlock result;
	result.begin = al_Ptr_char_get(&self->begin) + self->bytes_used;
	result.end   = result.begin + size;
	self->bytes_used += size;
	return result;
}

//------------------------------------------------------------------------------
// ss_StringSet
//------------------------------------------------------------------------------

static u16 ss_g_btree_index = 0;

internal void
ss_StringSet_init(ss_StringSet *self, al_Allocator *allocator)
{
//     u64              size; // number of key value pairs
//     u64              num_hashes; // if no collision, size==num_hashes
//     ss_Ptr_Node      root;
//     al_Ptr_Allocator allocator; // kv memory is allocated through this
//     al_Ptr_Cache     cache_nodes;     // cache nodes
//     al_Ptr_Cache     cache_entry_blocks; // initialize cache
//     ss_Ptr_EntryBlock   head_entry_block;   // initialize as null

	pt_fill((char*) self, (char*) self + sizeof(ss_StringSet), 0);

	/* @NOTE thread unsafe */
	u16 ss_id = ss_g_btree_index++;

	self->size       = 0;
	self->num_hashes = 0;
	ss_Ptr_Node_set_null(&self->root);

	char  name[al_MAX_CACHE_NAME_LENGTH+1];
	Print print;
	Print_init(&print, name, name + al_MAX_CACHE_NAME_LENGTH);

	{  // prepare name of cache
		Print_clear(&print);
		Print_cstr(&print, "ss_Node_");
		Print_u64(&print,(u64) ss_id);
		Print_char(&print, 0);
		name[al_MAX_CACHE_NAME_LENGTH] = 0;
	}
	al_Cache *cache_nodes = al_Allocator_create_cache(allocator, name, sizeof(ss_Node));
	al_Ptr_Cache_set(&self->cache_nodes, cache_nodes);

	{  // prepare name of cache
		Print_clear(&print);
		Print_cstr(&print, "ss_EntryBlock_");
		Print_u64(&print,(u64) ss_id);
		Print_char(&print, 0);
		name[al_MAX_CACHE_NAME_LENGTH] = 0;
	}
	al_Cache *cache_entry_blocks = al_Allocator_create_cache(allocator, name, sizeof(ss_EntryBlock));
	al_Ptr_Cache_set(&self->cache_entry_blocks, cache_entry_blocks);

	ss_Ptr_EntryBlock_set_null(&self->head_entry_block);

	al_Ptr_Allocator_set(&self->allocator, allocator);
}

internal ss_Entry*
ss_prepare_entry_from_record(char *base, ss_Record *record)
{
	// struct ss_String {
	// 	u32  length;
	// 	/* the space is reserved from [begin, begin + length) */
	// 	char text_begin;
	// };
	//
	// struct ss_Entry {
	//     ss_Ptr_Entry  next; // singly linked list of key values with the same key hash
	//     ss_String     string;
	// };
	// @PRE
	// - memory from base to base + record->sizes.total_aligned is reserved
	char *ptr  = base;
	pt_fill(ptr, ptr + record->buffer_size, '\0');
	ss_Entry *entry = (ss_Entry*) ptr;
	ss_Ptr_Entry_set_null(&entry->next);
	entry->string.length = (u32) (record->key.end - record->key.begin);
	ptr = &entry->string.begin;
	pt_copy_bytesn(record->key.begin, ptr, record->key.end - record->key.begin);
	return entry;
}

internal ss_EntryBlock*
ss_StringSet_new_block(ss_StringSet *self, u64 min_capacity)
{
	u32 new_block_pages = (u32)  pt_next_multiple(min_capacity, al_PAGE_SIZE)/al_PAGE_SIZE;

	al_Allocator *allocator = al_Ptr_Allocator_get(&self->allocator);
	al_PageBlock page_block = al_Allocator_back_reserve(allocator, new_block_pages);
	char *base = (char*) al_Allocator_page_to_ptr(allocator, page_block.page_index);

	// init head block
	al_Cache *cache_entry_blocks = al_Ptr_Cache_get(&self->cache_entry_blocks);
	ss_EntryBlock *entry_block = al_Cache_alloc(cache_entry_blocks);

	entry_block->capacity   = al_PAGE_SIZE * new_block_pages;
	entry_block->bytes_used = 0;
	al_Ptr_char_set(&entry_block->begin, base);
	ss_Ptr_EntryBlock_set_null(&entry_block->next);

	return entry_block;
}


internal ss_Entry*
ss_StringSet_new_entry(ss_StringSet *self, ss_Record* record)
{
	ss_EntryBlock *head = ss_Ptr_EntryBlock_get(&self->head_entry_block);
	if (!head) {
		// first block
		ss_EntryBlock *new_block = ss_StringSet_new_block(self,record->buffer_size);
		MemoryBlock mem = ss_EntryBlock_reserve(new_block, record->buffer_size);
		// set first block as head_entry_block
		ss_Ptr_EntryBlock_set(&self->head_entry_block, new_block);
		return ss_prepare_entry_from_record(mem.begin, record);
	} else if (record->buffer_size <= ss_EntryBlock_bytes_available(head)) {
		MemoryBlock mem = ss_EntryBlock_reserve(head, record->buffer_size);
		return ss_prepare_entry_from_record(mem.begin, record);
	} else {
		// head exists but new block will be needed
		// create new block with double capacity or enough to fit record.
		u64 next_block_size = 2 * head->capacity;
		if (next_block_size < record->buffer_size)
			next_block_size = record->buffer_size;
		ss_EntryBlock *new_block = ss_StringSet_new_block(self,next_block_size);
		ss_Ptr_EntryBlock_set(&new_block->next, head);
		ss_Ptr_EntryBlock_set(&self->head_entry_block, new_block);
		MemoryBlock mem = ss_EntryBlock_reserve(new_block, record->buffer_size);
		return ss_prepare_entry_from_record(mem.begin, record);
	}
}

internal void
ss_StringSet_split_child(ss_StringSet* self, ss_Node *node, u64 index)
{
	// assumes node is not full
	Assert(!ss_Node_full(node));

	ss_Node *child = ss_Node_child(node, index);

	Assert(ss_Node_full(child));

	ss_Node *new_child = (ss_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
	ss_Node_init(new_child);

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
	node->hashes[node->num_entries] = child->hashes[ss_MIN_ENTRIES];
	ss_Ptr_Entry_copy(node->data + node->num_entries, child->data + ss_MIN_ENTRIES);

	// copy new node as last child (will rotate later)
	ss_Ptr_Node_set(node->children + node->num_entries + 1, new_child);

	// copy second half of child keys into new child
	for (u64 j=0;j<ss_MIN_ENTRIES;++j) {
		new_child->hashes[j] = child->hashes[ss_MIN_ENTRIES + 1 + j];
	}
	for (u64 j=0;j<ss_MIN_ENTRIES;++j) {
		ss_Ptr_Entry_copy(new_child->data + j, child->data + ss_MIN_ENTRIES + 1 + j);
	}
	if (!child->is_leaf) {
		for (u64 j=0;j<ss_MIN_DEGREE;++j) {
			ss_Ptr_Node_copy(new_child->children + j, child->children + ss_MIN_ENTRIES + 1 + j);
		}
	}

	child->num_entries     = ss_MIN_ENTRIES;
	new_child->num_entries = ss_MIN_ENTRIES;
	new_child->is_leaf     = child->is_leaf;


	if (index < node->num_entries) {
		// rotate hashes
		ss_rotate_hashes(node->hashes + index, node->hashes + node->num_entries,
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

internal ss_String*
ss_StringSet_insert_nonfull(ss_StringSet *self, ss_Node* node, ss_Record *record)
{
	Assert(!ss_Node_full(node));
	s64 index = ss_Node_hash_insert_index(node, record->hash);
	if (index >= 0) {
		ss_Entry *entry = ss_StringSet_new_entry(self, record);
		ss_Node_push_entry(node, index, entry);
		++self->size;
		return &entry->string;
	} else if (node->is_leaf) {
		node->hashes[node->num_entries] = record->hash;
		// create new key value region and point to that region
		ss_Entry *entry = ss_StringSet_new_entry(self, record);
		ss_Ptr_Entry_set(node->data + node->num_entries, entry);
		// convert to insertion index rotations
		index = -index - 1;
		ss_rotate_hashes(node->hashes + index, node->hashes + node->num_entries, node->hashes + node->num_entries + 1);
		Ptr* begin = (Ptr*) node->data;
		pt_rotate_Ptr(begin + index, begin + node->num_entries, begin + node->num_entries + 1);
		++node->num_entries;
		++self->size;
		++self->num_hashes;
		return &entry->string;
	} else {
		index = -index - 1;
		ss_Node *child = ss_Node_child(node, index);
		if (ss_Node_full(child)) {
			ss_StringSet_split_child(self, node, index);
			// compare
			if (ss_Node_hash(node, index) < record->hash) {
				++index;
			}
		}
		return ss_StringSet_insert_nonfull(self, ss_Node_child(node,index), record);
	}
}

/* @TODO(llins): return error code with failure types */
internal ss_String*
ss_StringSet_insert(ss_StringSet* self, char *key_begin, char *key_end)
{
	ss_Node *root = ss_Ptr_Node_get(&self->root);

	if (!root) {
		root = (ss_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
		ss_Node_init(root);
		ss_Ptr_Node_set(&self->root, root);
	} else if (ss_Node_full(root)) {
		// special case if the root is full
		ss_Node *new_root = (ss_Node*) al_Cache_alloc(al_Ptr_Cache_get(&self->cache_nodes));
		ss_Node_init(new_root);
		new_root->is_leaf = 0;
		ss_Ptr_Node_set(new_root->children, root);
		ss_Ptr_Node_set(&self->root, new_root);
		root = new_root;
		ss_StringSet_split_child(self, root, 0);
	}

	ss_Record record;
	ss_Record_init(&record, key_begin, key_end);
	return ss_StringSet_insert_nonfull(self, root, &record);
}

internal ss_String*
ss_StringSet_get(ss_StringSet *self, char *key_begin, char *key_end)
{
	ss_Node *root = ss_Ptr_Node_get(&self->root);
	if (!root) {
		return 0;
	}
	ss_Hash hash = ss_murmur_hash2_64A(key_begin, (s32) (key_end-key_begin), ss_MURMUR_SEED);
	ss_Entry *kv = ss_Node_find_hash(root, hash);
	if (!kv) {
		return 0;
	} else {
		// search through linked list
		ss_Entry *it = kv;
		while (it) {
			ss_String *it_str = ss_Entry_string(it);
			if (pt_compare_memory(key_begin, key_end, &it_str->begin, &it_str->begin + it_str->length) == 0) {
				return it_str;
			} else {
				it = ss_Ptr_Entry_get(&it->next);
			}
		}
		return 0;
	}
}

#if 0

//------------------------------------------------------------------------------
// ss_Iter
//------------------------------------------------------------------------------

internal void
ss_Iter_init(ss_Iter *self, ss_StringSet *tree)
{
	ss_Node* node = ss_Ptr_Node_get(&tree->root);
	if (node) {
		self->stack_size = 1;
		ss_IterItem *item = &self->stack[0];
		item->node  =  node;
		item->index =  0;
		item->first =  1;
		item->kv    =  0;
	} else {
		self->stack_size = 0;
	}
}

internal b8
ss_Iter_next(ss_Iter *self, ss_Hash *hash, MemoryBlock *key, MemoryBlock *value)
{
	if (self->stack_size == 0) return 0;

	while (self->stack_size > 0)
	{
		ss_IterItem *item = &self->stack[self->stack_size-1];

		if (item->first) {
			/* first time arriving at node */
			item->first = 0;
			item->index = 0; // go to first entry next time we pop
			if (!item->node->is_leaf) {
				Assert(self->stack_size < ss_ITER_STACK_CAPACITY);
				ss_Node *child = ss_Node_child(item->node, 0);
				Assert(child);
				ss_IterItem *next_item = &self->stack[self->stack_size];
				next_item->node  = child;
				next_item->first =  1;
				next_item->index =  0;
				next_item->kv    =  0;
				++self->stack_size;
			}
		} else if (item->index < item->node->num_entries) {
			if (!item->kv) {
				/* go to next kv */
				item->kv = ss_Node_get_entry(item->node,item->index);
				Assert(item->kv); // should exist
				*hash  = ss_Node_hash(item->node,item->index);
				*key   = ss_Entry_key(item->kv);
				*value = ss_Entry_value(item->kv);
				// the fact that we advanced kv means we are
				// in position for the next call
				return 1;
			} else {
				ss_Entry *next_kv = ss_Entry_next(item->kv);
				if (next_kv) {
					item->kv = next_kv;
					*hash  = ss_Node_hash(item->node,item->index);
					*key   = ss_Entry_key(item->kv);
					*value = ss_Entry_value(item->kv);
					return 1;
				} else {
					++item->index;
					item->kv = 0;
					if (!item->node->is_leaf) {
						Assert(self->stack_size < ss_ITER_STACK_CAPACITY);
						ss_Node *child = ss_Node_child(item->node, item->index);
						Assert(child);
						ss_IterItem *next_item = &self->stack[self->stack_size];
						next_item->node  = child;
						next_item->index = 0;
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


#endif
