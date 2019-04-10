typedef struct rg_Object  rg_Object;
typedef struct rg_Locate  rg_Locate;
typedef struct rg_Tag     rg_Tag;

/* max of 64k for tag value */
typedef ss_String rg_String;

#define rg_NODE_FILTER_CALLBACK(name) b8 name(rg_Locate *locate, void *data)
typedef rg_NODE_FILTER_CALLBACK(rg_LocateFilterCallback);

//------------------------------------------------------------------------------
// Ptr Types
//------------------------------------------------------------------------------

PTR_SPECIALIZED_TYPE(rg_Ptr_Locate);
PTR_SPECIALIZED_TYPE(rg_Ptr_Object);
PTR_SPECIALIZED_TYPE(rg_Ptr_String);
PTR_SPECIALIZED_TYPE(rg_Ptr_Tag);

/*
 * 1 st. pass only sets ID
 * 2 increments on number of incident locates happen
 *
 *
 */

#define rg_Locate_COUNTING_STATE  0
#define rg_Locate_APPENDING_STATE 1
#define rg_Locate_READY_STATE     2

struct rg_Locate {
	u64 id: 62;
	u64 state: 2;
	f32 lat;
	f32 lon;

	/* capacity is a function of the number of locates */
	u16 num_objects;

	/* ptr to list of locates incident to this locate */
	union {
		rg_Ptr_Object  singleton;
		al_Ptr_char    objects;
	};

	/* vantage point info for fast nearest neighbor queries */
	struct {
		rg_Ptr_Locate near;
		rg_Ptr_Locate far;
		f32         radius;
	} vp;

	/* algorithms might need to tag rg_Locate */
	union {
		void *custom_data;

		/* will use this temporarily when initializing these nodes */
		u64  custom_flags;
	};
};

#define	rg_OBJECT_UNDEFINED        0

// roads
#define	rg_OBJECT_motorway         1
#define	rg_OBJECT_trunk            2
#define rg_OBJECT_primary          3
#define rg_OBJECT_secondary        4
#define rg_OBJECT_tertiary         5
#define rg_OBJECT_unclassified     6
#define rg_OBJECT_residential      7
#define rg_OBJECT_service          8
#define rg_OBJECT_motorway_link    9
#define rg_OBJECT_trunk_link      10
#define rg_OBJECT_primary_link    11
#define rg_OBJECT_secondary_link  12
#define rg_OBJECT_tertiary_link   13
#define rg_OBJECT_living_street   14
#define rg_OBJECT_pedestrian      15
#define rg_OBJECT_track           16
#define rg_OBJECT_bus_guideway    17
#define rg_OBJECT_escape          18
#define rg_OBJECT_raceway         19
#define rg_OBJECT_road            20
#define rg_OBJECT_footway         21
#define rg_OBJECT_bridleway       22
#define rg_OBJECT_steps           23
#define rg_OBJECT_path            24

// buildings
#define rg_OBJECT_building        25

// buildings
#define rg_OBJECT_tagged_node     26

// cells
#define rg_OBJECT_cell            27


/*
highlocate    motorlocate
highlocate    trunk
highlocate    primary
highlocate    secondary
highlocate    tertiary
highlocate    unclassified
highlocate    residential
highlocate    service
highlocate    motorlocate_link
highlocate    trunk_link
highlocate    primary_link
highlocate    secondary_link
highlocate    tertiary_link
highlocate    living_street
highlocate    pedestrian
highlocate    track
highlocate    bus_guidelocate
highlocate    escape
highlocate    racelocate
highlocate    road
highlocate    footlocate
highlocate    bridlelocate
highlocate    steps
highlocate    path
*/

/* max of 64k for tag value */
struct rg_Tag {
	rg_Ptr_String key;
	rg_Ptr_String value;
};

struct rg_Object {
	u64         id;

	/* replace all this by a list of key value tags */
	struct {
		/* at most 65k tags */
		u16        count;
		rg_Ptr_Tag begin;
	} tags;

	/*
	 * capacity of the nodes array is implicit:
	 * a function of the number of nodes
	 */
	u32         num_locates;
	al_Ptr_char locates;

	/* algorithms might need to tag rg_Object */
	union {
		void *custom_data;
		u64  custom_flags;
	};
};

