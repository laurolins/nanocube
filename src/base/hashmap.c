// hashmap of arbitrary blob keys to arbitrary blob values

/* set data structure */

#ifdef hashmap_UNIT_TEST
#include "platform.c"
#include "cstr.c"
#include "bst.c"
#endif


//------------------------------------------------------------------------------
//
// log
//
//------------------------------------------------------------------------------

#define hm_DEBUG_LEVEL 2


// #define hm_DEBUG_CHANNEL stdout

#ifndef hm_DEBUG_CHANNEL
#define hm_DEBUG_CHANNEL stderr
#endif

#define print(st) fprintf(stderr, "%s", st)

#if hm_DEBUG_LEVEL >= 0
#define hm_log0(format, ...) fprintf(hm_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define hm_log0(format, ...)
#endif

#if hm_DEBUG_LEVEL > 0
#define hm_log(format, ...) fprintf(hm_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define hm_log(format, ...)
#endif

#if hm_DEBUG_LEVEL > 1
#define hm_log2(format, ...) fprintf(hm_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define hm_log2(format, ...)
#endif

#if hm_DEBUG_LEVEL > 2
#define hm_log3(format, ...) fprintf(hm_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define hm_log3(format, ...)
#endif


//
// copied on Jan 17, 2017 from:
//
// https://github.com/wolkykim/qlibc/blob/master/src/utilities/qhash.c
//
static u32
hm_murmur3_32(const void *data, size_t nbytes) {
	if (data == 0 || nbytes == 0)
		return 0;

	const u32 c1 = 0xcc9e2d51;
	const u32 c2 = 0x1b873593;

	const int nblocks = nbytes / 4;
	const u32 *blocks = (const u32 *) (data);
	const u8 *tail = (const u8 *) (data + (nblocks * 4));

	u32 h = 0;

	int i;
	u32 k;
	for (i = 0; i < nblocks; i++) {
		k = blocks[i];

		k *= c1;
		k = (k << 15) | (k >> (32 - 15));
		k *= c2;

		h ^= k;
		h = (h << 13) | (h >> (32 - 13));
		h = (h * 5) + 0xe6546b64;
	}

	k = 0;
	switch (nbytes & 3) {
	case 3:
		k ^= tail[2] << 16;
	case 2:
		k ^= tail[1] << 8;
	case 1:
		k ^= tail[0];
		k *= c1;
		k = (k << 15) | (k >> (32 - 15));
		k *= c2;
		h ^= k;
	};

	h ^= nbytes;

	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}


//
// robin hood hashing
//
// delete from hashmap using backward shift method
// http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/#ref
//

typedef struct {
	u64 data;
	u32 hash;
	u32 dib:31;
	u32 used:1;
} hm_HItem;

typedef struct {
	u32 item_count;
	u32 item_capacity;
} hm_HMap;

StaticAssertAlignment(hm_HItem,8);
StaticAssertAlignment(hm_HMap,8);

static u32
hm_hmap_size(hm_HMap* self)
{
	return sizeof(hm_HMap) + self->item_capacity * sizeof(hm_HItem);
}

static hm_HMap*
hm_hmap_new(void *buffer, u32 buffer_length)
{
	Assert((u64)buffer % 8 == 0);
	Assert(buffer_length % 8 == 0);
	Assert(buffer_length > sizeof(hm_HMap));
	u32 capacity = (buffer_length - sizeof(hm_HMap)) / sizeof(hm_HItem);
	hm_HMap *map = buffer;
	map[0] = (hm_HMap) {
		.item_count = 0,
		.item_capacity = capacity
	};
	return map;
}

static s32
hm_hmap_full(hm_HMap *self)
{
	return self->item_count == self->item_capacity;
}

static u32
hm_hmap_size_for_capacity(u32 capacity)
{
	return sizeof(hm_HMap) + sizeof(hm_HItem) * capacity;
}

static hm_HItem*
hm_hmap_items(hm_HMap *self) { return OffsetedPointer(self, sizeof(hm_HMap)); }

