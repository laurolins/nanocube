#ifdef id2bl_UNIT_TEST
#include "platform.c"
#include "cstr.c"
#endif

//-----------------------------------------------------------------------------
//
// dictionary
//
//-----------------------------------------------------------------------------

//
// u32 to regular block
//
// keep some sort of LRU list so that we can
// free the resources that were not used
// for a while
//
// what if we delete an item on the robin-hood?
//
//    - just tag as delete and wait for a major restructure?
//
// u32bl_Map
//
// block size
// implement the delete and LRU
//

//
// dictionary of up to 4G
//
// [ Map ] [ hash ] [ bhandle's ] ----> .... <---- [ strings ]
//
// make sure data_handle 0 is not used
//
// this is more like a set
//
// element   ----> usually some text (eg. "rsrp", "rsrq")
//
// e_handle  ----> a number represeting an element inside the set
//                 can be used to retrieve element
//
// e_order   ----> order in which elemen was inserted
//                 can be used to retrieve element handle
//
// h_entry   ----> slot on the hash table
//
// e_entry   ----> slot where element and its order are stored
//                 a relative position of e_entry to the size
//                 is the e_handle
//

//
// hash map of 32 bits
//

//
// [ id2bl_Map ] [ -----> |                  <----- ]
//
// API:
//
// assumption, payload is of a fixed size
//
// id2bl_insert(<db>,<id>,<payload>)
// id2bl_get(<db>,<id>)  --> <payload>
// id2bl_delete(<db>,<id>)
//
// are there collisions?
//
// LRU list so that we can free up old content
//

typedef struct {
	void *base;
	u16   length;
} id2bl_Payload;

typedef struct {
	u32 bhandle;
	u32 hash;
	u32 dib:31;
	u32 used:1;
} id2bl_HashEntry;

typedef struct {
	u32 used_slots; // used slots
	u32 num_slots;  // slots in the hash table
	u32 left;
	u32 right;
	u32 size;
	u16 block_length;
	u16 payload_length;
	u32 mru_first;  // keep an most/least recently used list
	u32 mru_last;

	id2bl_HashEntry entries[];
} id2bl_Map;

typedef struct {
	u32  id;
	u32  mru_prev;
	u32  mru_next;
	char data[];
} id2bl_Block;

#define id2bl_OK 0
#define id2bl_SLOT_PRESSURE -1
#define id2bl_DATA_PRESSURE -2

//-----------------------------------------------------------------------------
//
// hash
//
//-----------------------------------------------------------------------------

/* pi-hash fn */
// #define HASH(X) (3141592653U * ((unsigned int)(X)) >> (32 - h->k))

//
// copied on Jan 17, 2017 from:
// https://github.com/wolkykim/qlibc/blob/master/src/utilities/qhash.c
//
u32 id2bl_hash(u32 id)
{
	u32 c1 = 0xcc9e2d51;
	u32 c2 = 0x1b873593;

	u32 h = 0;
	u32 k = id;
	k *= c1;
	k = (k << 15) | (k >> (32 - 15));
	k *= c2;

	h ^= k;
	h = (h << 13) | (h >> (32 - 13));
	h = (h * 5) + 0xe6546b64;

	h ^= 4;

	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}


//-----------------------------------------------------------------------------
//
// id2bl_Map
//
//-----------------------------------------------------------------------------

static id2bl_Map*
id2bl_init(void *buffer, u32 length, u16 payload_length, u32 slots)
{
	AssertMultiple((u64)buffer,4);
	AssertMultiple(length,4);

	// zero is initialization
	memset(buffer, 0, length);

	u32 min_storage_required = sizeof(id2bl_Map) + slots * sizeof(id2bl_HashEntry);
	if (length < min_storage_required) {
		return 0;
	}

	u16 block_length = payload_length + sizeof(id2bl_Block);

	id2bl_Map *dict = buffer;
	dict[0] = (id2bl_Map) {
		.used_slots = 0,
		.num_slots = slots,
		.left = sizeof(id2bl_Map),
		.right = length - slots * sizeof(id2bl_HashEntry),
		.size = length,
		.payload_length = payload_length,
		.block_length = block_length,
		.mru_first = 0,  // keep an most/least recently used list
		.mru_last = 0
	};

	return dict;
}


static id2bl_Block*
id2bl_bhandle_to_block_(id2bl_Map *self, u32 bhandle) { return (bhandle) ? (id2bl_Block*) OffsetedPointer(self, bhandle) : (id2bl_Block*) 0; }

static id2bl_HashEntry*
id2bl_entries_(id2bl_Map *self) { return OffsetedPointer(self,self->right); }