//------------------------------------------------------------------------------
// Caches
//------------------------------------------------------------------------------

//
// will keep it simple: a NodeWrap when created has an exact size that is
// rounded up to some levels. Which cache we use is dependent on that size
//

static const u32 rg_Caches_Classes[] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048,
	3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768, 49152, 65536, 98304, 131072, 196608, 262144,
	393216, 524288, 786432, 1048576, 1572864, 2097152, 3145728, 4194304, 6291456, 8388608, 12582912,
	16777216, 25165824, 33554432, 50331648, 67108864, 100663296 };

#define rg_Caches_CLASSES_CAPACITY 53

typedef struct {
	al_Ptr_Cache     caches[rg_Caches_CLASSES_CAPACITY];
	al_Ptr_Allocator allocator_p;
} rg_Caches;

typedef struct {

	/* cache of varios sizes */
	rg_Caches           caches;

	al_Ptr_Cache        node_cache_p;     // internal locate cache
	al_Ptr_Cache        locate_cache_p;      // payload cache

	/* the rbt_BTree stores offset version of pointer to char */
	/* should be casted on usage to nodes or locates */
	rbt_BTree           nodes;
	rbt_BTree           locates;

	ss_StringSet        string_set;

	rg_Ptr_Locate         vp_root;

} rg_Graph;


typedef struct {
	rg_Locate* *begin;
	rg_Locate* *end;
	rg_Locate* *capacity;
} rg_LocatePtrArray;




/*
 * rg_LocatePtrArray
 */

static void
rg_LocatePtrArray_init(rg_LocatePtrArray *self, rg_Locate* *begin, rg_Locate* *capacity)
{
	self->begin    = begin;
	self->end      = begin;
	self->capacity = capacity;
}

static void
rg_LocatePtrArray_append(rg_LocatePtrArray *self, rg_Locate *locate)
{
	Assert(self->end < self->capacity);
	*self->end = locate;
	++self->end;
}

static void
rg_LocatePtrArray_clear(rg_LocatePtrArray *self)
{
	self->end = self->begin;
}

static u32
rg_LocatePtrArray_length(rg_LocatePtrArray *self)
{
	return (u32) (self->end - self->begin);
}

static rg_Locate*
rg_LocatePtrArray_get(rg_LocatePtrArray *self, u32 index)
{
	Assert(self->begin + index < self->end);
	return *(self->begin + index);
}




PTR_SPECIALIZED_SERVICES(rg_Ptr_Locate, rg_Locate);
PTR_SPECIALIZED_SERVICES(rg_Ptr_Object, rg_Object);
PTR_SPECIALIZED_SERVICES(rg_Ptr_String, rg_String);
PTR_SPECIALIZED_SERVICES(rg_Ptr_Tag, rg_Tag);

/*
 * rg_Locate
 */

static void
rg_Locate_init(rg_Locate *self, u64 id)
{
	self->id = id;
	self->state = rg_Locate_COUNTING_STATE;
	self->lat = 0.0;
	self->lon = 0.0;
	self->num_objects = 0;
	self->custom_flags = 0;
	rg_Ptr_Object_set_null(&self->singleton);


	rg_Ptr_Locate_set_null(&self->vp.near);
	rg_Ptr_Locate_set_null(&self->vp.far);
	self->vp.radius = 0.0f;
}

static void
rg_Locate_increment_incidence(rg_Locate *self)
{
	Assert(self->state == rg_Locate_COUNTING_STATE);
	++self->num_objects;
}

static void
rg_Locate_start_appending(rg_Locate *self, rg_Ptr_Object *begin, rg_Ptr_Object *end)
{
	Assert(self->state == rg_Locate_COUNTING_STATE);
	if (self->num_objects == 0) {
		Assert(begin == 0 && end == 0);
		self->state = rg_Locate_READY_STATE;
	} else if (self->num_objects <= 1) {
		Assert(begin == 0 && end == 0);
		self->custom_flags = self->num_objects;
		self->num_objects     = 0;
		self->state = rg_Locate_APPENDING_STATE;
	} else {
		Assert(end - begin >= self->num_objects);

		/* hack: temporarily use custom flat to store num nodes */
		self->custom_flags = self->num_objects;
		self->num_objects     = 0;
		al_Ptr_char_set(&self->objects, (char*) begin);
		self->state = rg_Locate_APPENDING_STATE;
	}
}

