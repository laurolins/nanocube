/*
 * al_ allocation
 */

#define al_BITS_PER_PAGE 12
#define al_PAGE_SIZE 4096
#define al_NULL_PAGE 0
#define al_BITS_PAGE_ID 32
#define al_PAGEMAP_LEVEL1_ENTRIES 65536
#define al_PAGEMAP_LEVEL1_BITS 16
#define al_PAGEMAP_LEVEL2_ENTRIES 65536
#define al_PAGEMAP_LEVEL2_BITS 16
#define al_PAGEMAP_LEVEL2_MASK 0xFFFF

#define al_FLAG_NORMAL_CACHE 0
#define al_FLAG_SLAB_CACHE 1
#define al_FLAG_CACHE_CACHE 2

#define al_MAX_CACHE_NAME_LENGTH 29
#define al_MIN_CACHE_CHUNK_SIZE 8

//
// 4 bytes per page; assuming at most 16TB of memory will be managed
// this is the page offset of the Allocator
//
// if PageID "p" equals to 0 we say it is a sentinel to a nullpage
//
// typedef u32 u32;

// num pages cannot be larger than 2^32: 16TB of memory
// typedef u32 u32;

// index in the page map
// typedef u32 u32;
// typedef u64 u64;
// typedef u64 u64;

// a slab has at most 4B chunks
// typedef u64 u64;

// These three Ptr_* types were generated from
// nanocube_ptr.template using sed
PTR_SPECIALIZED_TYPE(al_Ptr_Slab);
PTR_SPECIALIZED_TYPE(al_Ptr_Cache);
PTR_SPECIALIZED_TYPE(al_Ptr_char);
PTR_SPECIALIZED_TYPE(al_Ptr_Allocator);

typedef struct {
	al_Ptr_Slab first_p;
	al_Ptr_Slab last_p;
} al_SlabList;

typedef struct {
	al_Ptr_Cache first_p;
	al_Ptr_Cache last_p;
} al_CacheList;

typedef struct {
	u32 page_index;
	u32 page_size;
} al_PageBlock;

// Slab: a run of pages containing chunks of a specific
// object size used for some cache (header contains a span record)
typedef struct {
	al_PageBlock block;

	u64 used; // number of chunks used

	al_Ptr_Cache cache_p;

	al_Ptr_Slab  prev_p; // prev next slabs on the same cache
	al_Ptr_Slab  next_p; // and same state: (parcial/empty) vs. full

	al_Ptr_char  free_chunks; // page page_index address (avoid having to have access to page_index ptr)
	// @Todo check if overhead of initializing the single linked list is
	// interesting here: there will be an extra hop every time we allocate
	// an object that is not on the free list. Maybe uniformity and price
	// at initialization is best (wasting 4 bytes per slab if we consider
	// an 8 byte alignment)
} al_Slab;

// An object cache based on the slab allocator ideas
typedef struct {

	u64 chunk_size;

	u64 chunk_capacity; // num chunks on all slabs

	u64 used_chunks; // num chunks on all slabs

	u32 pages; // num pages in all slabs on free and full lists

	u32 flags;

	al_SlabList free_slabs; // that have at least one empty chunk/buffer/object

	al_SlabList full_slabs; // list of slabs of this class size with no empty slot

	char name[al_MAX_CACHE_NAME_LENGTH + 1]; // a name with up to 31 characters

	al_Ptr_Allocator allocator_p;
	al_Ptr_Cache     next_p;
	al_Ptr_Cache     prev_p;

} al_Cache;

//
// assuming a total space of 2^32 * 2^12 = 2^44 = 16TB
//
// info to map a page in the 2^32 page space into a slab
// all meta data is stored outside of the content pages
//

//
// an entry level 2 consists of a map of all pages
// in a 32-bit page space where its first 16-bits are
// fixed.
//
// At every 2^16 * 2^12 byte allocation, a new
// PageMap_Level2 entry might be allocated
//
typedef struct {

	// page where an array of 2^16 Span pointers are located
	// these pointers map to the meta data (or span information)
	// of the contesnt stored in those pages
	u32 page; // offset where the PageMap_Level2 node is located

	// number of pages used in the 2^16 pages mapped by this
	// entry level
	u32 used_pages;

} al_PageMap_Level1;

//
typedef struct {

	// PageMap entry on level 2 is simply an array of span pointers
	// (stored in a relative way with Ptr)
	al_Ptr_Slab slabs[al_PAGEMAP_LEVEL2_ENTRIES];

} al_PageMap_Level2;

// ceil(sizeof(PageMap_Level2)/PAGE_SIZE) = 96
#define al_PAGEMAP_LEVEL2_PAGES 96

typedef struct {

	al_PageMap_Level1 level1_nodes[al_PAGEMAP_LEVEL1_ENTRIES];

} al_PageMap;