static s32
hm_hmap_insert_(hm_HMap *self, u32 hash, u64 data)
{
	if (self->item_count == self->item_capacity) return 0;
	u32 pos = hash % self->item_capacity;
	u32 dib = 0;
	hm_HItem *items = hm_hmap_items(self);
	for (;;) {
		if (!items[pos].used) {
			// it is empty. go ahead and insert it.
			items[pos] = (hm_HItem) { .hash = hash, .data = data, .dib=dib, .used = 1 };
			++self->item_count;
			return 1;
		} else if (items[pos].dib < dib) {
			// swap
			Swap(data, items[pos].data);
			Swap(hash, items[pos].hash);
			// Swap(dib,  items[pos].dib);
			u32 aux = dib;
			dib = items[pos].dib;
			items[pos].dib = aux;
		}
		++dib;
		pos = (pos+1) % self->item_capacity;
	}
}

static hm_HItem*
hm_hmap_get_(hm_HMap *self, u32 hash)
{
	s64 pos = hash % self->item_capacity;
	hm_HItem *items = hm_hmap_items(self);
	u32 dib = 0;
	for (;;) {
		if (items[pos].used) {
			if (items[pos].hash == hash) {
				return items + pos;
			} else if (dib > items[pos].dib) {
				// hash and key_handle not inserted
				return 0;
			} else {
				++dib;
				pos = (pos+1) % self->item_capacity;
			}
		} else {
			return 0;
		}
	}
}

static s32
hm_hmap_delete_(hm_HMap *self, u32 hash)
{
	s64 pos = hash % self->item_capacity;
	hm_HItem *items = hm_hmap_items(self);
	u32 dib = 0;

	for (;;) {
		if (items[pos].used) {
			if (items[pos].hash == hash) {
				break;
			} else if (dib > items[pos].dib) {
				// hash and key_handle not inserted
				return 0;
			} else {
				++dib;
				pos = (pos+1) % self->item_capacity;
			}
		} else {
			return 0;
		}
	}

	--self->item_count;

	// we get here with 'pos' marking the delete position
	// back shift every consecutive position that is being
	// used and is not in its exact place (dib == 0)
	u32 prev_pos = pos;
	for (u32 i=1;i<self->item_capacity;++i) {
		u32 curr_pos = (pos + i) % self->item_capacity;
		if (items[curr_pos].used && items[curr_pos].dib > 0) {
			--items[curr_pos].dib;
			items[prev_pos] = items[curr_pos];
			prev_pos = curr_pos;
		} else {
			// either we found a non-used slot, make prev_pos
			// be empty and non-used
			items[prev_pos] = (hm_HItem) { 0 };
			break;
		}
	}

	return 1;
}

//
// Item of a linked list which should be associated with the same 'hash'
//
typedef struct {
	u64 next; // offset of the next hm_HItem with the same hash
	u32 key_length;
	u32 value_length;
	char data[];
} hm_Item; // in terms of block storage let it be the next multiple of 8

//
// keep a free list of free blocks
//
typedef struct {
	u64 offset; // offset from the right side
	u64 length; // size of the block
} hm_Block;


// [hm_Map] [ hm_Hmap] [bst_Tree] ... <--- [bst_Items]
typedef struct {
	u32 hashes; // how many hashes are stored
	u32 count; // how many key,value pairs are stored
	u64 bytes_used; // bytes in used items
	u64 left;
	u64 right;
	u64 size;
} hm_Map;

StaticAssertAlignment(hm_Item,8);
StaticAssertAlignment(hm_Block,8);
StaticAssertAlignment(hm_Map,8);

static hm_HMap*
hm_hmap(hm_Map *self)
{
	return OffsetedPointer(self, sizeof(hm_Map));
}

static bst_Tree*
hm_free_bst(hm_Map *self)
{
	hm_HMap* hmap = hm_hmap(self);
	return OffsetedPointer(hmap, hm_hmap_size(hmap));
}

static hm_Item*
hm_item(hm_Map* self, u64 item_offset)
{
	if (!item_offset) return 0;
	return RightOffsetedPointer(self, self->size, item_offset);
}

static u32
hm_hash(void *buffer, u32 length)
{
	return hm_murmur3_32(buffer, length);
}