static void
rg_Locate_append_object(rg_Locate *self, rg_Object *object)
{
	Assert(self->state == rg_Locate_APPENDING_STATE);
	Assert(self->custom_flags > 0);
	if (self->custom_flags == 1) {
		rg_Ptr_Object_set(&self->singleton, object);
	} else {
		rg_Ptr_Object *begin = (rg_Ptr_Object*) al_Ptr_char_get(&self->objects);
		rg_Ptr_Object_set(begin + self->num_objects, object);
	}
	++self->num_objects;
	if ((u64) self->num_objects == self->custom_flags) {
		self->num_objects = self->custom_flags;
		self->state = rg_Locate_READY_STATE;
	}
}



/*
 * rg_Object
 */

static void
rg_Object_init(rg_Object *self, u64 id)
{
	self->id = id;
	self->tags.count = 0;
	self->num_locates = 0;
	rg_Ptr_Tag_set_null(&self->tags.begin);
	al_Ptr_char_set_null(&self->locates);
}

static void
rg_Object_set_tags(rg_Object *self, rg_Tag *begin, rg_Tag *end)
{
	self->tags.count = pt_safe_s64_u16(end - begin);
	rg_Ptr_Tag_set(&self->tags.begin, begin);
}

static void
rg_Object_set_incident_nodes(rg_Object *self, rg_Ptr_Locate *begin, rg_Ptr_Locate *end)
{
	self->num_locates = (u16) (end - begin);
	al_Ptr_char_set(&self->locates, (char *) begin);
}

static rg_Ptr_Locate*
rg_Object_get_locates_begin(rg_Object *self)
{
	return (rg_Ptr_Locate*) al_Ptr_char_get(&self->locates);
}


static rg_Object*
rg_Locate_get_object(rg_Locate *self, s32 index)
{
	Assert(index < self->num_objects);
	if (self->num_objects == 1) {
		rg_Object *object = rg_Ptr_Object_get(&self->singleton);
		return(object);
	} else {
		rg_Ptr_Object *locates_begin = (rg_Ptr_Object*) al_Ptr_char_get(&self->objects);
		rg_Object *object = rg_Ptr_Object_get(locates_begin + index);
		return(object);
	}
}



/*
 * rg_Caches
 */

static void
rg_Caches_init(rg_Caches *self, al_Allocator *allocator)
{
	al_Ptr_Allocator_set(&self->allocator_p, allocator);
	al_Ptr_Cache *it  = self->caches;
	al_Ptr_Cache *end = self->caches + rg_Caches_CLASSES_CAPACITY;
	while (it != end) {
		al_Ptr_Cache_set_null(it);
		++it;
	}
}

/* round to the next number with zeros everywhere except on the two most significative bits */
static inline u32
rg_size_class_to_index(u32 size)
{
	u32 msb = pt_msb32(size);
	return (size > 1) * ( (msb-1) * 2 - ( ( (1 << (msb-2)) & size ) == 0 ) );
}

static void*
rg_Caches_alloc(rg_Caches *self, u64 size)
{
	Assert(size > 0);
	u32 size_class = pt_normalize_msb2(size);
	u32 index = rg_size_class_to_index(size_class);
	Assert(index < rg_Caches_CLASSES_CAPACITY);
	al_Cache *cache = al_Ptr_Cache_get(self->caches + index);
	if (!cache) {
		// create w_node cache (nodes are uniformly sized!)
		char name[al_MAX_CACHE_NAME_LENGTH+1];
		Print print;
		print_init(&print, name, sizeof(name));

		print_clear(&print);
		print_cstr(&print, "rg_Cache_");
		print_u64(&print, size_class);
		print_char(&print, 0);

		al_Allocator *allocator = al_Ptr_Allocator_get(&self->allocator_p);
		if (size_class < al_MIN_CACHE_CHUNK_SIZE) {
			size_class = al_MIN_CACHE_CHUNK_SIZE;
		}
		cache = al_Allocator_create_cache(allocator, name, size_class);
		al_Ptr_Cache_set(self->caches + index, cache);
	}
	return al_Cache_alloc(cache);
}

