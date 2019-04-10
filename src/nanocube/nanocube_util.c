
#ifdef nanocube_util_UNIT_TEST
#include "../base/platform.c"
#include "../base/alloc.c"
#include "nanocube_index.c"
global_variable PlatformAPI platform;
#endif

//------------------------------------------------------------------------------
// String Manipulation
//------------------------------------------------------------------------------

static b8
nu_cstr_is_prefix_of(const char *cstr,
		     const char *text_begin,
		     const char *text_end)
{
	Assert(text_begin <= text_end && "is_prefix: invalid input pointer");
	while (*cstr && text_begin != text_end)
	{
		if (*cstr != *text_begin)
			return 0;
		++cstr;
		++text_begin;
	}
	return *cstr == 0;
}

static s64
nu_first_match_on_cstr_prefix_table(const char **table_cstr,
				    s32          table_begin,
				    s32          table_end,
				    const char  *text_begin,
				    const char  *text_end)
{
	Assert(table_begin >= 0 && table_begin <= table_end);
	for (s64 i=table_begin; i<table_end; ++i) {
		if (nu_cstr_is_prefix_of(*(table_cstr + i), text_begin, text_end)) {
			return i;
		}
	}
	return -1; // no match
}

//------------------------------------------------------------------------------
// BufferTokenizer
//------------------------------------------------------------------------------

typedef struct {
	char* buffer; // on initialization

	MemoryBlock text; // remaining text (note that text.end doesn't change)
	MemoryBlock token;

	u32    token_empty:1;
	u32    overflow:1;

	union {
		char separator;
		u32 _align1;
	};
} nu_BufferTokenizer;

static void
nu_BufferTokenizer_init(nu_BufferTokenizer *btok, char separator, char* begin, char *end)
{
	Assert(begin <= end && "BufferTokenizer invalid init begin > end");
	btok->buffer      = begin;
	btok->text.begin  = begin;
	btok->text.end    = end;
	btok->token.begin = begin;
	btok->token.end   = begin;
	btok->token_empty = 0;
	btok->overflow    = 0;
	btok->separator   = separator;
}

static b8
nu_BufferTokenizer_next(nu_BufferTokenizer *btok)
{
	if (btok->text.begin == btok->text.end)
		return 0;

	// search for token separator
	char* separator_pos = btok->text.begin;

	while (1) {

		for ( ; separator_pos < btok->text.end; ++separator_pos ) {
			if (*separator_pos == btok->separator) {
				break;
			}
		}

		if (separator_pos < btok->text.end) {            // found complete token in the buffer
			btok->token.begin  = btok->text.begin;       // token will go from [begin,end]
			btok->token.end    = separator_pos; // note that case begin < end might happen (empty token).
			btok->text.begin   = separator_pos + 1; // get read for the next

			btok->token_empty  = btok->token.begin == btok->token.end;

			return 1;
		}
		else {
			// read all available data and no more to load
			// signal a token with overflow
			btok->overflow    = 1;
			btok->token.begin = btok->text.begin;
			btok->token.end   = btok->text.end; // consume everything

			btok->text.begin  = btok->text.end;
			btok->token_empty = btok->token.begin == btok->token.end;

			return 1; // generated last token: any subsequent call to next should be zero
		}

	}
}

//
// can detect truncation checking token begin and end
// return number of bytes written to the "buffer"
//
static u64
nu_BufferTokenizer_copy_token(nu_BufferTokenizer* btok,
			      char *buffer,
			      u64 capacity)
{
	u64 n = (u64) (btok->token.end - btok->token.begin);
	if (n > capacity) n = capacity;
	u64 ii = (u64) btok->token.begin;
	for (u64 i=0;i<n;++i) {
		*(buffer + i) = btok->buffer[ii++];
	}
	return n;
}

//------------------------------------------------------------------------------
// FileTokenizer
//------------------------------------------------------------------------------

typedef struct {
	pt_File     *pfh;

	u64         file_offset; // file offset of begin

	/* buffered content, has to be larger than the largest token to work */
	MemoryBlock buffer;

	/* buffer content not processed */
	MemoryBlock text;

	/* next token */
	MemoryBlock token;

	u32         file_done:1;    // file_done == 1 if no more data in file
	u32         token_empty:1;  // token_empty == 1  token_begin > token_end
	u32         overflow:1;     // overflow == 1 if last token had no separator it was end of file

	char        separator;
} nu_FileTokenizer;

//
// Assumes buffer is big enough to fit the largest token that will
// appear throughout read process
//
static void
nu_FileTokenizer_init(nu_FileTokenizer* self, char  sep, char *buffer_begin, char *buffer_end, pt_File *input_file)
{
	pt_fill((char*)self, (char*)self + sizeof(nu_FileTokenizer), 0);
	self->pfh = input_file;

	Assert(self->pfh->handle && self->pfh->read && self->pfh->open);

	self->buffer.begin = buffer_begin;
	self->buffer.end   = buffer_end;
	platform.read_next_file_chunk(self->pfh, self->buffer.begin, self->buffer.end);
	self->text.begin  = self->buffer.begin;
	self->text.end    = self->buffer.begin + self->pfh->last_read;
	self->token.begin = self->buffer.begin;
	self->token.end   = self->buffer.begin;
	self->file_done   = (u32) self->pfh->eof;
	self->overflow    = 0;
	self->separator   = sep;
	self->file_offset = 0;
}