static void*
hm_item_key(hm_Item *self)
{
	return OffsetedPointer(self,sizeof(hm_Item));
}

static void*
hm_item_value(hm_Item *self)
{
	return OffsetedPointer(self,sizeof(hm_Item) + RAlign(self->key_length,8));
}

static u32
hm_normalized_item_length(u32 key_length, u32 value_length)
{
	return sizeof(hm_Item) + RAlign(key_length,8) + RAlign(value_length,8);
}

static hm_Block
hm_cut_block(hm_Block *block, u32 length)
{
	Assert(length <= block->length);
	hm_Block remaining = { .offset = block->offset - block->length, .length = block->length - length };
	block->length -= remaining.length;
	return remaining;
}

static void
hm_dump(hm_Map *self)
{
	hm_HMap *hmap = hm_hmap(self);
	// just list the keys and first k bytes of the values


	hm_log0("[ hm_Map hashes=%d count=%d right=%"PRIu64" size=%"PRIu64" ]\n", self->hashes, self->count, self->right, self->size);

	hm_log0("....[ hmap count=%d capacity=%d ]\n", hmap->item_count, hmap->item_capacity);

	hm_HItem *items = hm_hmap_items(hmap);
	for (s32 i=0;i<hmap->item_capacity;++i) {
		if (items[i].used) {

			hm_log0("........[ hm_HItem:  slot:%"PRId32"  data:%"PRIu64"  hash:%08x  dib:%"PRIu32"  used:%"PRIu32"]\n", i, items[i].data, items[i].hash, items[i].dib, items[i].used);
			u64 item_handle = items[i].data;
			while (item_handle) {
				hm_Item *it = hm_item(self, item_handle);
				char *key   = hm_item_key(it);
				char *value = hm_item_value(it);
				// hm_normalized_item_length(u32 key_length, u32 value_length)
				hm_log0("............[hm_Item off:%"PRIu64" len:%"PRIu32"\n", item_handle, hm_normalized_item_length(it->key_length, it->value_length));
				// hm_log0("............key: '%s'  value: '%s'\n", key, value);
				item_handle = it->next;
			}
		}
	}

	bst_Tree *free_bst = hm_free_bst(self);
	hm_log0("....[ free blocks count: %d ]\n", free_bst->count);
	bst_Node *nodes = bst_nodes(free_bst);
	for (s32 i=0;i<free_bst->node_count;++i) {
		if (nodes[i].used) {
			u32 value_handle = nodes[i].value;
			// bst_Node *node = nodes + i;
			while (value_handle) {
				// loop on all values for the nodes with the same key
				hm_Block *block = bst_value(free_bst, value_handle);
				hm_log0("........[ handle: %"PRIu32" key: %"PRIu64" block offset: %"PRIu64" length: %"PRIu64" ]\n", value_handle, nodes[i].key, block->offset, block->length);
				bst_ValueTrailer* value_trailer = bst_value_trailer(free_bst, value_handle);
				value_handle = value_trailer->next;
			}
		}
	}
}





static hm_Map*
hm_new(void *buffer, u64 buffer_length, u32 expected_max_items)
{
	Assert((u64) buffer % 8 == 0);
	Assert(buffer_length % 8 == 0);

	u32 hmap_storage     = hm_hmap_size_for_capacity(3 * expected_max_items / 2);
	u32 free_bst_storage = bst_size_for_capacity(sizeof(hm_Block), expected_max_items);
	Assert(hmap_storage % 8 == 0);
	Assert(free_bst_storage % 8 == 0);

	// estimate an average of 16 byte keys and 16 byte values
	u64 item_avg_length = RAlign(sizeof(hm_Item) + 32,8);
	u32 min_item_storage = item_avg_length * expected_max_items;

	if (hmap_storage + free_bst_storage + min_item_storage > buffer_length)
		return 0;

	hm_Map *db = buffer;
	*db = (hm_Map) {
		.hashes = 0,
		.count = 0,
		.bytes_used = 0,
		.left = 0,
		.right = buffer_length,
		.size = buffer_length
	};

	void *hmap_buffer = hm_hmap(db);
	hm_HMap *hmap = hm_hmap_new(hmap_buffer, hmap_storage);

	void *free_bst_buffer = hm_free_bst(db);
	bst_Tree *free_bst = bst_new(sizeof(hm_Block), free_bst_buffer, free_bst_storage);

	db->left = PointerDifference(OffsetedPointer(free_bst,free_bst->size),db);

	return db;
}