static void
rg_Caches_free(rg_Caches *self, u64 size, void *p)
{
	u32 size_class = pt_normalize_msb2(size);
	u32 index = rg_size_class_to_index(size_class);
	Assert(index < rg_Caches_CLASSES_CAPACITY);
	al_Cache *cache = al_Ptr_Cache_get(self->caches + index);
	al_Cache_free(cache, p);
}

/*
 * rg_Heap and rg_HeapItem
 */

typedef struct {
	void *data;
	f64  value;
} rg_HeapItem;


typedef struct {
	rg_HeapItem *begin;
	rg_HeapItem *end;
	rg_HeapItem *capacity;
} rg_Heap;

static void
rg_Heap_init(rg_Heap *self, rg_HeapItem *begin, rg_HeapItem *capacity)
{
	Assert(capacity >= begin);
	self->begin = begin;
	self->end   = begin;
	self->capacity = capacity;
}

static void
rg_HeapItem_swap(rg_HeapItem *self, rg_HeapItem *other)
{
	f64 x = self->value;
	self->value = other->value;
	other->value = x;
	void* d = self->data;
	self->data = other->data;
	other->data = d;
}

static b8
rg_Heap_full(rg_Heap *self)
{
	return self->end == self->capacity;
}

static f64
rg_Heap_min(rg_Heap *self)
{
	Assert(self->begin < self->end);
	return self->begin->value;
}

static void
rg_Heap_insert(rg_Heap *self, void *data, f64 value)
{
	Assert(self->end < self->capacity);
	self->end->data  = data;
	self->end->value = value;

	/* bubble up */
	rg_HeapItem *it = self->end;
	s64 index       = self->end - self->begin;
	while (index > 0) {
		s64 parent_index = (index-1) / 2;
		rg_HeapItem *parent = self->begin + parent_index;
		if (parent->value > it->value) {
			rg_HeapItem_swap(it, parent);
			index = parent_index;
			it    = parent;
		} else {
			break;
		}
	}
	++self->end;
}

static void
rg_Heap_bubble_down(rg_Heap *self)
{
	/* bubble down */
	s64 n = self->end - self->begin;

	/* bubble up */
	rg_HeapItem *it = self->begin;
	rg_HeapItem *it_left, *it_right;
	s64 index       = 0;
	while (index < n) {
		s64 left  = 2 * index + 1;
		s64 right = 2 * index + 2;
		if (left >= n) {
			break; // done
		} else {
			it_left = self->begin + left;
			if (right >= n) {
				if (it->value > it_left->value) {
					rg_HeapItem_swap(it_left, it);
					it    = it_left;
					index = left;
				} else {
					break;
				}
			} else {
				it_right = self->begin + right;
				if (it_left->value <= it_right->value) {
					if (it->value > it_left->value) {
						rg_HeapItem_swap(it_left, it);
						it    = it_left;
						index = left;
					} else {
						break;
					}
				} else {
					if (it->value > it_right->value) {
						rg_HeapItem_swap(it_right, it);
						it    = it_right;
						index = right;
					} else {
						break;
					}
				}
			}
		}
	}
}

static rg_HeapItem
rg_Heap_pop(rg_Heap *self)
{
	Assert(self->begin < self->end);
	rg_HeapItem result = *self->begin;
	rg_HeapItem_swap(self->begin, self->end-1);
	--self->end;

	rg_Heap_bubble_down(self);

	return result;
}

static void
rg_Heap_final_sort(rg_Heap *self)
{
	rg_HeapItem *end_backup = self->end;
	while (self->end - self->begin > 1) {
		rg_HeapItem_swap(self->begin, self->end-1);
		--self->end;
		rg_Heap_bubble_down(self);
	}
	self->end = end_backup;
}

/*
 * rg_Graph
 */