// returns 0 if no more tokens, otherwise stores
// info to query token
static b8
nu_FileTokenizer_next(nu_FileTokenizer* self)
{
	if (self->text.begin == self->text.end && self->file_done)
		return 0;

	// search for token separator
	b8 round_one = 1;

	char* separator_pos = self->text.begin;

	for (;;) {

		for ( ; separator_pos != self->text.end; ++separator_pos ) {
			if (*separator_pos == self->separator) {
				break;
			}
		}

		if (separator_pos != self->text.end) {      // found complete token in the buffer
			self->token.begin  = self->text.begin;  // token will go from [begin,end]
			self->token.end    = separator_pos;     // note that case begin < end might happen (empty token).
			self->text.begin   = separator_pos + 1; // get read for the next
			self->file_offset += self->text.begin - self->token.begin;
			self->token_empty  = self->token.begin == self->token.end;
			return 1;
		} else if (self->file_done) {
			// read all available data and no more
			// characters to load signal a token
			// signal an overflow for the last token
			// to indicate it came with an eof
			self->overflow    = 1;
			self->token.begin = self->text.begin;
			self->token.end   = self->text.end; // consume everything

			self->text.begin  = self->text.end; // no more tokens will be available
			self->token_empty = self->token.begin == self->token.end;

			return 1; // generated last token: any subsequent call to next should be zero
		} else if (round_one) { // load more data

			// push current token to 0
			pt_rotate(self->buffer.begin, self->text.begin,
				  self->text.end);
			self->text.end   = self->buffer.begin
				+ (self->text.end - self->text.begin);
			self->text.begin = self->buffer.begin;
			separator_pos = self->text.end;

			// load more data from file
			platform.read_next_file_chunk(self->pfh, self->text.end, self->buffer.end);

			self->text.end += self->pfh->last_read;
			self->file_done = (u32) self->pfh->eof;

			round_one = 0;
		}
		else {
			Assert(0 && "FileTokenizer: found token larger than the buffer");
			return 0;
		}
	}
}



//------------------------------------------------------------------------------
// TokensArray
//------------------------------------------------------------------------------

typedef struct {
	b8           parse_overflow:1;
	b8           initialized:1;
	MemoryBlock *begin;
	MemoryBlock *end;
	MemoryBlock *capacity;
} nu_TokensArray;

static void
nu_TokensArray_init(nu_TokensArray *self, char *begin, char *end)
{
	self->begin    = (MemoryBlock*) begin;
	self->capacity = self->begin + (end - begin) / sizeof(MemoryBlock);
	self->end      = self->begin;
	self->initialized = 1;
	self->parse_overflow = 0;
}

static void
nu_TokensArray_split(nu_TokensArray *self, nu_TokensArray *target)
{
	Assert(self->initialized);
	target->begin    = self->begin;
	target->end      = self->end;
	target->capacity = self->capacity;
	target->initialized = 1;
	target->parse_overflow = self->parse_overflow;

	self->begin = self->end;
}

static s32
nu_TokensArray_find(nu_TokensArray *self, char *begin, char *end)
{
	s32 index = 0;
	MemoryBlock *it = self->begin;
	while (it != self->end) {
		if (cstr_compare_memory(begin, end, it->begin, it->end) == 0) {
			return index;
		} else {
			++index;
			++it;
		}
	}
	return -1;
}


static void
nu_TokensArray_parse(nu_TokensArray *self, char sep, char *text_begin, char *text_end)
{
	Assert(text_begin <= text_end);

	self->parse_overflow = 0;
	self->end = self->begin;

	char *it    = text_begin;
	char *begin = it;

	while (it != text_end) {
		if (*it == sep) {
			// new token
			if (self->end == self->capacity) {
				self->parse_overflow = 1;
				return;
			}
			self->end->begin = begin;
			self->end->end   = it;
			++self->end;
			++it;
			begin = it;
		} else {
			++it;
		}
	}

	if (self->end == self->capacity) {
		self->parse_overflow = 1;
	} else {
		self->end->begin = begin;
		self->end->end   = it;
		++self->end;
	}
}