// there can be multiple lla::Allocator being used in the same process
// wrapping it into a class. the position of this object must be aligned
// with a page from the system
typedef struct {

	// max page used by this allocator
	u32 used_pages;

	// max page used by this allocator
	u32 page_capacity;

	// expecting to use this dobly-linked list for debugging and reporting
	// purposes; should not ne on the track of allocation or freeing
	al_CacheList caches;

	// two main caches
	al_Ptr_Cache caches_cache_p;
	al_Ptr_Cache slabs_cache_p;

	// pagemap
	al_PageMap pagemap;

	// optional root_p object that can be associated with an Allocator
	al_Ptr_char root_p;

	u16 padding;

} al_Allocator;



//------------------------------------------------------------------------------
// Boilerplate Ptr_Slab
//------------------------------------------------------------------------------

PTR_SPECIALIZED_SERVICES(al_Ptr_Slab, al_Slab);
PTR_SPECIALIZED_SERVICES(al_Ptr_Cache, al_Cache);
PTR_SPECIALIZED_SERVICES(al_Ptr_char, char);
PTR_SPECIALIZED_SERVICES(al_Ptr_Allocator, al_Allocator);

//------------------------------------------------------------------------------
// SlabList boilerplate code
//------------------------------------------------------------------------------

internal inline al_Slab*
al_SlabList_first(al_SlabList *self)
{
	return al_Ptr_Slab_get(&self->first_p);
}

internal inline al_Slab*
al_SlabList_last(al_SlabList *self)
{
	return al_Ptr_Slab_get(&self->last_p);
}

internal inline b8
al_SlabList_is_empty(al_SlabList *self)
{
	return al_Ptr_Slab_is_null(&self->first_p);
}

internal void
al_SlabList_init(al_SlabList *self)
{
	al_Ptr_Slab_set_null(&self->first_p);
	al_Ptr_Slab_set_null(&self->last_p);
}

internal void
al_SlabList_insert(al_SlabList *self, al_Slab *new_item, al_Slab *at)
{
	if (!at) { // if at == nullptr append
		if (al_SlabList_is_empty(self)) {
			Assert(al_Ptr_Slab_is_null(&new_item->next_p));
			Assert(al_Ptr_Slab_is_null(&new_item->prev_p));
			al_Ptr_Slab_set(&self->first_p,new_item);
			al_Ptr_Slab_set(&self->last_p,new_item);
		}
		else {
			al_Slab* last = al_Ptr_Slab_get_not_null(&self->last_p);
			al_Ptr_Slab_set(&last->next_p,new_item);
			al_Ptr_Slab_set(&new_item->prev_p,last);
			al_Ptr_Slab_set(&self->last_p,new_item);
		}
	}
	else { // assume at is a valid entry in the list
		al_Slab* item_p = al_Ptr_Slab_get(&at->prev_p);
		al_Ptr_Slab_set(&new_item->prev_p,item_p);
		if (item_p) {
			al_Ptr_Slab_set(&item_p->next_p,new_item);
		}
		else {
			al_Ptr_Slab_set(&self->first_p,new_item);
		}
		al_Ptr_Slab_set(&new_item->next_p,at);
		al_Ptr_Slab_set(&at->prev_p,new_item);
	}
}

internal void
al_SlabList_remove(al_SlabList *self, al_Slab *item)
{
	al_Slab* item_p = al_Ptr_Slab_get(&item->prev_p);
	al_Slab* item_n = al_Ptr_Slab_get(&item->next_p);

	if (item_n) {
		al_Ptr_Slab_set(&item_n->prev_p,item_p);
		al_Ptr_Slab_set_null(&item->next_p);
	}
	else {
		al_Ptr_Slab_set(&self->last_p,item_p);
	}

	if (item_p) {
		al_Ptr_Slab_set(&item_p->next_p,item_n);
		al_Ptr_Slab_set_null(&item->prev_p);
	}
	else {
		al_Ptr_Slab_set(&self->first_p,item_n);
	}
}

//------------------------------------------------------------------------------
// CacheList boilerplate code
//------------------------------------------------------------------------------

internal al_Cache*
al_CacheList_first(al_CacheList *self)
{
	return al_Ptr_Cache_get(&self->first_p);
}

internal al_Cache*
al_CacheList_last(al_CacheList *self)
{
	return al_Ptr_Cache_get(&self->last_p);
}

internal b8
al_CacheList_is_empty(al_CacheList *self)
{
	return al_Ptr_Cache_is_null(&self->first_p);
}

internal void
al_CacheList_init(al_CacheList *self)
{
	al_Ptr_Cache_set_null(&self->first_p);
	al_Ptr_Cache_set_null(&self->last_p);
}