static void
rg_Graph_init(rg_Graph *self, al_Allocator *allocator)
{
	// create w_node cache (nodes are uniformly sized!)
	char name[al_MAX_CACHE_NAME_LENGTH+1];
	Print print;
	print_init(&print, name, sizeof(name));

	print_clear(&print);
	print_cstr(&print, "rg_Locate");
	print_char(&print, 0);
	al_Ptr_Cache_set(&self->node_cache_p, al_Allocator_create_cache(allocator, name, (u32) sizeof(rg_Locate)));

	print_clear(&print);
	print_cstr(&print, "rg_Object");
	print_char(&print, 0);
	al_Ptr_Cache_set(&self->locate_cache_p, al_Allocator_create_cache(allocator, name, (u32) sizeof(rg_Object)));

	ss_StringSet_init(&self->string_set, allocator);
	rbt_BTree_init(&self->nodes, allocator);
	rbt_BTree_init(&self->locates,  allocator);

	rg_Caches_init(&self->caches, allocator);
}

static rg_Object*
rg_Graph_insert_locate(rg_Graph *self, u64 id, MemoryBlock *key_values_begin, MemoryBlock *key_values_end)
{
	Assert((key_values_end - key_values_begin) % 2 == 0);

	/* allocate a new locate */
	al_Cache* cache = al_Ptr_Cache_get(&self->locate_cache_p);
	rg_Object *locate = (rg_Object*) al_Cache_alloc(cache);

	/* init locate with id */
	rg_Object_init(locate, id);

	/* allocate space for tags */
	u16 num_tags = pt_safe_s64_u16((key_values_end - key_values_begin)/2);
	if (num_tags > 0) {
		rg_Tag *tags = rg_Caches_alloc(&self->caches, num_tags * sizeof(rg_Tag));
		for (s32 i=0;i<num_tags;++i) {
			MemoryBlock *key   = key_values_begin + (2*i);
			MemoryBlock *value = key + 1;
			rg_String *key_str = ss_StringSet_get(&self->string_set, key->begin, key->end);
			if (!key_str) {
				key_str = ss_StringSet_insert(&self->string_set, key->begin, key->end);
			}
			rg_String *value_str = ss_StringSet_get(&self->string_set, value->begin, value->end);
			if (!value_str) {
				value_str = ss_StringSet_insert(&self->string_set, value->begin, value->end);
			}
			rg_Ptr_String_set(&(tags + i)->key,   key_str);
			rg_Ptr_String_set(&(tags + i)->value, value_str);
		}

		/* set tags into locate */
		rg_Object_set_tags(locate, tags, tags + num_tags);
	}

	/* insert pointer to locate into "locates" key-value store */
	rbt_BTree_insert(&self->locates, id, (char*) locate);

	return locate;
}

static rg_Locate*
rg_Graph_insert_node(rg_Graph *self, u64 id)
{
	al_Cache* cache = al_Ptr_Cache_get(&self->node_cache_p);
	rg_Locate *locate = (rg_Locate*) al_Cache_alloc(cache);
	rg_Locate_init(locate, id);

	rbt_BTree_insert(&self->nodes, id, (char*) locate);
	return locate;
}

/* find locate by id */
static rg_Locate*
rg_Graph_get_node(rg_Graph *self, u64 node_id)
{
	char *value = 0;
	if (rbt_BTree_get_value(&self->nodes, node_id, &value)) {
		return (rg_Locate*) value;
	} else {
		return 0;
	}
}

/* find locate by id */
static rg_Object*
rg_Graph_get_object(rg_Graph *self, u64 locate_id)
{
	char *value = 0;
	if (rbt_BTree_get_value(&self->locates, locate_id, &value)) {
		return (rg_Object*) value;
	} else {
		return 0;
	}
}

static void
rg_Graph_set_incident_locates_to_object(rg_Graph *self, rg_Object *object, rg_Locate* *begin, rg_Locate* *end)
{
	Assert(object->num_locates == 0);

	u32 num_nodes = pt_safe_s64_u32(end - begin);

	/* alobject space to store nodes */
	if (num_nodes > 0) {
		u64 size = sizeof(rg_Ptr_Locate) * num_nodes;
		rg_Ptr_Locate *node_ptr_array = (rg_Ptr_Locate*) rg_Caches_alloc(&self->caches, size);
		rg_Ptr_Locate *it_dst = node_ptr_array;
		rg_Locate*    *it_src = begin;
		while (it_src != end) {
			/* increase incidence of locate */
			rg_Locate_increment_incidence(*it_src);

			rg_Ptr_Locate_set(it_dst, *it_src);
			++it_dst;
			++it_src;
		}

		/* initialize incidence list on object */
		rg_Object_set_incident_nodes(object, node_ptr_array, node_ptr_array + num_nodes);
	}
}