//------------------------------------------------------------------------------
// log memory of a nanocube index
//------------------------------------------------------------------------------
static void
nu_log_memory(al_Allocator *allocator, nx_NanocubeIndex *nanocube_index, Print* print, b8 details, b8 print_empty_caches)
{
	//
	// list caches and their usage
	//
	{
		al_Cache* cache = al_Ptr_Cache_get(&allocator->caches.first_p);
		while (cache) {
			if (print_empty_caches || cache->used_chunks > 0) {
				print_format(print,"%24s | %5d len | %12lld use | %12lld cap | %8.1fMB mem | %5.1f%% muse | %7lld pages\n",
					     cache->name,
					     cache->chunk_size,
					     cache->used_chunks,
					     cache->chunk_capacity,
					     (f64) ((u64)cache->pages * al_PAGE_SIZE) / Megabytes(1),
					     (f64) (cache->chunk_capacity > 0 ? (100.0f * cache->used_chunks / cache->chunk_capacity) : 0.0f),
					     cache->pages);
			}

			cache = al_Ptr_Cache_get(&cache->next_p);
		}
	}

	if (details) {
		{
			al_Cache *inode_cache = al_Ptr_Cache_get(&nanocube_index->inode_cache_p);

			/* check max bytes needed for this iteration */

			u64 size = al_Cache_max_slots_in_nonfull_slab(inode_cache);
			print_cstr(print, "INodes by degree...\n");
			if (size > 0) {

				pt_Memory *mem = platform.allocate_memory(size, 0);

				al_IterCache iter;
				al_IterCache_init(&iter, inode_cache, OffsetedPointer(mem->base,0), OffsetedPointer(mem->base,mem->size));

				u64 degrees[257];
				pt_fill((char*) degrees, (char*) degrees + sizeof(degrees), 0);
				void *it;
				while ((it = al_IterCache_next(&iter))) {
					nx_INode *inode = (nx_INode*) it;
					s32 deg = nx_Node_degree(&inode->node);
					++degrees[deg];
				}

				for (s32 i=0;i<=256;++i) {
					if (degrees[i]) {
						print_cstr(print, "INodes with degree ");
						print_u64(print, (u64) i);
						print_align(print, 3, 1, ' ');
						print_cstr(print, " -> ");
						print_u64(print, degrees[i]);
						print_align(print, 13, 1, ' ');
						print_cstr(print, "\n");
					}
				}

				platform.free_memory(mem);
			}
		}
		{
			al_Cache *pnode_cache = al_Ptr_Cache_get(&nanocube_index->pnode_cache_p);

			/* check max bytes needed for this iteration */

			u64 size = al_Cache_max_slots_in_nonfull_slab(pnode_cache);
			print_cstr(print, "PNodes by degree...\n");

			if (size > 0) {

				pt_Memory *mem = platform.allocate_memory(size, 0);

				al_IterCache iter;
				al_IterCache_init(&iter, pnode_cache, OffsetedPointer(mem->base,0), OffsetedPointer(mem->base,mem->size));

				u64 degrees[257];
				pt_fill((char*) degrees, (char*) degrees + sizeof(degrees), 0);
				void *it;
				while ((it = al_IterCache_next(&iter))) {
					nx_PNode *pnode = (nx_PNode*) it;
					s32 deg = nx_Node_degree(&pnode->node);
					++degrees[deg];
				}

				for (s32 i=0;i<=256;++i) {
					if (degrees[i]) {
						print_cstr(print, "PNodes with degree ");
						print_u64(print, (u64) i);
						print_align(print, 3, 1, ' ');
						print_cstr(print, " -> ");
						print_u64(print, degrees[i]);
						print_align(print, 13, 1, ' ');
						print_cstr(print, "\n");
					}
				}

				platform.free_memory(mem);
			}
		}
	}

	{
		print_cstr(print, "Number of records: ");
		print_u64(print, nanocube_index->number_of_records);
		print_cstr(print, "\nAllocator total memory: ");
		MemoryBlock mb = al_Allocator_memory_block(allocator);
		print_u64(print, RAlign((mb.end - mb.begin),Megabytes(1))/ Megabytes(1));
		print_cstr(print, "MB\n");
	}

	//    fprintf(stderr, "Allocator total memory: %lldMB", Allocator_memory_block(allocator).size / Megabytes(1));

	//
	// Go through all PNodes and count number of leaves
	//
	//     {
	//         Assert(Ptr_Cache_is_not_null(&nanocube_index->pnode_cache_p));
	//         Cache* cache = Ptr_Cache_get_not_null(&nanocube_index->pnode_cache_p);
	//
	//         s32 num_d0 = 0;
	//         s32 num_d2_p0 = 0;
	//         s32 num_d2_p1g = 0; // 1g = one or greater
	//
	//         // full slabs is easy
	//         {
	//             Slab* slab = Ptr_Slab_get(&cache->full_slabs.first_p);
	//             while (slab) {
	//                 NumChunks n = Slab_capacity(slab);
	//                 char* base = (char*)Allocator_page_to_ptr(allocator, slab->block.page_index);
	//                 for (u64 i = 0; i < n; ++i) {
	//                     PNode* pnode = (PNode*) (base + i * cache->chunk_size);
	//                     if (pnode->node.degree == 0) {
	//                         ++num_d0;
	//                     }
	//                     else if (pnode->node.degree == 2 && pnode->node.path_length == 0) {
	//                         ++num_d2_p0;
	//                     }
	//                     else if (pnode->node.degree == 2 && pnode->node.path_length > 0) {
	//                         ++num_d2_p1g;
	//                     }
	//                 }
	//                 slab = Ptr_Slab_get(&slab->next_p);
	//             }
	//         }
	//         fprintf(stderr, "\n----------\n");
	//         fprintf(stderr, "Num PNodes with degree 0           in full slabs: %d\n", num_d0);
	//         fprintf(stderr, "Num PNodes with degree 2 path 0    in full slabs: %d\n", num_d2_p0);
	//         fprintf(stderr, "Num PNodes with degree 2 path >= 1 in full slabs: %d\n", num_d2_p1g);
	//
	//         //// supports up to 4094 * 8 slots
	//         //char free_slots[Kilobytes(4)];
	//
	//         //// iterate through full slabs first
	//         //alloc::Slab *slabs[2] = {
	//         //	cache->full_slabs.first_p.raw_null(),
	//         //	cache->free_slabs.first_p.raw_null()
	//         //};
	//         //
	//
	//         //for (auto i = 0; i < 2; ++i) {
	//         //	auto slab = slabs[i];
	//
	//         //	auto n = Slab_capacity(slab);
	//
	//         //	auto bytes_for_n_bits = (n + 7) / 8;
	//         //	Assert(bytes_for_n_bits <= sizeof(free_slots));
	//
	//         //	// everything is occupied (zero)
	//         //	for (auto i = 0; i < bytes_for_n_bits; ++i) {
	//         //		free_slots[i] = 0;
	//         //	}
	//
	//         //	// iterate through singly linked list
	//         //	// of free chunks
	//         //	auto slab_base = (char*) Allocator_page_to_ptr(allocator, slab->block.page_index);
	//         //
	//         //	char one = 1;
	//
	//         //	auto it = &slab->free_chunks;
	//         //	while (it->is_not_null()) {
	//         //		auto next = it->raw();
	//         //		auto index = (next - slab_base)/cache->chunk_size;
	//         //		write_bits(&one, index, 1, free_slots);
	//         //		it = (ptr::Ptr<char>*) next;
	//         //	}
	//
	//
	//
	//
	//         //	// slab->block.page_index
	//         //	// while (slab) {
	//         //	//	slab->
	//         //	//	slab = slab->next_p.raw_null();
	//         //	// }
	//         //}
	//     }

}

