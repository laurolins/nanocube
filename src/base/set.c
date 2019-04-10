/* set data structure */

#ifdef set_UNIT_TEST
#include "platform.c"
#endif

//
// offset grows backwards in the set
//
typedef struct {
	u32 offset;
	u32 length;
} set_Block;

typedef struct {
	u32 hash;
	u32 order;
	set_Block key;
	set_Block value;   // optional
	u64 counter;       // set counter number when this entry was inserted
} set_Entry;

// limited to 4G of storage
typedef struct {
	u32 num_entries;
	u32 left;
	u32 right;
	u32 length;
	u64 counter; // a user can manually increment a counter
	             // that will be used to tag entries. The
		     // idea is that a sequential batch of new
		     // entries might get the same counter number
	union {
		char      *base;
		set_Entry *entries;
	};
} set_Set;

//
// copied on Jan 17, 2017 from:
//
// https://github.com/wolkykim/qlibc/blob/master/src/utilities/qhash.c
//
u32 set_murmur3_32(const void *data, size_t nbytes) {
#if 0
	return 1;
#else
	if (data == NULL || nbytes == 0)
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
#endif
}

#if 0
static u32
set_murmur3_32(char* key, size_t len) {
	u32 h = 0; // seed;
	if (len > 3) {
		u32* key_x4 = (u32*) key;
		size_t i = len >> 2;
		do {
			u32 k = *key_x4++;
			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;
			h ^= k;
			h = (h << 13) | (h >> 19);
			h += (h << 2) + 0xe6546b64;
		} while (--i);
		key = (u8*) key_x4;
	}
	if (len & 3) {
		size_t i = len & 3;
		u32 k = 0;
		key = &key[i - 1];
		do {
			k <<= 8;
			k |= *key--;
		} while (--i);
		k *= 0xcc9e2d51;
		k = (k << 15) | (k >> 17);
		k *= 0x1b873593;
		h ^= k;
	}
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}
#endif

static void
set_Set_init(set_Set *self, char *base, u32 length)
{
	self->num_entries = 0;
	self->left  = 0;
	self->right = length;
	self->length = length;
	self->base = base;
	self->counter = 0;
}

static void
set_Set_increment_counter(set_Set *self)
{
	++self->counter;
}

static void
set_Set_set_counter(set_Set *self, u64 counter)
{
	self->counter = counter;
}

static MemoryBlock
set_Set_get_key(set_Set *self, s32 index)
{
	Assert(index < self->num_entries);
	set_Entry *entry = self->entries + index;
	MemoryBlock result;
	result.begin = self->base + self->length - entry->key.offset;
	result.end   = result.begin + entry->key.length;
	return result;
}

static MemoryBlock
set_Set_get_value(set_Set *self, s32 index)
{
	Assert(index < self->num_entries);
	set_Entry *entry = self->entries + index;
	MemoryBlock result;
	if (entry->value.offset == 0) {
		result.begin = 0;
		result.end   = 0;
	} else {
		result.begin = self->base + self->length - entry->value.offset;
		result.end   = result.begin + entry->value.length;
	}
	return result;
}


static s32
set_Set_binary_search_hash(set_Set *self, u32 hash)
{
	u32 l = 0;
	u32 r = self->num_entries;
	if (l == r) {
		return -1;
	}
	while (r - l > 2) {
		u32 m = (l + r) / 2;
		u32 hash_m = self->entries[m].hash;
		/* notice how we either finish, l changes or r changes */
		if (hash < hash_m) {
			r = m;
		} else if (hash > hash_m) {
			l = m + 1;
		} else {
			// go to left most entry with
			// the same hash
			s32 mm = (s32) m - 1;
			while (mm >= 0 && self->entries[mm].hash == hash)
				--mm;
			return mm + 1;
		}
	}
	/* r-l is either 1 or 2 */
	Assert(r - l > 0 && r - l <= 2);
	u32 hash_l = self->entries[l].hash;
	if (hash < hash_l) {
		return -1 - l;
	} else if (hash == hash_l) {
		return l;
	} else {
		if (r - l == 1) {
			return -1 - (l+1);
		} else {
			u32 hash_l1 = self->entries[l+1].hash;
			if (hash < hash_l1) {
				return -1 - (l+1);
			} else if (hash == hash_l1) {
				return l+1;
			} else {
				return -1 - (l+2);
			}
		}
	}
}

#define set_INSERT_FULL     -1
#define set_INSERT_EXISTS    0
#define set_INSERT_INSERTED  1