static void
rg_Graph_initialize_locates_incidence(rg_Graph *self)
{
	/* Allocate space for locate incidence */
	{
		rbt_Iter iter;
		rbt_Iter_init(&iter, &self->nodes);
		u64  id = 0;
		char *value = 0;
		while (rbt_Iter_next(&iter, &id, &value)) {
			rg_Locate *locate = (rg_Locate*) value;
			Assert(locate);
			u32 num_objects = locate->num_objects;
			if (num_objects > 1) {
				u64 size = sizeof(rg_Ptr_Object) * num_objects;
				rg_Ptr_Object  *locate_ptr_array = (rg_Ptr_Object*) rg_Caches_alloc(&self->caches, size);
				rg_Locate_start_appending(locate, locate_ptr_array, locate_ptr_array + num_objects);
			} else {
				rg_Locate_start_appending(locate, 0, 0);
			}
		}
	}

	/* Associate locates to nodes */
	{
		rbt_Iter iter;
		rbt_Iter_init(&iter, &self->locates);
		u64  id = 0;
		char *value = 0;
		while (rbt_Iter_next(&iter, &id, &value)) {
			rg_Object *object = (rg_Object*) value;
			if (object->num_locates > 0) {
				rg_Ptr_Locate *it = (rg_Ptr_Locate*) al_Ptr_char_get(&object->locates);
				rg_Ptr_Locate *end = it + object->num_locates;
				while (it != end) {
					rg_Locate *locate = rg_Ptr_Locate_get(it);
					rg_Locate_append_object(locate, object);
					++it;
				}
			}
		}
	}
}

/*
 * Copied this rnd_ stuff from
 * http://www.eternallyconfuzzled.com/tuts/algorithms/jsw_tut_rand.aspx
 */
#define rg_rnd_M 2147483647
#define rg_rnd_A 16807
#define rg_rnd_Q ( rg_rnd_M / rg_rnd_A )
#define rg_rnd_R ( rg_rnd_M % rg_rnd_A )

static s32 rg_rnd_state = 1;
s32 rg_rnd_next()
{
    rg_rnd_state = rg_rnd_A * (rg_rnd_state % rg_rnd_Q) - rg_rnd_R * (rg_rnd_state / rg_rnd_Q);
    if (rg_rnd_state <= 0) {
	    rg_rnd_state += rg_rnd_M;
    }
    return rg_rnd_state;
}

typedef struct {
	rg_Locate *locate;
	f32     distance;
} rg_VPNode;

static void
rg_VPNode_swap(rg_VPNode *self, rg_VPNode *other)
{
	rg_Locate *naux = self->locate;
	self->locate = other->locate;
	other->locate = naux;

	f32 daux =  self->distance;
	self->distance = other->distance;
	other->distance = daux;
}

/*
 * permute the input array
 *       x_0      ...      x_{n-1}
 * using any permutation i1 ... in such that
 *       x_i[0] ... x_i[k] ... x_i[n-1]
 * either
 * (1) x_i[k]  >  x_i[j]  for 0 <= j < k
 *
 * (2) x_i[kk] >  x_i[j]  for 0 <= j < kk
 *     x_i[j]  == x_i[k]  for kk <= j <= k
 */
static rg_VPNode*
rg_vp_kth_smallest(rg_VPNode *begin, rg_VPNode *end, s64 k)
{
	Assert(k < end - begin);

	s64 n = end - begin;
	Assert(n <= rg_rnd_M);
	Assert(n > 0);

	rg_VPNode *global_begin = begin;

	rg_VPNode *result = begin;
	while (end - begin > 1) {
		n = end - begin;

		/* pick a random index */
		s64 index = rg_rnd_next() % n;
		rg_VPNode_swap(begin, begin + index);

		rg_VPNode *left  = begin + 1;
		rg_VPNode *right = end   - 1;
		f32 pivot = begin->distance;

		while (left < right) {

			while (left->distance < pivot && left < right) {
				++left;
			}
			/* left is in a position where left->distance >= pivot */
			/* or left == right */

			/* hyp: if left != right, then this loop will stop */
			while (pivot <= right->distance && left < right) {
				--right;
			}
			/* left is in a position where right->distance < pivot */
			/* or left == right */

			if (left < right) {
				rg_VPNode_swap(left, right);
			}
		}

		if (left->distance >= pivot) {
			rg_VPNode_swap(begin, left-1);
			result = left - 1;
		} else  {
			rg_VPNode_swap(begin, left);
			result = left;
		}

		if (result - global_begin == k) {
			break;
		} else if (result - global_begin > k) {
			end = result;
		} else {
			/* result can be the same as begin */
			begin = result + 1;
		}
		result = begin;
	}

	/* make sure we are at the left most item with the same value */
	while (result != global_begin && result->distance == (result-1)->distance) {
		--result;
	}

	return result;
}