//--------------------------------------------------------------------------------
// Iterate through a nanocube index
//--------------------------------------------------------------------------------

#define nu_Iter_Max_Stack_Size 1000

typedef enum {
	nu_Iter_NONE,
	nu_Iter_NODE,
	nu_Iter_PARENT_CHILD_EDGE,
	nu_Iter_CONTENT_EDGE,
	nu_Iter_PAYLOAD_EDGE,
	nu_Iter_PAYLOAD
} nu_Iter_Item_Type;

typedef struct {
	nu_Iter_Item_Type type;
	nx_NodeWrap src;
	nx_NodeWrap dst;
	nx_PayloadAggregate *payload;
	// u64      record_set;
	b8       shared;
	s32      suffix_length;
} nu_Iter_Item;

typedef struct {
	nx_Node* node;
	s32   dimension;
} nu_Iter_E;

typedef struct {
	nx_NanocubeIndex          *hierarchy;
	nu_Iter_E    node_stack[nu_Iter_Max_Stack_Size];
	nu_Iter_Item item_stack[nu_Iter_Max_Stack_Size];
	nu_Iter_Item current;
	s32                     node_stack_size;
	s32                     item_stack_size;
} nu_Iter;

static inline void
nu_Iter_push_node(nu_Iter *self, nx_Node* node, s32 dimension)
{
	Assert(self->node_stack_size+1 < nu_Iter_Max_Stack_Size);
	nu_Iter_E *slot =
		self->node_stack + self->node_stack_size;
	slot->node = node;
	slot->dimension = dimension;
	++self->node_stack_size;
}

static void
nu_Iter_init(nu_Iter *self, nx_NanocubeIndex *h)
{
	self->hierarchy = h;
	self->node_stack_size = 0;
	self->item_stack_size = 0;
	self->current.type = nu_Iter_NONE;
	nu_Iter_push_node(self, nx_Ptr_Node_get(&h->root_p), 0);
}

static nu_Iter_Item*
nu_Iter_push_item(nu_Iter *self, nu_Iter_Item_Type type)
{
	Assert(self->item_stack_size + 1 < nu_Iter_Max_Stack_Size);
	nu_Iter_Item* item = self->item_stack + self->item_stack_size;
	item->type = type;
	++self->item_stack_size;
	return item;
}