static hm_Map*
hm_new_raw(u64 size, u32 expected_max_items)
{
	void *buffer = platform.allocate_memory_raw(size,0);
	return hm_new(buffer, size, expected_max_items);
}

static hm_Item*
hm_insert(hm_Map *self, u32 hash, void *key_buffer, u32 key_length, u32 value_length, char *value_buffer)
{
	// does it fit
	hm_HMap  *hmap = hm_hmap(self);

	hm_log2("inserting hash:%08x  slot:%"PRIu32"   key_length:%"PRIu32"  value_length:%"PRIu32"\n", hash, (hash%hmap->item_capacity), key_length, value_length);

	// if key already exists. Users of hm_Map should check for the
	// existence of the

	bst_Tree  *free_bst = hm_free_bst(self);

	// check that new entry will fit
	if (bst_full(free_bst) || hm_hmap_full(hmap))
		return 0;

	u64 item_length =  hm_normalized_item_length(key_length, value_length);

	//
	// @note: bst needs to make sure the element found in lower bound
	// is the same as the one deleted
	//
	// get the best fit block from the free list
	bst_Node *best_fit = bst_lower_bound(free_bst, item_length);

	// no space for the new item
	if (!best_fit && self->left + item_length > self->right)
		return 0;

	// prepare the item
	hm_Item *item = 0;
	if (best_fit) {
		// get the block and pop once
		hm_Block block = ((hm_Block*) bst_value(free_bst,best_fit->value))[0];

		// update the list of free blocks
		hm_log2("reusing %"PRIu64" bytes from block { off:%"PRIu64" len:%"PRIu64" }\n", item_length, block.offset, block.length);

		u64 block_length = block.length;
		hm_Block remaining_block = hm_cut_block(&block, item_length);

		hm_log2("deleting block { off:%"PRIu64" len:%"PRIu64" } from free list\n", block.offset, block.length);

		hm_log2("....before delete...\n");
		hm_dump(self);

		bst_delete(free_bst, block_length);

		hm_log2("....after delete...\n");
		hm_dump(self);

		if (remaining_block.length) {
			hm_log2("insert block into free list { off:%"PRIu64" len:%"PRIu64" }\n", block.offset, block.length);
			s32 status = bst_insert(free_bst, remaining_block.length, (void*) &remaining_block, sizeof(hm_Block));
			Assert(status);
		}


		item = hm_item(self, block.offset);
	} else {
		self->right -= item_length;
		item = OffsetedPointer(self, self->right);
	}
	Assert(item);
	*item = (hm_Item) {
		.next = 0,
		.key_length = key_length,
		.value_length = value_length
	};
	platform.copy_memory(hm_item_key(item),   key_buffer, key_length);
	if (value_buffer) {
		platform.copy_memory(hm_item_value(item), value_buffer, value_length);
	} else {
		platform.memory_set(hm_item_value(item), 0, value_length);
	}

	u64 item_handle = self->size - PointerDifference(item,self);

	hm_HItem *hitem = hm_hmap_get_(hmap, hash);
	if (!hitem) {
		hm_hmap_insert_(hmap, hash, item_handle);
		++self->hashes;
	} else {
		// stack this item on top of the previous item with the same hash
		// existing_item = hm_item(self, hitem->data);
		item->next = hitem->data;
		hitem->data = item_handle;
	}
	++self->count;
	self->bytes_used += item_length;

	return item;
}