typedef struct {
	rg_VPNode *vantage_point;
	rg_VPNode *near;
	rg_VPNode *far;
	rg_VPNode *end;
	f32       radius;
} rg_vp_SelectResult;


#define rg_vp_EARTH_RADIUS 6371000.0

static f32
rg_vp_haversine_dist(f32 lat1, f32 lon1, f32 lat2, f32 lon2)
{
	f64 rx1 = pt_PI * lon1/180.0;
	f64 ry1 = pt_PI * lat1/180.0;
	f64 rx2 = pt_PI * lon2/180.0;
	f64 ry2 = pt_PI * lat2/180.0;

	f64 drx = rx2 - rx1;
	f64 dry = ry2 - ry1;

	f64 sin_dry = pt_sin_f64(dry/2);
	f64 sin_drx = pt_sin_f64(drx/2);
	f64 a = sin_dry * sin_dry +
		pt_cos_f64(ry1) * pt_cos_f64(ry2) * sin_drx * sin_drx;

	f64 c = 2 * pt_atan2_f64(pt_sqrt_f64(a), pt_sqrt_f64(1.0 - a));

	return (f32) (rg_vp_EARTH_RADIUS * c);
}

static f32
rg_vp_dist_nodes(rg_Locate *a, rg_Locate *b)
{
	return rg_vp_haversine_dist(a->lat, a->lon, b->lat, b->lon);
}

static f32
rg_vp_dist(rg_Locate *a, f32 lat, f32 lon)
{
	return rg_vp_haversine_dist(a->lat, a->lon, lat, lon);
}

// internal f32
// rg_vp_distL1_nodes(rg_Locate *a, rg_Locate *b)
// {
// 	return pt_abs_f32(a->lon - b->lon) + pt_abs_f32(a->lat - b->lat);
// }
//
// internal f32
// rg_vp_distL1(rg_Locate *a, f32 lat, f32 lon)
// {
// 	return pt_abs_f32(a->lon - lon) + pt_abs_f32(a->lat - lat);
// }
//
//
// internal f32
// rg_vp_distL1_nodes(rg_Locate *a, rg_Locate *b)
// {
// 	return pt_abs_f32(a->lon - b->lon) + pt_abs_f32(a->lat - b->lat);
// }
//
// internal f32
// rg_vp_distL1(rg_Locate *a, f32 lat, f32 lon)
// {
// 	return pt_abs_f32(a->lon - lon) + pt_abs_f32(a->lat - lat);
// }


/*
 * As described by Peter Yianilos the function vp_select
 * tries to find the locate that has the largest "median spread"
 * in the current active set of point to see which point should
 * be chosen as the next vantage point.
 */
static rg_vp_SelectResult
rg_vp_select(rg_VPNode *begin, rg_VPNode *end)
{
	rg_vp_SelectResult result;

	/* find a nice spot to make a partition and recurse */
	s64 n = end - begin;

	s64 index = rg_rnd_next() % n;
	rg_VPNode_swap(begin, begin + index);

	begin->distance = 0.0f;

	/* compute distance relative to selected element */
	rg_VPNode *it = begin + 1;
	while (it != end) {
		/* compute distance metric */
		it->distance = rg_vp_dist_nodes(begin->locate, it->locate);
		++it;
	}


	/* find the median */
	result.vantage_point = begin;
	if (n == 2) {
		result.near          = begin + 1;
		result.far           = begin + 1;
		result.end           = end;
		result.radius        = result.far->distance;
	} else {
		result.near          = begin + 1;
		result.far           = rg_vp_kth_smallest(result.near, end, (end - result.near + 1)/2);
		result.end           = end;
		result.radius        = result.far->distance;
	}

	return result;
}