internal void
al_CacheList_insert(al_CacheList *self, al_Cache *new_item, al_Cache *at)
{
	if (!at) { // if at == nullptr append
		if (al_CacheList_is_empty(self)) {
			Assert(al_Ptr_Cache_is_null(&new_item->next_p));
			Assert(al_Ptr_Cache_is_null(&new_item->prev_p));
			al_Ptr_Cache_set(&self->first_p,new_item);
			al_Ptr_Cache_set(&self->last_p,new_item);
		}
		else {
			al_Cache* last = al_Ptr_Cache_get_not_null(&self->last_p);
			al_Ptr_Cache_set(&last->next_p,new_item);
			al_Ptr_Cache_set(&new_item->prev_p,last);
			al_Ptr_Cache_set(&self->last_p,new_item);
		}
	}
	else { // assume at is a valid entry in the list
		al_Cache* item_p = al_Ptr_Cache_get(&at->prev_p);
		al_Ptr_Cache_set(&new_item->prev_p,item_p);
		if (item_p) {
			al_Ptr_Cache_set(&item_p->next_p,new_item);
		}
		else {
			al_Ptr_Cache_set(&self->first_p,new_item);
		}
		al_Ptr_Cache_set(&new_item->next_p,at);
		al_Ptr_Cache_set(&at->prev_p,new_item);
	}
}

internal void
al_CacheList_remove(al_CacheList *self, al_Cache *item)
{
	al_Cache* item_p = al_Ptr_Cache_get(&item->prev_p);
	al_Cache* item_n = al_Ptr_Cache_get(&item->next_p);

	if (item_n) {
		al_Ptr_Cache_set(&item_n->prev_p,item_p);
		al_Ptr_Cache_set_null(&item->next_p);
	}
	else {
		al_Ptr_Cache_set(&self->last_p,item_p);
	}

	if (item_p) {
		al_Ptr_Cache_set(&item_p->next_p,item_n);
		al_Ptr_Cache_set_null(&item->prev_p);
	}
	else {
		al_Ptr_Cache_set(&self->first_p,item_n);
	}
}

//------------------------------------------------------------------------------
// al_PageBlock
//------------------------------------------------------------------------------

internal inline void
al_PageBlock_init(al_PageBlock* self, u32 base, u32 page_size)
{
	self->page_index = base;
	self->page_size = page_size;
}

internal inline u64
al_PageBlock_bytes(al_PageBlock* self)
{
	return (u64) self->page_size * (u64) al_PAGE_SIZE;
}


//------------------------------------------------------------------------------
// al_PageBlock
//------------------------------------------------------------------------------

internal inline u32
al_pagemap_index_level1(u32 p) {
	return p >> al_PAGEMAP_LEVEL2_BITS;
}

internal inline u32
al_pagemap_index_level2(u32 p) {
	return p & al_PAGEMAP_LEVEL2_MASK;
}

//------------------------------------------------------------------------------
// Allocator related services: stand-alone ones
//------------------------------------------------------------------------------

internal inline void*
al_Allocator_page_to_ptr(al_Allocator *allocator, u32 index)
{
	/* there was a bug here before : oferflow on index * al_PAGE_SIZE */
	return (char*)allocator + al_PAGE_SIZE * (u64) index;
}

internal inline u32
al_Allocator_ptr_to_page(al_Allocator *self, void *p)
{
	return (u32)(((char*)p - (char*) self) >> al_BITS_PER_PAGE);
}

internal MemoryBlock
al_Allocator_memory_block(al_Allocator *self)
{
	MemoryBlock memblock;
	memblock.begin = (char*) self;
	memblock.end   = memblock.begin + (u64) self->used_pages * al_PAGE_SIZE;
	return memblock;
}

//
// WARNING(llins): user of this function should check if the requested number
// of pages was reserved. The reason is that:
//     if request does not fit, returns a page block with zero pages
//
internal al_PageBlock
al_Allocator_back_reserve(al_Allocator *self, u32 n)
{
	Assert(n > 0);
	al_PageBlock result;
	if (self->used_pages + (u64) n > self->page_capacity) {
		al_PageBlock_init(&result, 0, 0);
		return result;
	} else {
		u32 base = self->used_pages;
		self->used_pages += n;
		al_PageBlock_init(&result, base, n);
		return result;
	}
}

//------------------------------------------------------------------------------
// al_PageMap_Level2
//------------------------------------------------------------------------------

internal void
al_PageMap_Level2_init(al_PageMap_Level2* self)
{
	// should we reset all slabs?
	for (int i = 0; i < al_PAGEMAP_LEVEL2_ENTRIES; ++i) {
		al_Ptr_Slab_set_null(&self->slabs[i]);
	}
}

//------------------------------------------------------------------------------
// PageMap_Level1
//------------------------------------------------------------------------------