static set_Entry*
set_Set_insert_with_value(set_Set *self, char *begin, char *end, set_Block value, s32 *status)
{
	u32 new_item_length = (end - begin);

	/* get hash */
	u32 hash = set_murmur3_32(begin, new_item_length);

	/* binary search for hash insertion point */
	s32 index = set_Set_binary_search_hash(self, hash);

	if (index >= 0) {
		/*
		 * search for exact match or prove none exists and adjust
		 * index to insert position on colliding hashes
		 */
		while (index < self->num_entries) {
			// compare content
			set_Entry *it = self->entries + index;
			if (hash != it->hash)
				break;
			char *it_text_begin = self->base + self->length - it->key.offset;
			char *it_text_end   = it_text_begin + it->key.length;
			s32 cmp = cstr_compare_memory(begin, end, it_text_begin, it_text_end);
			if (cmp == 0) {
				if (status) {
					*status = set_INSERT_EXISTS;
				}
				return it;
			} else if (cmp < 0) {
				break;
			} else {
				++index;
			}
		}
	} else {
		index = -index - 1;
	}

	u32 new_left  = self->left + sizeof(set_Entry);
	u32 new_right = self->right - new_item_length;
	if (new_right < new_left) {
		if (status) {
			*status = set_INSERT_FULL;
		}
		return 0;
	}

	/* copy item data */
	char *it = begin;
	for (u32 i=new_right;i<self->right;++i) {
		self->base[i] = *it++;
	}

	/* initialize last entry in the array */
	set_Entry *it_end = self->entries + index;
	{
		set_Entry new_entry = {
			.hash = hash,
			.order = self->num_entries,
			.key = (set_Block) { .offset = self->length - new_right, .length = new_item_length },
			.value = value,
			.counter = self->counter
		};
		/* rotate new_Entry */
		set_Entry *it     = self->entries + self->num_entries;
		while (it != it_end) {
			*it = *(it - 1);
			--it;
		}
		*it_end = new_entry;
	}

	++self->num_entries;
	self->left  = new_left;
	self->right = new_right;

// 	for (u32 i=1;i<self->num_entries;++i) {
// 		Assert(self->entries[i].hash >= self->entries[i-1].hash);
// 	}

	if (status) {
		*status = set_INSERT_INSERTED;
	}
	return it_end;
}

//
// value should have beed stored into the set cache earlier
//
static set_Entry*
set_Set_insert(set_Set *self, char *begin, char *end, s32 *status)
{
	return set_Set_insert_with_value(self, begin, end, (set_Block) { .offset=0, .length=0 }, status);
}


static set_Entry*
set_Set_find(set_Set *self, char *begin, char *end)
{
	/* get hash */
	u32 hash = set_murmur3_32(begin, end - begin);

	/* binary search for hash insertion point */
	s32 index = set_Set_binary_search_hash(self, hash);

	if (index >= 0) {
		/*
		 * search for exact match or prove none exists and adjust
		 * index to insert position on colliding hashes
		 */
		while (index < self->num_entries) {
			// compare content
			set_Entry *it = self->entries + index;
			if (hash != it->hash)
				break;
			char *it_text_begin = self->base + self->length - it->key.offset;
			char *it_text_end   = it_text_begin + it->key.length;
			s32 cmp = cstr_compare_memory(begin, end, it_text_begin, it_text_end);
			if (cmp == 0) {
				return it;
			} else if (cmp < 0) {
				return 0;
			} else {
				++index;
			}
		}
	}
	return 0;
}

static set_Block
set_Set_store_value(set_Set *self, char *begin, char *end)
{
	u32 length = (u32) (end - begin);
	Assert( self->left <= self->right - length);
	self->right -= length;
	char *dst = self->base + self->right;
	char *src = begin;
	while (src != end) {
		*dst = *src;
		++src;
		++dst;
	}
	return (set_Block) { .offset = self->length - self->right, .length = length };
}

#ifdef set_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	u64   len = Megabytes(1);
	char *mem = (char*) malloc(len);
	set_Set set;
	set_Set_init(&set, mem, len);

	char *names[] = { "lauro", "jim", "horace" };

	for (u32 i=0;i<ArrayCount(names);++i) {
		set_Entry *e = set_Set_insert(&set, names[i], cstr_end(names[i]), 0);
		printf("Order is %d\n", e->order);
	}

	{
		set_Entry *e = set_Set_find(&set, names[2], cstr_end(names[2]));
		printf("Order is %d\n", e->order);
	}
}

#endif