static rg_VPNode*
rg_vp_make_tree(rg_VPNode *begin, rg_VPNode *end)
{
	/* find a nice spot to make a partition and recurse */
	s64 n = end - begin;

	if (n == 0) {
		return 0;
	}

	/* limited to 2B nodes */
	Assert(n < rg_rnd_M);

	if (n == 1) {
		return begin;
	}

	rg_vp_SelectResult partition = rg_vp_select(begin, end);

	/* nodes whose distance to vantage point is less than mu */
	rg_VPNode *near = rg_vp_make_tree(partition.near, partition.far);

	/* nodes whose distance to vantage point is mu or more */
	rg_VPNode *far  = rg_vp_make_tree(partition.far,  partition.end);

	rg_Locate* vp = partition.vantage_point->locate;
	rg_Ptr_Locate_set(&vp->vp.near, (near ? near->locate : 0));
	rg_Ptr_Locate_set(&vp->vp.far,  (far  ? far->locate  : 0));
	vp->vp.radius = partition.radius;

	return partition.vantage_point;
}

/*
 *
 */
static void
rg_Graph_initalize_vantage_point_tree(rg_Graph *self, char *buffer_begin, char *buffer_end)
{
	LinearAllocator work_memory;
	LinearAllocator_init(&work_memory, buffer_begin, buffer_begin, buffer_end);

	rg_VPNode *begin   = (rg_VPNode*) LinearAllocator_alloc(&work_memory, sizeof(rg_VPNode*) * self->nodes.size);
	rg_VPNode *end     = begin + self->nodes.size;
	rg_VPNode *it_vp   = begin;

	rbt_Iter iter;
	rbt_Iter_init(&iter, &self->nodes);
	u64  id     = 0;
	char *value = 0;
	while (rbt_Iter_next(&iter, &id, &value)) {
		Assert(it_vp != end);
		rg_Locate *locate = (rg_Locate*) value;
		it_vp->locate = locate;
		it_vp->distance = 0.0f;
		++it_vp;
	}
	Assert(it_vp == end);

	/* now recursively partition the array */
	rg_VPNode *vp_root = rg_vp_make_tree(begin, end);

	rg_Ptr_Locate_set(&self->vp_root, vp_root->locate);
}

static void
rg_Locate_nearest_neighbors(rg_Locate *self, f32 lat, f32 lon, f32 max_dist, rg_Heap *heap, rg_LocateFilterCallback *filter, void *filter_data)
{
	f32 dist = rg_vp_dist(self, lat, lon);

	if (dist <= max_dist) {
		if (!filter || (*filter)(self, filter_data)) {
			if (!rg_Heap_full(heap)) {
				rg_Heap_insert(heap, self, -dist);
			} else {
				if (dist < -rg_Heap_min(heap)) {
					/* @TODO(llins): implement rg_Heap_replace */
					rg_Heap_pop(heap);
					rg_Heap_insert(heap, self, -dist);
				}
			}
		}
	}

	b8 search_near = dist < self->vp.radius + max_dist;
	b8 search_far  = (self->vp.radius <= max_dist) || (dist >= self->vp.radius - max_dist);

	if (search_near) {
		rg_Locate *near = rg_Ptr_Locate_get(&self->vp.near);
		if (near) {
			rg_Locate_nearest_neighbors(near, lat, lon, max_dist, heap, filter, filter_data);
		}
	}

	if (search_far) {
		rg_Locate *far = rg_Ptr_Locate_get(&self->vp.far);
		if (far) {
			rg_Locate_nearest_neighbors(far, lat, lon, max_dist, heap, filter, filter_data);
		}
	}
}

static void
rg_Graph_nearest_neighbors(rg_Graph *self, f32 lat, f32 lon, f32 max_dist, rg_Heap *heap, rg_LocateFilterCallback *filter, void *filter_data)
{
	rg_Locate *vp_root = rg_Ptr_Locate_get(&self->vp_root);
	if (vp_root)
		rg_Locate_nearest_neighbors(vp_root, lat, lon, max_dist, heap, filter, filter_data);
}