internal void
al_PageMap_Level1_assign(al_PageMap_Level1 *self, al_Allocator* allocator, u32 index, al_Slab* slab)
{
	//
	// index is an offset into the Allocator in number of pages.
	// Since page 0 is reserved to the Allocator itself it is
	// used here as a special flag to indicate this this Level1
	// page range is untouched
	//
	if (self->page != al_NULL_PAGE) {
		al_PageMap_Level2* level2_nodes = (al_PageMap_Level2*) al_Allocator_page_to_ptr(allocator, self->page);

		if (al_Ptr_Slab_is_null(&level2_nodes->slabs[index]))
			++self->used_pages;

		al_Ptr_Slab_set(&level2_nodes->slabs[index],slab);
	} else {
		// this should initialize correctly setting null pointers on all pages
		al_PageBlock block = al_Allocator_back_reserve(allocator, al_PAGEMAP_LEVEL2_PAGES);
		if (block.page_size == 0) {
			Assert(0);
		}

		al_PageMap_Level2* level2_nodes = (al_PageMap_Level2*) al_Allocator_page_to_ptr(allocator, block.page_index);

		// @todo: in principle we could avoid initializing level2 entries
		// since they only should participate when an existing object is freed
		al_PageMap_Level2_init(level2_nodes);

		al_Ptr_Slab_set(&level2_nodes->slabs[index],slab);

		self->page = block.page_index;
		++self->used_pages;
	}
}

internal al_Slab*
al_PageMap_Level1_get(al_PageMap_Level1 *self, al_Allocator* allocator, u32 index)
{
	if (!self->page) {
		return 0;
	} else {
		// this should initialize correctly setting null pointers on all pages
		al_PageMap_Level2* level2_nodes = (al_PageMap_Level2*)al_Allocator_page_to_ptr(allocator, self->page);
		return al_Ptr_Slab_get(&level2_nodes->slabs[index]);
	}
}

//------------------------------------------------------------------------------
// PageMapCursor
//------------------------------------------------------------------------------

typedef struct
{
	u32 index1;
	u32 index2;
} al_PageMap_Cursor;

internal void
al_PageMap_Cursor_init(al_PageMap_Cursor *self, u32 page_id)
{
	self->index1 = al_pagemap_index_level1(page_id);
	self->index2 = al_pagemap_index_level2(page_id);
}

internal inline void
al_PageMap_Cursor_next(al_PageMap_Cursor *self)
{
	++self->index2;
	if (self->index2 >= al_PAGEMAP_LEVEL2_ENTRIES) {
		++self->index1;
		self->index2 = 0;
		Assert(self->index1 < al_PAGEMAP_LEVEL1_ENTRIES);
	}
}

//------------------------------------------------------------------------------
// Allocator related services: stand-alone ones
//------------------------------------------------------------------------------

internal al_Slab*
al_Allocator_slab_of(al_Allocator *self, void *p)
{
	al_PageMap_Cursor cursor;
	al_PageMap_Cursor_init(&cursor, al_Allocator_ptr_to_page(self, p));
	al_PageMap_Level1* node_level1 = &self->pagemap.level1_nodes[cursor.index1];
	return al_PageMap_Level1_get(node_level1, self, cursor.index2);
}

//------------------------------------------------------------------------------
// PageMap_Iterator
//------------------------------------------------------------------------------

typedef struct al_PageMap_Iterator
{
	al_PageMap_Cursor cursor;
	u32       remaining;
	u32            items_consumed;
} al_PageMap_Iterator;

internal inline void
al_PageMap_Iterator_init(al_PageMap_Iterator *self, al_PageBlock* block)
{
	al_PageMap_Cursor_init(&self->cursor, block->page_index);
	self->remaining = block->page_size;
	self->items_consumed = 0;
}

internal inline b8
al_PageMap_Iterator_next(al_PageMap_Iterator *self)
{
	if (self->items_consumed) {
		if (self->remaining > 1) {
			al_PageMap_Cursor_next(&self->cursor); // don't go out of range
			--self->remaining;
			return 1;
		}
		else {
			self->remaining = 0;
			return 0;
		}
	}
	else {
		self->items_consumed = 1;
		return self->remaining > 0;
	}
}

//------------------------------------------------------------------------------
// Allocator related services: stand-alone ones
//------------------------------------------------------------------------------

internal void
al_Allocator_tag_pagemap(al_Allocator *self, al_Slab* slab)
{
	al_PageMap_Iterator iter;
	al_PageMap_Iterator_init(&iter, &slab->block);
	while (al_PageMap_Iterator_next(&iter)) {
		al_PageMap_Level1* node_level1 = &self->pagemap.level1_nodes[iter.cursor.index1];
		al_PageMap_Level1_assign(node_level1, self, iter.cursor.index2, slab);
	}
	// std::cout << "Allocator::pagemap_tag -> wrote on " << i << " page map slots " << std::endl;
}



//------------------------------------------------------------------------------
// Slab
//------------------------------------------------------------------------------

internal u64
al_Slab_capacity(al_Slab *self)
{
	return al_PageBlock_bytes(&self->block) / al_Ptr_Cache_get(&self->cache_p)->chunk_size;
}