static u32
id2bl_block_to_bhandle_(id2bl_Map *self, id2bl_Block *block) { return (block) ? (u32) Offset(self, block) : (u32) 0; }

//
// find first hash entry matching a given hash
//
static id2bl_HashEntry*
id2bl_get_hentry_from_hash_(id2bl_Map *self, u32 hash)
{
	u64 pos = hash % self->num_slots;
	id2bl_HashEntry *entries = id2bl_entries_(self);
	u64 dib = 0;
	for (;;) {
		if (entries[pos].used) {
			if (entries[pos].hash == hash) {
				return entries + pos;
			} else if (dib > entries[pos].dib) {
				// hash and key_handle not inserted
				return 0;
			} else {
				++dib;
				pos = (pos+1) % self->num_slots;
			}
		} else {
			return 0;
		}
	}
}

static id2bl_HashEntry*
id2bl_get_hentry_from_hash_id_(id2bl_Map *self, u32 hash, u32 id)
{
	id2bl_HashEntry* it = id2bl_get_hentry_from_hash_(self, hash);
	id2bl_HashEntry *entries = id2bl_entries_(self);
	id2bl_HashEntry *end = entries + self->num_slots;
	u64 dib = 0;
	while (it->used && it->hash == hash) {
		id2bl_Block *block = id2bl_bhandle_to_block_(self,it->bhandle);
		if (block->id == id) {
			return it;
		} else {
			++it;
			if (it == end) {
				it = entries;
			}
		}
	}
	return 0;
}



static id2bl_Block*
id2bl_linear_exact_search_(id2bl_Map *self, id2bl_HashEntry *it, u32 hash, u32 id)
{
	id2bl_HashEntry *entries = id2bl_entries_(self);
	id2bl_HashEntry *end     = entries + self->num_slots;
	// assuming that dict is never full
	while (it && it->used && (it->hash == hash)) {
		id2bl_Block *record = id2bl_bhandle_to_block_(self, it->bhandle);
		if (record->id == id) {
			return record;
		} else {
			++it;
			if (it == end) it = entries;
		}
	}
	return 0;
}

static s32
id2bl_insert_hentry_(id2bl_Map *self, u32 hash, u32 bhandle)
{
	if (self->used_slots == self->num_slots) return 0;
	u32 pos = hash % self->num_slots;
	u32 dib = 0;
	id2bl_HashEntry *entries = id2bl_entries_(self);
	for (;;) {
		if (!entries[pos].used) {
			// it is empty. go ahead and insert it.
			entries[pos] = (id2bl_HashEntry) { .hash = hash, .bhandle = bhandle, .dib=dib, .used = 1 };
			++self->used_slots;
			return 1;
		} else if (entries[pos].dib < dib) {
			// swap
			Swap(bhandle, entries[pos].bhandle);
			Swap(hash, entries[pos].hash);
			u32 aux = dib;
			dib = entries[pos].dib;
			entries[pos].dib = aux;
		}
		++dib;
		pos = (pos+1) % self->num_slots;
	}
}










static id2bl_Block*
id2bl_insert_(id2bl_Map *self, u32 id, void *payload, s32 dont_duplicate, s32 *out_status)
{
	u32 hash = id2bl_hash(id);

	if (dont_duplicate) {
		// check if element already exists
		id2bl_HashEntry *it = id2bl_get_hentry_from_hash_(self, hash);
		id2bl_Block *block = id2bl_linear_exact_search_(self, it, hash, id);
		if (block) return block;
	}

	// check for slot pressure
	if (4 * self->used_slots >= 3 * self->num_slots) {
		msg2("dictinary is suffering from slot pressure\n");
		if (out_status) out_status[0] = id2bl_SLOT_PRESSURE;
		return 0;
	}

	// check if we can fit new data
	if (self->left + self->block_length > self->right) {
		msg2("dictinary is suffering from data pressure\n");
		if (out_status) out_status[0] = id2bl_DATA_PRESSURE;
		return 0;
	}

	u32 bhandle = self->left;
	id2bl_Block *record = OffsetedPointer(self,bhandle);
	self->left += self->block_length;

	record[0] = (id2bl_Block) {
		.id = id,
		.mru_prev = 0,
		.mru_next = 0
	};
	platform.copy_memory(&record->data[0], payload, self->payload_length);

	s32 ok = id2bl_insert_hentry_(self, hash, bhandle);

	Assert(ok);
	/*
	msg("set %p element '%.*s' order %"PRIu32" handle %"PRIu32"\n",
		self,
		record->length,
		record->element,
		record->eorder,
		bhandle);
		*/

	if (out_status) out_status[0] = id2bl_OK;

	return record;

}