static nu_Iter_Item*
nu_Iter_next(nu_Iter* self)
{
	for(;;) {

		if (self->item_stack_size) {
			self->current =
				self->item_stack[self->item_stack_size-1];
			--self->item_stack_size;
			return &self->current;
		}

		if (!self->node_stack_size) { return 0; } // done!

		nu_Iter_E* e =
			self->node_stack + self->node_stack_size - 1;
		s32   dim  = e->dimension;
		nx_Node* node = e->node;
		--self->node_stack_size; // pop node

		nx_NodeWrap node_w =
			nx_NanocubeIndex_to_node(self->hierarchy, node, dim);

		// push node into item stack
		nu_Iter_Item* item =
			nu_Iter_push_item(self, nu_Iter_NODE);

		// initialize pushed item
		item->src = node_w;

		for (s32 i=0;i<node_w.children.length;++i) {
			nx_Child* child_slot = nx_Children_get_child(&node_w.children,i);
			nx_NodeWrap child_w = nx_NanocubeIndex_to_node(self->hierarchy, nx_Child_get_node(child_slot), dim);

			item = nu_Iter_push_item(self, nu_Iter_PARENT_CHILD_EDGE);
			item->src = node_w;
			item->dst = child_w;
			item->shared = child_slot->shared;
			item->suffix_length = child_slot->suffix_length;

			if (!child_slot->shared) {
				nu_Iter_push_node(self, child_w.raw_node, dim);
			}
		}

		if (dim == self->hierarchy->dimensions-1) {
			item = nu_Iter_push_item(self, nu_Iter_PAYLOAD);

			nx_PayloadAggregate *payload = &((nx_PNode*)node)->payload_aggregate;
			item->payload = payload;

			item = nu_Iter_push_item(self, nu_Iter_PAYLOAD_EDGE);
			item->src = node_w;
			item->payload = payload;
		} else { // send content
			item = nu_Iter_push_item(self, nu_Iter_CONTENT_EDGE);
			item->src = node_w;
			nx_Node* content_node = nx_INode_content((nx_INode*) node);
			item->dst = nx_NanocubeIndex_to_node(self->hierarchy, content_node, dim+1);
			nu_Iter_push_node(self, content_node, dim+1);
		}
	}
}

//--------------------------------------------------------------------------------
// NodeDepth
//--------------------------------------------------------------------------------

#define nu_NodeDepth_Max_Dimensions 10

typedef struct {
	b8   largest;
	s32  length;
	s32  depth[nu_NodeDepth_Max_Dimensions];
} nu_NodeDepth;

static inline void
nu_NodeDepth_init_largest(nu_NodeDepth* self)
{
	self->largest = 1;
	self->length = 0;
	self->depth[0] = 0;
}

static void
nu_NodeDepth_init(nu_NodeDepth* self, nx_Node* node)
{
	self->largest = 0;
	self->length = 0;
	self->depth[0] = 0;
	while (node) {
		self->depth[self->length] += node->path_length;
		if (node->root) {
			Assert(self->length+1 < nu_NodeDepth_Max_Dimensions);
			++self->length;
			self->depth[self->length] = 0;
		}
		node = nx_Node_parent(node);
	}
}

static s32
nu_NodeDepth_depth_at_dimension(nu_NodeDepth *self, s32 dim)
{
	Assert(dim < self->length);
	return self->depth[self->length-1-dim];
}

static b8
nu_NodeDepth_is_equal(nu_NodeDepth *self, nu_NodeDepth *other)
{
	if (self->largest && other->largest) return 1;
	else if (self->length != other->length) return 0;
	else {
		return self->depth[0] == other->depth[0];
	}
}

static b8
nu_NodeDepth_is_smaller(nu_NodeDepth *self, nu_NodeDepth *other)
{
	if (self->largest) return 0;
	else if (other->largest) return 1;
	else if (self->length > other->length) return 0;
	else if (self->length < other->length) return 1;
	else {
		return self->depth[0] < other->depth[0];
	}
}

//--------------------------------------------------------------------------------
// NodeDepth_Ordered_Set
//--------------------------------------------------------------------------------

#define nu_NodeDepth_Ordered_Set_Max_Size 100
typedef struct
{
	nu_NodeDepth list[nu_NodeDepth_Ordered_Set_Max_Size];
	s32       order[nu_NodeDepth_Ordered_Set_Max_Size];
	s32       size;
}
nu_NodeDepth_Ordered_Set;

static inline void
nu_NodeDepth_Ordered_Set_init(nu_NodeDepth_Ordered_Set* self)
{
	self->size = 0;
}

static inline nu_NodeDepth*
nu_NodeDepth_Ordered_Set_at(nu_NodeDepth_Ordered_Set* self, s32 index)
{
	Assert(index < self->size);
	return self->list + self->order[index];
}