internal void
al_Slab_init(al_Slab *self, al_Cache* cache, al_PageBlock block, s64 free_chunk_offset)
{

// 	al_PageBlock block; ok
// 	u64 used; ok
// 	al_Ptr_Cache cache_p; ok
// 	al_Ptr_Slab  prev_p; ok
// 	al_Ptr_Slab  next_p; ok
// 	al_Ptr_char  free_chunks; ok

	// allocator is the reference; use the offset ptr explicity and avoid
	// non locality of having to bring allocator address every time we need
	// a new slot

	// go through all the block memory writing an Ptr<char> to the next
	// free object

	al_Ptr_Cache_set(&self->cache_p,cache);
	self->block = block;
	self->used  = free_chunk_offset;

	u64 chunk_size = cache->chunk_size;

	//
	// prepare single linked list of free objects
	// the free chunk offset is used in the bootstrap initialization
	//
	Assert(al_Ptr_Allocator_is_not_null(&cache->allocator_p));
	al_Allocator* allocator = al_Ptr_Allocator_get_not_null(&cache->allocator_p);

	u64 capacity    = al_PageBlock_bytes(&self->block) / chunk_size;
	u64 free_chunks = capacity - free_chunk_offset;

	// first free chunk is the first chunk_size bytes on the target page block
	char* it = (char*) al_Allocator_page_to_ptr(allocator, block.page_index);
	it += free_chunk_offset * chunk_size;
	al_Ptr_char* slot = &self->free_chunks;
	for (u64 i = 0; i < free_chunks; ++i) {
		// std::cout << slot << " <- " << (void*) p << std::endl;
		al_Ptr_char_set(slot, it);
		slot = (al_Ptr_char*) it;
		it += chunk_size;
	}
	al_Ptr_char_set_null(slot); // null (no more free ptrs)

	// insert self into cache slabs
	al_Ptr_Slab_set_null(&self->next_p);
	al_Ptr_Slab_set_null(&self->prev_p);

}


internal b8
al_Slab_is_full(al_Slab *self)
{
	return al_Ptr_char_is_null(&self->free_chunks);
}

internal void*
al_Slab_alloc(al_Slab *self)
{
	Assert(al_Ptr_char_is_not_null(&self->free_chunks));
	char* result = al_Ptr_char_get_not_null(&self->free_chunks);
	al_Ptr_char_set(&self->free_chunks, al_Ptr_char_get((al_Ptr_char*) result));
	++self->used;
	return result;
}

internal void
al_Slab_free(al_Slab *self, void *p)
{
	al_Ptr_char* slot = (al_Ptr_char*) p;
	al_Ptr_char_set(slot, al_Ptr_char_get(&self->free_chunks));
	al_Ptr_char_set(&self->free_chunks, (char*)p);
	--self->used;
}

internal void*
al_Slab_chunk_at(al_Slab *self, u64 index)
{
	al_Cache     *cache     = al_Ptr_Cache_get(&self->cache_p);
	al_Allocator *allocator = al_Ptr_Allocator_get(&cache->allocator_p);
	char *base = (char*) al_Allocator_page_to_ptr(allocator, self->block.page_index);
	return base + index * cache->chunk_size;
}

//------------------------------------------------------------------------------
// Cache
//------------------------------------------------------------------------------

internal void
al_Cache_init(al_Cache *self, al_Allocator* allocator, u64 chunk_size, const char* name, u32 flags)
{
	pt_fill((char*) self, (char*) self + sizeof(al_Cache), 0);

	al_Ptr_Allocator_set(&self->allocator_p, allocator);
	self->used_chunks    = 0;
	self->chunk_capacity = 0;
	self->chunk_size     = chunk_size;
	self->flags          = flags;
	self->pages          = 0;

	al_Ptr_Cache_set_null(&self->prev_p);
	al_Ptr_Cache_set_null(&self->next_p);

	al_SlabList_init(&self->free_slabs);
	al_SlabList_init(&self->full_slabs);

	// @todo clean this ugly copy of a nil-terminated string
	char* it  = self->name;
	char* end = it + sizeof(self->name); // leave one character for zero
	Assert(it != end);
	while (*name != 0 && it != end - 1) {
		*it = *name;
		++name;
		++it;
	}
	while (it != end) { *it = 0; ++it; } // clear one or more remaining name entries
}

internal void
al_Cache_insert_slab(al_Cache *self, al_Slab *slab)
{
	self->pages          += slab->block.page_size;
	self->chunk_capacity += al_Slab_capacity(slab);
	self->used_chunks    += slab->used;
	if (!al_Slab_is_full(slab)) {
		al_SlabList_insert(&self->free_slabs, slab, 0);
	} else {
		al_SlabList_insert(&self->full_slabs, slab, 0);
	}
}

internal u32
al_Cache_more_pages(al_Cache *self)
{
	u64 extra_bytes_needed = self->chunk_size * (self->pages ? ((self->chunk_capacity + 1) / 2) : 1);
	return (u32) ((extra_bytes_needed + al_PAGE_SIZE - 1) / al_PAGE_SIZE);
}