static void*
hm_get(hm_Map *self, u32 hash, void *key_buffer, u32 key_length, u32 *out_length)
{
	// does it fit
	hm_HMap  *hmap     = hm_hmap(self);
	bst_Tree  *free_bst = hm_free_bst(self);

	//
	hm_HItem *hitem = hm_hmap_get_(hmap, hash);
	if (!hitem) { return 0; }

	u64 item_handle = hitem->data;
	while (item_handle) {
		hm_Item *item = hm_item(self, item_handle);
		char *key = hm_item_key(item);
		if (key_length != item->key_length) {
			item_handle = item->next;
		} else {
			// compare
			s32 cmp = cstr_compare_memory(key_buffer, key_buffer + key_length, key, key + key_length);
			if (cmp == 0) {
				if (out_length) out_length[0] = item->value_length;
				return hm_item_value(item);
			} else {
				item_handle = item->next;
			}
		}
	}
	return 0;
}

static s32
hm_delete(hm_Map *self, u32 hash, void *key_buffer, u32 key_length)
{
	// Assuming key doesn't exist. Inconsistent state might occur
	// if key already exists. Users of hm_Map should check for the
	// existence of the

	// does it fit
	hm_HMap  *hmap = hm_hmap(self);
	bst_Tree  *free_bst = hm_free_bst(self);

	//
	hm_HItem *hitem = hm_hmap_get_(hmap, hash);
	if (!hitem) { return 0; }

	hm_Item *prev_item = 0;
	hm_Item *item = 0;
	u64 item_handle = hitem->data;
	while (item_handle) {
		item = hm_item(self, hitem->data);
		void *key = hm_item_key(item);
		// have to compare the actual keys to be certain
		s32 cmp = cstr_compare_memory(key_buffer, key_buffer + key_length,
					    key,        key        + key_length);
		if (cmp == 0) {
			break;
		} else {
			prev_item = item;
			item_handle = item->next;
		}
	}

	if (!item_handle) { return 0; } // the hash exists, but not the key

	if (prev_item) {
		prev_item->next = item->next;
	} else if (item->next) {
		hitem->data = item->next;
	}

	// no more items with the given hash
	u32 item_length = hm_normalized_item_length(item->key_length, item->value_length);
	u64 item_handle_left_offset = self->size - item_handle;
	if (item_handle_left_offset == self->right) {
		self->right += item_length;
	} else {
		hm_Block block = {
			.offset = item_handle,
			.length = item_length
		};
		hm_log2("inserting block { off:%"PRIu64" len:%"PRIu64" } to free list\n", block.offset, block.length );
		s32 status = bst_insert(free_bst, item_length, &block, sizeof(hm_Block));
		Assert(status);
	}

	if (hitem->data) {
		// remove hitem
		hm_log2("deleting hash from hmap: %"PRIu32"\n",hash);
		hm_hmap_delete_(hmap, hash);
		--self->hashes; // number of hashes
	}

	--self->count;  // number of k,v pairs
	self->bytes_used -= item_length;

	return 1;

}












//-----------------------------------------------------------------------------
//
//
//
// main
//
//
//
//-----------------------------------------------------------------------------

#ifdef hashmap_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

#include "arena.c"
#include "print.c"

#include "../platform_dependent/nix_platform.c"