static s32
nu_NodeDepth_Ordered_Set_find(nu_NodeDepth_Ordered_Set* self,
			      nu_NodeDepth *item)
{
	if (self->size==0)
		return -1;
	s32 a = 0;
	s32 b = self->size;
	while (a + 1 < b) { // loop while 2 or more elements (a,b]
		s32 m = (b + a)/2;
		nu_NodeDepth* item_m = nu_NodeDepth_Ordered_Set_at(self,m);
		if (nu_NodeDepth_is_smaller(item, item_m)) { b = m; }
		else if (nu_NodeDepth_is_smaller(item_m,item)) { a = m+1; }
		else { return m; }
	}
	nu_NodeDepth* item_a = nu_NodeDepth_Ordered_Set_at(self,a);
	if (nu_NodeDepth_is_smaller(item, item_a)) { return -a - 1; }
	else if (nu_NodeDepth_is_smaller(item_a,item)) { return -(a+1) - 1; }
	else { return a; }
}

static void
nu_NodeDepth_Ordered_Set_insert(nu_NodeDepth_Ordered_Set* self,
				nu_NodeDepth *item)
{
	s32 index = nu_NodeDepth_Ordered_Set_find(self, item);

	if (index >= 0)
		return; // already inserted

	self->list[self->size] = *item;
	index = -index -1; // rotate order array so that last item becomes index-th item

	for (s32 i=self->size;i>index;--i) {
		self->order[i] = self->order[i-1];
	}
	self->order[index] = self->size;

	++self->size;
}

// std::ostream& operator<<(std::ostream& os, const NodeDepth& node_depth) {
//     if (node_depth._largest) os << "[largest]";
//     else {
//         os << "[";
//         for (auto i=0;i<node_depth.length();++i)
//             os << (i > 0 ? "," : " ") << node_depth[i];
//         os << "]";
//     }
//     return os;
// }

//--------------------------------------------------------------------------------
// save_dot_file
//--------------------------------------------------------------------------------

#define nu_Save_Dot_File_Max_Nodes 1000

#define nu_PRINT_PAYLOAD(name) \
	void (name)(Print *print, nx_PayloadAggregate *payload)
typedef nu_PRINT_PAYLOAD(nu_PrintPayloadFunc);

static u64
nu_save_dot_file_node_id(nx_NanocubeIndex* nanocube_index, void *p)
{
	return (u64) ((char*) p - (char*) nanocube_index);
}