internal void*
al_Cache_alloc(al_Cache *self)
{
	if (!al_SlabList_is_empty(&self->free_slabs)) {

		//
		// there is at least one chunk available
		// in a cache slab somewhere
		//

		al_Slab* slab   = al_SlabList_first(&self->free_slabs);

		void* result = al_Slab_alloc(slab);
		++self->used_chunks;

		if (al_Slab_is_full(slab)) {
			al_SlabList_remove(&self->free_slabs, slab);
			al_SlabList_insert(&self->full_slabs, slab, 0);
		}
		return result;
	} else {
		// TODO(llins) this assertion should be true, but it is not
		Assert(self->used_chunks == self->chunk_capacity);
		//
		// no chunk is available
		// needs new slab
		//

		al_Allocator* allocator = al_Ptr_Allocator_get_not_null(&self->allocator_p); // assuming it is not null

		u32 new_slab_pages = al_Cache_more_pages(self);

		// @Todo make a page cache
		al_PageBlock block = al_Allocator_back_reserve(allocator, new_slab_pages);
		if (block.page_size == 0) {
			u32 min_pages = (self->chunk_size + al_PAGE_SIZE - 1) / al_PAGE_SIZE;
#if 0
			/* pressure is low */
			new_slab_pages = min_pages;
			block = al_Allocator_back_reserve(allocator, new_slab_pages);
			if (block.page_size == 0) {
				Assert(0);
			}
#else
			/* try smaller fractions */
			new_slab_pages /= 5;
			new_slab_pages = MAX(new_slab_pages, min_pages);
			while (new_slab_pages >= min_pages) {
				block = al_Allocator_back_reserve(allocator, new_slab_pages);
				if (block.page_size != 0) {
					break;
				} else if (new_slab_pages > min_pages) {
					new_slab_pages /= 5;
					new_slab_pages = MAX(new_slab_pages, min_pages);
				} else {
					// TODO(llins) should we return 0? at the moment assume
					// program responsibility to avoid this situation.
					Assert(0);
				}
			}
#endif
		}

		// create new slab
		al_Slab *new_slab;

		if ((self->flags & al_FLAG_SLAB_CACHE) == 0) {
			// call to allocate new slab on the slab allocaor
			new_slab = (al_Slab*) al_Cache_alloc(al_Ptr_Cache_get_not_null(&allocator->slabs_cache_p));
			al_Slab_init(new_slab, self, block, 0); // no offset
		} else {
			// offset of 1 indicates one chunk is already being used in this new slab
			new_slab = (al_Slab*) al_Allocator_page_to_ptr(allocator, block.page_index);
			al_Slab_init(new_slab, self, block, 1);
		}

		al_Cache_insert_slab(self, new_slab);

		al_Allocator_tag_pagemap(allocator, new_slab);

		void* result = al_Slab_alloc(new_slab);
		++self->used_chunks;

		if (al_Slab_is_full(new_slab)) {
			al_SlabList_remove(&self->free_slabs, new_slab);
			al_SlabList_insert(&self->full_slabs, new_slab, 0);
		}

		return result;
	}
}

internal void
al_Cache_free(al_Cache *self, void* p)
{
	if (!p)
		return;

	// find slab from the page map
	al_Slab* slab = al_Allocator_slab_of(al_Ptr_Allocator_get_not_null(&self->allocator_p), p);

	// check if the slab is from the proper cache
	Assert(slab);

	/*
	 * @BUG(Y): running test with 2 bits, 7 levels, 2 dim
	 * at around 22% (823MB) this assertion failed
	 */
	Assert(al_Ptr_Cache_get_not_null(&slab->cache_p) == self);

	--self->used_chunks;

	//
	b8 slab_was_full = al_Slab_is_full(slab); // slab->full();
	al_Slab_free(slab, p);
	if (slab_was_full) {
		al_SlabList_remove(&self->full_slabs, slab);
		al_SlabList_insert(&self->free_slabs, slab, 0);
	}
}


/*
 * Finds the slab with the largest number of slots in a non full slab.
 * This number is useful to iterate through all the occupied slots.
 */
internal u64
al_Cache_max_slots_in_nonfull_slab(al_Cache *self)
{
	u64 result = 0;
	al_Slab *it  = al_Ptr_Slab_get(&self->free_slabs.first_p);
	while (it) {
		result = MAX(result, al_Slab_capacity(it));
		it = al_Ptr_Slab_get(&it->next_p);
	}
	return result;
}

//------------------------------------------------------------------------------
// al_Allocator
//------------------------------------------------------------------------------