int test2()
{
	char *key_values[] = {
		"3"  , "NORTHERN CALIFORNIA",
		"19" , "PR/VI",
		"6"  , "NY/NJ",
		"13" , "NEW ENGLAND",
		"1"  , "IL/WI/MI",
// 		"14" , "MID ATLANTIC",
// 		"11" , "GA/SC",
// 		"20" , "NA",
// 		"10" , "GREATER LOS ANGELES",
// 		"12" , "PNW/AK/HI",
// 		"8"  , "SOUTH TEXAS",
// 		"15" , "DC/MD/DE",
// 		"0"  , "AR/KS/MO/OK",
// 		"18" , "NORTHERN PLAINS",
// 		"7"  , "NORTH TEXAS",
// 		"9"  , "GULF STATES",
// 		"17" , "ROCKY MTNS",
// 		"5"  , "IN/TN/KY",
// 		"16" , "SOUTHWEST",
// 		"2"  , "OH/PA",
// 		"4"  , "FLORIDA",
	};


	u32   buffer_length = Megabytes(1);
	char *buffer = malloc(buffer_length);

	// s32 n = ArrayCount(key_values)/2;

	hm_Map *db = hm_new(buffer, buffer_length, ArrayCount(key_values)/2);

	// store
	for (s32 i=0;i<ArrayCount(key_values);i+=2) {
		char *key = key_values[i];
		char *val = key_values[i+1];
		u32 key_length = cstr_len0(key);
		u32 hash = hm_hash(key, key_length);
		fprintf(stderr, "[insert] '%s' -> '%s'\n", key, val);
		if (!hm_insert(db, hash, key, key_length, cstr_len0(val), val)) {
			fprintf(stderr,"dictionary is full\n");
		}
		hm_dump(db);
	}

	for (s32 i=0;i<ArrayCount(key_values);i+=2) {
		char *key = key_values[i];
		u32 key_length = cstr_len0(key_values[i]);
		u32 hash = hm_hash(key, key_length);
		u32 value_length = 0;
		char *value = hm_get(db, hash, key, key_length, &value_length);
		if (value) {
			fprintf(stderr, "[query] '%s' -> '%s'\n", key, value);
		} else {
			fprintf(stderr, "[query] '%s' -> NOT_PRESENT\n", key);
		}
	}

	return 0;
}

static char *keys[] = { "lauro", "jim", "horace" };
static char *values[] = { "lins", "klosowski", "ip" };

void query(hm_Map *db)
{
	for (s32 i=0;i<ArrayCount(keys);++i) {
		u32 key_length = cstr_len(keys[i])+1;
		u32 hash = hm_hash(keys[i], key_length);
		u32 value_length = 0;
		char *value = hm_get(db, hash, keys[i], key_length, &value_length);
		if (value) {
			fprintf(stderr, "[query] '%s' -> '%s'\n", keys[i], value);
		} else {
			fprintf(stderr, "[query] '%s' -> NOT_PRESENT\n", keys[i]);
		}
	}
}

int test1()
{
	u32   buffer_length = Kilobytes(16);
	char *buffer = malloc(buffer_length);
	hm_Map *db = hm_new(buffer, buffer_length, 100);

	// store
	for (s32 j=0;j<ArrayCount(keys);++j) {
		s32 i = j;

		u32 key_length = cstr_len0(keys[i]);
		u32 hash = hm_hash(keys[i], key_length);
		fprintf(stderr, "[insert] '%s' -> '%s'\n", keys[i], values[i]);
		hm_insert(db, hash, keys[i], key_length, cstr_len0(values[i]), values[i]);
		hm_dump(db);
	}

	for (s32 j=0;j<ArrayCount(keys);++j) {
		s32 i = j; // ArrayCount(keys) - 1 - j;
		if (i == 2) continue;

		u32 key_length = cstr_len0(keys[i]);
		u32 hash = hm_hash(keys[i], key_length);
		fprintf(stderr, "[delete] key '%s'\n", keys[i]);
		hm_delete(db, hash, keys[i], key_length);
		hm_dump(db);
	}

	// insert back in reverse order the deleted items
	for (s32 j=0;j<ArrayCount(keys);++j) {
		s32 i = j; // ArrayCount(keys) - 1 - j;
		if (i == 2) continue;

		u32 key_length = cstr_len0(keys[i]);
		u32 hash = hm_hash(keys[i], key_length);
		fprintf(stderr, "[insert] '%s' -> '%s'\n", keys[i], values[i]);
		hm_insert(db, hash, keys[i], key_length, cstr_len0(values[i]), values[i]);
		hm_dump(db);
	}

	// double insert in direct order
	for (s32 j=0;j<ArrayCount(keys);++j) {
		s32 i = j;

		u32 key_length = cstr_len0(keys[i]);
		u32 hash = hm_hash(keys[i], key_length);
		fprintf(stderr, "[insert] '%s' -> '%s'\n", keys[i], values[i]);
		hm_insert(db, hash, keys[i], key_length, cstr_len0(values[i]), values[i]);
		hm_dump(db);
	}

	query(db);

	return 0;
}



int main(int argc, char *argv[])
{
	nix_init_platform(&platform);

	// test1();
	test2();

	return 0;
}

#endif