static void
nu_save_dot_file(nx_NanocubeIndex* nanocube_index, char *filename_begin, char *filename_end, nu_PrintPayloadFunc *payload_print, b8 show_ids)
{
	pt_File pfh = platform.open_file(filename_begin, filename_end, pt_FILE_WRITE);

	char buffer[Kilobytes(4)];
	Print print;
	print_init(&print, buffer, sizeof(buffer));

	//     Assert(fp);
	print_cstr(&print, "digraph nested_hierarchy {\nrankdir=LR;\n");
	//     fprintf(fp,"digraph nested_hierarchy {\nrankdir=LR;\n");

	// dump
	platform.write_to_file(&pfh,print.begin,print.end);
	print_clear(&print);


	nu_Iter it;
	nu_Iter_Item *item;

	/* first pass: nodes */
	nu_Iter_init(&it, nanocube_index);
	while ((item = nu_Iter_next(&it))) {
		if (item->type == nu_Iter_NODE) {
			print_cstr(&print, "n");
			print_u64(&print, (u64) item->src.raw_node);
			print_cstr(&print, " [label=\"");

			if (show_ids) {
				print_format(&print, "id %llu\n", nu_save_dot_file_node_id(nanocube_index, item->src.raw_node));
			}

			if (item->src.path.length == 0) {
				print_char(&print, 'e');
			} else {
				for (u8 i=0; i<item->src.path.length; ++i) {
					if (i > 0) {
						print_char(&print, ',');
					}
					if (i > 0 && i % 10 == 0) {
						print_char(&print, '\n');
					}
					print_u64(&print, (u64) nx_Path_get(&item->src.path,i));
				}
			}
			print_cstr(&print, "\"];\n");
			//                 fprintf(fp,"\"];\n");
		} else if (item->type == nu_Iter_PAYLOAD) {
			// Assuming payload only shows once
			print_cstr(&print, "g");
			print_u64(&print, (u64) item->payload);
			print_cstr(&print, "[label=\"");
			(*payload_print)(&print, item->payload);
			print_cstr(&print, "\"];\n");
		}

		platform.write_to_file(&pfh,print.begin,print.end);
		print_clear(&print);

	}
	/* end: first pass: nodes */

	/* second pass: edges */
	nu_Iter_init(&it, nanocube_index);
	while ((item = nu_Iter_next(&it))) {
		if (item->type == nu_Iter_PARENT_CHILD_EDGE) {
			print_char(&print, 'n');
			print_u64(&print, (u64) item->src.raw_node);
			print_cstr(&print, " -> n");
			print_u64(&print, (u64) item->dst.raw_node);
			print_cstr(&print, " [dir=none penwidth=1.4 style=");
			print_cstr(&print, item->shared ? "dotted" : "solid");
			print_cstr(&print, "  label=\"");
			print_u64(&print, item->suffix_length);
			print_cstr(&print, "\"];\n");
			//                 fprintf(fp, "n%lld -> n%lld [dir=none penwidth=1.4 style=%s label=\"%d\"];",
			//                         (u64) item->src.raw_node,
			//                         (u64) item->dst.raw_node,
			//                         item->shared ? "dotted" : "solid",
			//                         item->suffix_length);

		}
		else if (item->type == nu_Iter_PAYLOAD_EDGE) {
			print_char(&print, 'n');
			print_u64(&print, (u64) item->src.raw_node);
			print_cstr(&print, " -> g");
			print_u64(&print, (u64) item->payload);
			print_cstr(&print, "  [dir=forward style=solid color=\"#80B1D3\"];\n");

			//                 fprintf(fp, "n%lld -> g%lld [dir=forward style=solid color=\"#80B1D3\"];\n",
			//                         (u64) item->src.raw_node,
			//                         (u64) item->record_set);

		}
		else if (item->type == nu_Iter_CONTENT_EDGE) {

			print_char(&print, 'n');
			print_u64(&print, (u64) item->src.raw_node);
			print_cstr(&print, " -> n");
			print_u64(&print, (u64) item->dst.raw_node);
			print_cstr(&print, "  [dir=forward style=solid color=\"#80B1D3\"];\n");

			//                 fprintf(fp, "n%lld -> n%lld [dir=forward style=solid color=\"#80B1D3\"];\n",
			//                         (u64) item->src.raw_node,
			//                         (u64) item->dst.raw_node);

		}

		platform.write_to_file(&pfh,print.begin,print.end);
		print_clear(&print);
	}
	/* end: second pass: edges */

	// good test for the back pointer
	{
		nu_NodeDepth_Ordered_Set ndset;
		nu_NodeDepth_Ordered_Set_init(&ndset);

		nu_NodeDepth nd;

		nu_NodeDepth_init_largest(&nd);
		nu_NodeDepth_Ordered_Set_insert(&ndset, &nd);

		s32 num_nodes = 0;
		nu_Iter_init(&it, nanocube_index);
		while ((item = nu_Iter_next(&it))) {
			if (item->type == nu_Iter_NODE) {
				nu_NodeDepth_init(&nd, item->src.raw_node);
				nu_NodeDepth_Ordered_Set_insert(&ndset, &nd);
				++num_nodes;
			}
		}

		Assert(num_nodes <= nu_Save_Dot_File_Max_Nodes);

		s32 ranks[nu_Save_Dot_File_Max_Nodes];
		u64 ids[nu_Save_Dot_File_Max_Nodes];

		{
			s32 i = 0;
			nu_Iter_init(&it, nanocube_index);
			while ((item = nu_Iter_next(&it))) {
				if (item->type == nu_Iter_NODE) {
					nu_NodeDepth_init(&nd,
							item->src.raw_node);
					s32 rank = nu_NodeDepth_Ordered_Set_find
						(&ndset, &nd);
					Assert(rank >= 0);
					ranks[i] = rank;
					ids[i] = (u64) item->src.raw_node;
					++i;
				}
			}
			Assert(i == num_nodes);
		}

		// create one dummy node for each level
		for (s32 i=0;i<ndset.size;++i) {
			print_cstr(&print, "dummy");
			print_u64(&print, (u64) i);
			print_cstr(&print, "[style=invis width=0.1 label=\"\"];\n");
			//             fprintf(fp, "dummy%d [style=invis width=0.1 label=\"\"];\n", i);

			platform.write_to_file(&pfh,print.begin,print.end);
			print_clear(&print);
		}


		print_cstr(&print, "dummy0");
		for (s32 i=1;i<ndset.size;++i) {
			print_cstr(&print, " -> dummy");
			print_u64(&print, (u64) i);
			platform.write_to_file(&pfh,print.begin,print.end);
			print_clear(&print);
		}
		print_cstr(&print, " [style=invis];\n");

		// @inefficient qudratic cost could be avoided here
		// but use case should be small enough
		for (s32 i=0;i<ndset.size-1;++i) {
			print_cstr(&print, "{ rank=same; dummy");
			print_u64(&print, (u64) i);
			for (s32 j=0;j<num_nodes;++j) {
				if (ranks[j] == i)
				{
					print_cstr(&print, ", n");
					print_u64(&print, (u64) ids[j]);
					platform.write_to_file(&pfh,print.begin,
							print.end);
					print_clear(&print);
				}
			}
			print_cstr(&print, " };\n");
		}

		// @inefficient qudratic cost could be avoided here
		// but use case should be small enough
		{
			print_cstr(&print, "{ rank=same; dummy");
			print_u64(&print, (u64) ndset.size-1);
			nu_Iter_init(&it, nanocube_index);
			while ((item = nu_Iter_next(&it))) {
				if (item->type == nu_Iter_PAYLOAD) {
					print_cstr(&print, ", g");
					print_u64(&print, (u64) item->payload);
					platform.write_to_file(&pfh,
							print.begin,print.end);
					print_clear(&print);
				}
			}
			print_cstr(&print, " };\n");
		}
	}

	print_cstr(&print, "}");

	platform.write_to_file(&pfh,print.begin,print.end);
	print_clear(&print);

	platform.close_file(&pfh);
}