internal al_Allocator*
al_Allocator_new(char *begin, char *end)
{

	// cannot create allocator on a PAGE misaligned address
	Assert(((u64) begin % al_PAGE_SIZE) == 0);

	// round to max page capacity
	u32 page_capacity = (u32)((end - begin)/ al_PAGE_SIZE);

	// check if allocator record fits
	u64 s = sizeof(al_Allocator);
	Assert((u64) page_capacity * al_PAGE_SIZE >= s);

	al_Allocator* self = (al_Allocator*) begin;

	u32 pages0 = ((s + al_PAGE_SIZE - 1) / al_PAGE_SIZE);
	pt_fill((char*) self, (char*) self + pages0 * al_PAGE_SIZE, 0);

	self->page_capacity = page_capacity;
	self->used_pages    = pages0;

	// make sure allocator is all zeroed

	//
	// bootstrap
	//

	//
	// note that back reserve only checks the page capacity
	// and used pages fields of an Allocator
	// the other fields here are unitialized
	//

	al_PageBlock slab_caches_block = al_Allocator_back_reserve(self, 1); // one page slab for cache objects
	al_PageBlock slab_slabs_block  = al_Allocator_back_reserve(self, 1); // one page slab for slab  objects

	al_Cache* cache_caches = (al_Cache*) al_Allocator_page_to_ptr(self, slab_caches_block.page_index); // first object in the caches slab is a cache of caches
	al_Cache* cache_slabs  = cache_caches + 1;                     // second object in the caches slab is a cache of slabs

	// initialize the caches
	al_Cache_init(cache_caches, self, sizeof(al_Cache), "Cache", al_FLAG_CACHE_CACHE);
	al_Cache_init(cache_slabs, self, sizeof(al_Slab), "Slab", al_FLAG_SLAB_CACHE);

	al_Slab* slab_caches = (al_Slab*) al_Allocator_page_to_ptr(self, slab_slabs_block.page_index);
	al_Slab* slab_slabs  = slab_caches + 1;

	// slabs
	al_Slab_init(slab_caches, cache_caches, slab_caches_block, 2);
	al_Slab_init(slab_slabs,  cache_slabs,  slab_slabs_block,  2);

	// insert initialized slabs into caches
	al_Cache_insert_slab(cache_caches, slab_caches);
	al_Cache_insert_slab(cache_slabs,  slab_slabs);

	//
	al_CacheList_init(&self->caches);
	al_CacheList_insert(&self->caches, cache_caches, 0);
	al_CacheList_insert(&self->caches, cache_slabs, 0);

	// update pagemap
	al_Allocator_tag_pagemap(self, slab_caches);
	al_Allocator_tag_pagemap(self, slab_slabs);

	// direct pointers to the caches and slabs
	al_Ptr_Cache_set(&self->caches_cache_p,cache_caches);
	al_Ptr_Cache_set(&self->slabs_cache_p,cache_slabs);

#if 0
	// simulate inserting 100 more slabs
	Print *print = &debug_request->print;
	for (s32 i=0;i<100;++i) {
		void *p = al_Cache_alloc(cache_slabs);
		Print_clear(print);
		Print_u64(print, (u64) p);
		Print_cstr(print, "\n");
		Request_print(debug_request, print);
	}
#endif
	return self;
}

// A block of memory in the first page, 8 bytes
// aligned that can be used to write some custom
// data. The plan is to use it to watermark the
// version of the code that created the allocator
internal MemoryBlock
al_Allocator_watermark_area(al_Allocator *self)
{
	u64 size0 = RALIGN(sizeof(al_Allocator), al_PAGE_SIZE);
	u64 watermark_offset = RALIGN(size0 - sizeof(al_Allocator),8);
	char *begin = (char*) self + watermark_offset;
	char *end   = (char*) self + size0;
	return (MemoryBlock) { .begin = begin, .end = end };
}

internal void
al_Allocator_fit(al_Allocator *self)
{
	self->page_capacity = self->used_pages;
}

internal b8
al_Allocator_resize(al_Allocator *self, u64 new_size)
{
	u64 new_num_pages = new_size / al_PAGE_SIZE;
	if (new_num_pages >= self->used_pages) {
		self->page_capacity = new_num_pages;
		return 1;
	} else {
		return 0;
	}
}

internal al_Cache*
al_Allocator_create_cache(al_Allocator *self, const char* name, u64 chunk_size)
{
	Assert(chunk_size >= al_MIN_CACHE_CHUNK_SIZE);
	al_Cache* cache = (al_Cache*) al_Cache_alloc(al_Ptr_Cache_get_not_null(&self->caches_cache_p));
	al_Cache_init(cache, self, chunk_size, name, al_FLAG_NORMAL_CACHE);
	al_CacheList_insert(&self->caches, cache, 0);
	return cache;
}

internal u64
al_Allocator_used_memory(al_Allocator *self)
{
	return (u64) self->used_pages * al_PAGE_SIZE;
}

internal u64
al_Allocator_capacity(al_Allocator *self)
{
	return (u64) self->page_capacity * al_PAGE_SIZE;
}

internal void
al_Allocator_set_root(al_Allocator *self, void *root)
{
	u64  used_memory = al_Allocator_used_memory(self);
	char *begin = (char*) self;
	char *end   = begin + used_memory;
	char *root_char = (char*) root;
	Assert(begin <= root_char && root_char < end);
	al_Ptr_char_set(&self->root_p, root_char);
}