static s32
id2bl_delete_(id2bl_Map *self, u32 hash, u32 id)
{
	id2bl_HashEntry *it = id2bl_get_hentry_from_hash_id_(self, hash, id);

	// item does not exist
	if (!it) return 0;

	id2bl_HashEntry *entries = id2bl_entries_(self);
	id2bl_HashEntry *end     = entries + self->num_slots;

	u32 bhandle_dst = it->bhandle;

	//
	// it exists: delete from entries and push back everything that was
	// move the payload of the last block to the place
	//
	for (;;) {
		id2bl_HashEntry *next_it = it + 1;
		if (next_it == end) next_it = entries;
		if (next_it->used && next_it->dib > 0) {
			--next_it->dib;
			it[0] = next_it[0];
		} else {
			// mark it as not used
			it[0] = (id2bl_HashEntry) { 0 };
			break;
		}
		it = next_it;
	}

	//
	u32 bhandle_src = self->left - self->block_length;
	if (bhandle_src != bhandle_dst) {

		Assert(bhandle_dst < bhandle_src);

		platform.copy_memory(OffsetedPointer(self,bhandle_dst),
				     OffsetedPointer(self,bhandle_src),
				     self->block_length);

		id2bl_Block *block_dst = id2bl_bhandle_to_block_(self, bhandle_dst);
		u32 id_dst = block_dst->id;
		u32 hash_dst = id2bl_hash(id_dst);
		id2bl_HashEntry *hentry_dst = id2bl_get_hentry_from_hash_id_(self, hash_dst, id_dst);
		Assert(hentry_dst->bhandle = bhandle_src);
		hentry_dst->bhandle = bhandle_dst;
	}

	// shrink the payload used storage and used_slots
	--self->used_slots;
	self->left -= self->block_length;

	return 1;
}

static s32
id2bl_delete(id2bl_Map *self, u32 id)
{
	u32 hash = id2bl_hash(id);
	return id2bl_delete_(self, hash, id);
}

static id2bl_Block*
id2bl_get_(id2bl_Map *self, u32 id)
{
	u32 hash = id2bl_hash(id);
	id2bl_HashEntry *e = id2bl_get_hentry_from_hash_(self, hash);
	return id2bl_linear_exact_search_(self, e, hash, id);
}

static id2bl_Payload
id2bl_get(id2bl_Map *self, u32 id)
{
	id2bl_Block *block = id2bl_get_(self, id);
	if (block) {
		return (id2bl_Payload) { .base = &block->data[0], .length = self->payload_length };
	} else {
		return (id2bl_Payload) { .base = 0, .length = 0 };
	}
}

// return the block_handle of inserted element
static u32
id2bl_insert(id2bl_Map *self, u32 id, id2bl_Payload payload, s32 dont_duplicate, s32 *out_status)
{
	Assert(payload.length == self->payload_length);
	id2bl_Block *block = id2bl_insert_(self, id, payload.base, dont_duplicate, out_status);
	return id2bl_block_to_bhandle_(self, block);
}



#ifdef id2bl_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

#include "arena.c"
#include "print.c"

#include "../platform_dependent/nix_platform.c"

int main(int argc, char *argv[])
{
	nix_init_platform(&platform);

	// insert keys and payloads will be the keys as well

	u32 buffer_length = Megabytes(1);
	u32 *buffer = platform.allocate_memory_raw(buffer_length,0);

	id2bl_Map *map = id2bl_init(buffer, buffer_length, 4, 1024);

	s32 status = 0;
	for (u32 i=0;i<512;++i) {
		id2bl_Payload payload = {.base=&i, .length=sizeof(u32) };
		u32 block_handle = id2bl_insert(map, i, payload, 1, &status);
		msg("insert %"PRIu32" --> %"PRIu32"\n", i, block_handle);
	}

	for (u32 i=0;i<512;++i) {
		id2bl_Payload payload = id2bl_get(map, i);
		u32 value = ((u32*)payload.base)[0];
		msg("get %"PRIu32" --> %"PRIu32"\n", i, value);
	}

	for (u32 i=1;i<512;i+=2) {
		id2bl_delete(map, i);
	}

	for (u32 i=0;i<512;i+=2) {
		id2bl_Payload payload = id2bl_get(map, i);
		u32 value = ((u32*)payload.base)[0];
		msg("get2 %"PRIu32" --> %"PRIu32"\n", i, value);
	}


	platform.free_memory_raw(buffer);

	return 0;
}

#endif