//
// hierarchy odometer
//

typedef struct {
	u8  stack[128];
	u8  stack_size;
	u8  bits;
	s32 values[128];
} nu_HOdom;

static void
nu_HOdom_init(nu_HOdom *self, u8 bits)
{
	self->bits = bits;
	self->stack_size = 0;
}

static void
nu_HOdom_reset(nu_HOdom *self, u8 msb, u8 lsb)
{
	for (s32 i=msb;i<lsb;++i)
		self->values[i] = 0;
	self->values[lsb] = -1;
}

static void
nu_HOdom_prepare_odometer(nu_HOdom *self, u8 index, u8 *msb, u8 *lsb)
{
	if (self->stack_size == 0) {
		self->stack[self->stack_size] = index;
		++self->stack_size;
		*msb = 0;
		*lsb = index;
		nu_HOdom_reset(self, *msb, *lsb);
	} else {
		// assuming stack is not emtpy
		s32 top_index = self->stack[self->stack_size-1];
		if (index > top_index) {
			// new odometer and reset
			self->stack[self->stack_size] = index;
			++self->stack_size;
			*msb = top_index+1;
			*lsb = index;
			nu_HOdom_reset(self, *msb, *lsb);
		} else if (index == top_index) {
			if (self->stack_size > 1) {
				*msb = self->stack[self->stack_size-2];
			} else {
				*msb = 0;
			}
			*lsb = top_index;
		} else {
			// drop everything from the stack up to where
			// index is equal or greater keep the current digit on the odometer
			// the call to next will increment it
			b8 done = 0;
			while (self->stack_size > 0) {
				s32 current_top = self->stack[self->stack_size-1];
				if (current_top == index) {
					done = 1;
					break;
				} else if (current_top < index) {
					break;
				} else {
					--self->stack_size;
				}
			}
			if (!done) {
				self->stack[self->stack_size] = index;
				++self->stack_size;
			}
			if (self->stack_size > 1) {
				*msb = self->stack[self->stack_size-2]+1;
				*lsb = self->stack[self->stack_size-1];
			} else {
				*msb = 0;
				*lsb = self->stack[self->stack_size-1];
			}

		}
	}
//
// 	// msb = most significant digit location
// 	// lsb = least significant digit location
// 	Assert(self->stack_size > 0);
// 	if (self->stack_size == 1) {
// 		*msb = 0;
// 		*lsb   = self->stack[self->stack_size-1];
// 	} else {
// 		*msb = self->stack[self->stack_size-2];
// 		*lsb = self->stack[self->stack_size-1];
// 	}
}

static b8
nu_HOdom_next(nu_HOdom *self, u8 msb, u8 lsb)
{
	s32 index = lsb;
	s32 max_digit = (1 << self->bits) - 1;
	while (index >= msb) {
		++self->values[index];
		if (self->values[index] > max_digit) {
			self->values[index] = 0;
			--index;
		} else {
			break;
		}
	}
	if (index < msb) {
		// overflow
		return 0;
	} else {
		return 1;
	}
}

static b8
nu_HOdom_advance(nu_HOdom *self, u8 index)
{
	u8 msb = 0;
	u8 lsb  = 0;
	nu_HOdom_prepare_odometer(self, index, &msb, &lsb);
	return nu_HOdom_next(self, msb, lsb);
}





#ifdef nanocube_util_UNIT_TEST

#include <stdio.h>
#include <stdlib.h>

static void
advance_and_print(nu_HOdom *hodom, s32 index)
{
	nu_HOdom_advance(hodom,index);
	for (s32 i=0;i<=index;++i) {
		printf("%d ", hodom->values[i]);
	}
	printf("\n");
}

int main()
{
	nu_HOdom it;
	nu_HOdom_init(&it, 8);

	advance_and_print(&it,1);
	advance_and_print(&it,3);
	advance_and_print(&it,3);
	advance_and_print(&it,3);
	advance_and_print(&it,1);
	advance_and_print(&it,3);
	advance_and_print(&it,3);
	advance_and_print(&it,1);
	advance_and_print(&it,3);
	advance_and_print(&it,1);
	advance_and_print(&it,2);
	advance_and_print(&it,0);
	advance_and_print(&it,3);
	advance_and_print(&it,2);
	advance_and_print(&it,3);

	// expected output
	// 0 0
	// 0 0 0 0
	// 0 0 0 1
	// 0 0 0 2
	// 0 1
	// 0 1 0 0
	// 0 1 0 1
	// 0 2
	// 0 2 0 0
	// 0 3
	// 0 3 0
	// 1
	// 1 0 0 0
	// 1 0 1
	// 1 0 1 0
}

#endif