internal void*
al_Allocator_get_root(al_Allocator *self)
{
	return (void*) al_Ptr_char_get(&self->root_p);
}

/*
 * Iterate through all used chunks in a particular cache
 */

typedef struct {
	al_Cache  *cache;
	al_Slab   *slab;
	char      *chunk_base;
	u64       chunk_index;
	u64       chunk_capacity;
	b8        slab_is_full;
	char*     empty_flags_begin; /* used to flag occupancy */
	char*     empty_flags_end;
} al_IterCache;

/* assumes there is at least one used slot in this slab or it is null */
internal void
al_IterCache_prepare_slab(al_IterCache *self, al_Slab *slab, b8 is_full)
{
	if (slab) {
		/* there is at least one occupied chunk on this slide */
		self->slab           = slab;
		self->chunk_index    = 0;
		self->chunk_capacity = al_Slab_capacity(slab);
		self->slab_is_full   = is_full;
		self->chunk_base     = al_Slab_chunk_at(slab, 0);
		if (!is_full) {
			Assert(self->empty_flags_end-self->empty_flags_begin >= (s64)self->chunk_capacity);

			/* clear empty flags */
			pt_fill(self->empty_flags_begin,
				self->empty_flags_begin + self->chunk_capacity, 0);

			/* initialize buffer with empty slots */
			al_Ptr_char *it = &self->slab->free_chunks;
			for (;;) {
				char *p = al_Ptr_char_get(it);
				if (p) {
					s64 offset = (p - self->chunk_base);
					Assert(offset >= 0);
					Assert((offset % self->cache->chunk_size) == 0);
					s64 index  = offset / self->cache->chunk_size;
					*(self->empty_flags_begin + index) = 1;
					it = (al_Ptr_char*) p;
				} else {
					break;
				}
			}

			for (u64 i=0;i<self->chunk_capacity;++i) {
				if (*(self->empty_flags_begin + i) == 0) {
					self->chunk_index = i;
					break;
				}
			}

			Assert(self->chunk_index != self->chunk_capacity);

		}
	} else {
		/* no slabs abailable: done! */
		self->slab = 0;
		self->chunk_index = 0;
		self->chunk_capacity = 0;
		self->chunk_base = 0;
		self->slab_is_full = 0;
	}
}

//
internal void
al_IterCache_init(al_IterCache *self, al_Cache *cache, char *buffer_begin, char *buffer_end)
{
	Assert(buffer_begin <= buffer_end);

	self->cache = cache;
	self->empty_flags_begin = buffer_begin;
	self->empty_flags_end   = buffer_end;

	/* iterate through all the full slabs first */
	al_Slab *slab = al_Ptr_Slab_get(&cache->full_slabs.first_p);
	while (slab && slab->used == 0) {
		slab = al_Ptr_Slab_get(&slab->next_p);
	}

	if (slab) {
		al_IterCache_prepare_slab(self, slab, 1);
	} else {
		slab = al_Ptr_Slab_get(&cache->free_slabs.first_p);
		while (slab && slab->used == 0) {
			slab = al_Ptr_Slab_get(&slab->next_p);
		}
		al_IterCache_prepare_slab(self, slab, 0);
	}
}

internal void
al_IterCache_next_slab(al_IterCache *self)
{
	Assert(self->slab);

	al_Slab *next_slab = al_Ptr_Slab_get(&self->slab->next_p);
	while (next_slab && next_slab->used == 0) {
		next_slab = al_Ptr_Slab_get(&next_slab->next_p);
	}

	if (next_slab) {
		al_IterCache_prepare_slab(self, next_slab, self->slab_is_full);
	} else if (self->slab_is_full) {
		/* go to list of empty slabs */
		next_slab = al_Ptr_Slab_get(&self->cache->free_slabs.first_p);
		while (next_slab && next_slab->used == 0) {
			next_slab = al_Ptr_Slab_get(&next_slab->next_p);
		}
		al_IterCache_prepare_slab(self, next_slab, 0);
	} else {
		al_IterCache_prepare_slab(self, 0, 0);
	}
}

internal void*
al_IterCache_next(al_IterCache *self)
{
	if (self->slab == 0)
		return 0;

	Assert(self->chunk_index < self->chunk_capacity);

	void *result = self->chunk_base + self->chunk_index * self->cache->chunk_size;

	++self->chunk_index;

	Assert(self->chunk_index <= self->chunk_capacity);

	if (self->chunk_index == self->chunk_capacity) {
		/* go to next slab */
		al_IterCache_next_slab(self);

	} else if (!self->slab_is_full) {
		while (self->chunk_index < self->chunk_capacity) {
			if (*(self->empty_flags_begin + self->chunk_index) == 0) {
				break;
			} else {
				++self->chunk_index;
			}
		}
		Assert(self->chunk_index <= self->chunk_capacity);
		if (self->chunk_index == self->chunk_capacity) {
			al_IterCache_next_slab(self);
		}
	}
	return result;
}


