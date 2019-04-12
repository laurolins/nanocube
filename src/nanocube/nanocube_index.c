/*
BEGIN_TODO
# 2017-03-23T10:59
Consider including the depth information of a node in the node. This would
eliminate the need to maintain suffix lengths on the child links and maybe
simplify things. First though, make sure we have a robust insertion algorithm
with the current child suffix length scheme.

# 2017-03-23T11:01
Make a specific class for PNodeLeaf and INodeLeaf. Leaves never change,
and they can be smaller, since they will never ever need children links.

# 2017-03-23T18:52
Optimize function nx_Threads_singleton to avoid computing the same
depth list over and over on worst run cases. The example bug4e was
such a case. Where threads 0 and 1 neither one would dominate the
other, but thread 2 would dominate both.

END_TODO
*/

//
// The idea behind the a nested hierarchy object is that
// we want to route payload packages through complete
// paths crossing all the hierarchies. Those packages
// should be "stored" in each node (ie constraint)
// hit by these paths.
//
// PNode assumes the following classes are available:
//
// PayloadUnit       - a payload package.
// PayloadAggregate  - custom abstraction for a list
//                     of payload units.
//
// In the reference implementation for a payload scheme
// provided below, we assume a PayloadUnit is a number
// between 0 and 63, and the PayloadAggregate is the set
// of "units" that incide in that set.
//


// turn on and off profiling hooks in this module
// #define nx_PROFILE

#ifdef nx_PROFILE
#define nx_pf_BEGIN_BLOCK(a) pf_BEGIN_BLOCK(a)
#define nx_pf_END_BLOCK() pf_END_BLOCK()
#else
#define nx_pf_BEGIN_BLOCK(a)
#define nx_pf_END_BLOCK()
#endif

#ifndef nx_PAYLOAD

typedef u32 nx_PayloadUnit;

typedef struct {
	u64 bitset;
} nx_PayloadAggregate;

/* declare the 3 services that are expected with respect to payloads */
static void
nx_PayloadAggregate_init(nx_PayloadAggregate *self, void* payload_context);

static void
nx_PayloadAggregate_share(nx_PayloadAggregate* self, nx_PayloadAggregate* other, void *payload_context);

static void
nx_PayloadAggregate_insert(nx_PayloadAggregate* self, void *payload_unit, void *payload_context);

#define nx_PAYLOAD
#define nx_DEFAULT_PAYLOAD

#endif


//------------------------------------------------------------------------------
// Ptr Types
//------------------------------------------------------------------------------

PTR_SPECIALIZED_TYPE(nx_Ptr_Node);
PTR_SPECIALIZED_TYPE(nx_Ptr_Child);
PTR_SPECIALIZED_TYPE(nx_Ptr_Label);

//------------------------------------------------------------------------------
// Type Aliaseses
//------------------------------------------------------------------------------

typedef u32 nx_SizeClass;
typedef u32 nx_Bytes;
typedef u8  nx_Bits;
typedef u64 nx_NumNodes;
typedef u8  nx_Label;

#define nx_MAX_NUM_DIMENSIONS  64
#define nx_MAX_PATH_LENGTH    127
#define nx_MAX_DEGREE         256

//
// a INode occupies 20 bytes
// a Detail occupies up to
//
//   127 bytes for path + 256 * sizeof(Child) = 127 + 256 * 8 = 2175
//

//------------------------------------------------------------------------------
// Path
//------------------------------------------------------------------------------

//
// Wraps a region reserved for a path with the functionality needed.
// Desgined to be used shortly every time there is a need to manipulate
// paths (query or update)
//

typedef struct {
	nx_Label*     begin; // this is a compressed vector
	u8 length;
	u8 capacity;
	nx_Bits       bits_per_label;
	u8            padding1;
	u32           padding2;
} nx_Path;


//------------------------------------------------------------------------------
// Child
//------------------------------------------------------------------------------

//
// (1) suffix length is valid only when dealing with a shared child;
// (2) if shared and suffix length == 0, then it is a proxy to indicate that
//     the suffix has the length of the child path
//

typedef struct {
	u8          label;
	u8          suffix_length : 7;
	u8          shared : 1;      // first label of child
	nx_Ptr_Node node_p;
} nx_Child;

//------------------------------------------------------------------------------
// Children
//------------------------------------------------------------------------------

//
// Wraps a region reserved for a children node pointer with the functionality
// needed. Desgined to be used shortly every time there is a need to manipulate
// children (query or update)
//

typedef struct {
	nx_Child* begin;
	s32       length;
	s32       capacity;
} nx_Children;

//------------------------------------------------------------------------------
// Detail
//------------------------------------------------------------------------------

typedef enum {
	nx_DETAIL_c0p22,
	nx_DETAIL_c16p6,
	nx_DETAIL_c16pL,
	nx_DETAIL_cLp16,
	nx_DETAIL_cLpL,
	nx_DETAIL_COUNT
} nx_DetailType;

//
// c = children
// p = path
// <number> = number of bytes
// L = link
//
typedef union {
	char raw[22];
	struct {
		nx_Child children[2];
		nx_Label path[6];
	} c16p6;
	struct {
		nx_Label path[22]; // this is compressed label info
	} c0p22;
	struct {
		nx_Child children[2];
		nx_Ptr_Label path_p;
	} c16pL;
	struct {
		nx_Ptr_Child children_p;
		nx_Label path[16];
	} cLp16;
	struct {
		nx_Ptr_Child children_p;
		nx_Ptr_Label path_p;
	} cLpL;
} nx_Detail;


//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

typedef struct {
	u8                path_length : 7;
	u8                root :        1;
	u8                degree_; // 0 is zero, 1 .. 255 represents respectively 2 .. 256
	nx_Ptr_Node       parent_p;
	nx_Detail         detail; // memory block that depending on degree and path_length
	// is interpreted in different ways
} nx_Node;

//------------------------------------------------------------------------------
// INode
//------------------------------------------------------------------------------

//
// INode or Internal NodeWrap are the nodes in the hierarchies that
// are not in the last layer. The nodes in the last layer are
// called PNodes or Payload nodes. The difference between a
// PNode and a INode is that instead of a content pointer
// present in the INodes, a PNode has a payload area
// reserved for the user of the nested hierarchy.
//
//
// cannot figure out dimension jump following the parent()
// infra-structure unless we have a root_p flag somewhere
//
// assume we can have 255 children and 127-level deep trees
//
//
// 2 + 3 * 6 = 20 bytes per node
//

typedef struct {
	nx_Node       node;

	//
	// Ptr to another hierarchy (in case of intermediate node)
	// otherwise pointer to specific content. We should cast to the
	// specific content in this case to keep everything simple.
	//
	// children will be tagged offset ptr
	//
	nx_Ptr_Node   content_p;

} nx_INode;



//------------------------------------------------------------------------------
// PNode - Payload Node
//------------------------------------------------------------------------------

typedef struct {
	nx_Node             node;
	nx_PayloadAggregate payload_aggregate;
} nx_PNode;

//------------------------------------------------------------------------------
// NodeWrap
//------------------------------------------------------------------------------

typedef struct {
	nx_Node*      raw_node;
	nx_Path       path;
	nx_Children   children;
	nx_SizeClass  path_class;
	nx_SizeClass  children_class;
	nx_DetailType detail_type;
} nx_NodeWrap;

//
// By our definition that a hierarchy can be at most 127 levels deep
// and the degree of a node can have at most 255 children, the maximum
// size of a NodeWrap is: 2 bytes of control + 255 * 8 bytes + 127 * 1 bytes.
// A total of 2167 bytes wide.
//

//------------------------------------------------------------------------------
// Labels
//------------------------------------------------------------------------------

typedef struct {
	u8* begin;
	u8  length;
} nx_Array;

//------------------------------------------------------------------------------
// LabelArrayList
//------------------------------------------------------------------------------

//
// Single linked list of label arrays
//
typedef struct LabelArrayList {
	nx_Array                  labels;
	struct LabelArrayList *next;
} nx_LabelArrayList;

//------------------------------------------------------------------------------
// Caches
//------------------------------------------------------------------------------

//
// will keep it simple: a NodeWrap when created has an exact size that is
// rounded up to some levels. Which cache we use is dependent on that size
//

static const nx_SizeClass nx_Caches_Classes[] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64,
	96, 128, 192, 256, 384, 512, 768, 1024,
	1536, 2048, 3072, 4096
};

#define nx_Caches_NumClasses 24

typedef struct {
	u32             initialized : 1;
	al_Ptr_Cache    caches[nx_Caches_NumClasses];
} nx_Caches;

//-------------------------------------------------------------------------------
// NestedHierarchy
//-------------------------------------------------------------------------------

//
// Hierarchy wraps a single node as the root_p of a hierarchy.
// This initial node might be null indicating an initally empty
// hierarchy. The hierarchy must also have access to the
// allocator objects to reserve memory for new paths and updates.
//
typedef struct {
	u8           dimensions;    // number of dimensions
	u8           initialized : 1;    // if this flag is zero: this object is invalid

	nx_Caches    detail_caches; // caches for node class sizes

	al_Ptr_Cache inode_cache_p;    // internal node cache
	al_Ptr_Cache pnode_cache_p;    // payload cache

	nx_Ptr_Node  root_p;          // might be null

	nx_Bits      bits_per_label[nx_MAX_NUM_DIMENSIONS]; // number of bits per dimension

	/* records inserted into this index */
	u64          number_of_records;
} nx_NanocubeIndex;


//------------------------------------------------------------------------------
// Default Payload
//------------------------------------------------------------------------------

#ifdef nx_DEFAULT_PAYLOAD

/*
   using PayloadUnit = u32;

   struct PayloadAggregate {
   u64 bitset{ 0 };
   };
 */

// @TODO(llins): upgrade to include context

static void
nx_PayloadAggregate_init(nx_PayloadAggregate *self, void *payload_context)
{
	self->bitset = 0;
}

static void
nx_PayloadAggregate_insert(nx_PayloadAggregate *self, void *unit, void *payload_context)
{
	u32 index = *((u32*) unit);
	Assert(index < 64 && "Invalid PayloadUnit");
	self->bitset |= (1ull << index);
}

static void
nx_PayloadAggregate_share(nx_PayloadAggregate *self, nx_PayloadAggregate *other, void *payload_context)
{
	// here we don't care about sharing since it
	// is cheap to make a whole copy u64
	self->bitset = other->bitset;
}

#endif


//------------------------------------------------------------------------------
// boilerplate code
//------------------------------------------------------------------------------

PTR_SPECIALIZED_SERVICES(nx_Ptr_Node,  nx_Node)
PTR_SPECIALIZED_SERVICES(nx_Ptr_Child, nx_Child)
PTR_SPECIALIZED_SERVICES(nx_Ptr_Label, nx_Label)

//------------------------------------------------------------------------------
// Labels
//------------------------------------------------------------------------------

static void
nx_Array_init(nx_Array *self, u8 *begin, u8 length)
{
	self->begin = begin;
	self->length = length;
}

static nx_Array
nx_Array_build(u8 *begin, u8 length)
{
	nx_Array result;
	nx_Array_init(&result, begin, length);
	return result;
}

static nx_Array
nx_Array_prefix(nx_Array *self, u8 length)
{
	Assert(length <= self->length);
	return nx_Array_build(self->begin, length);
}

static nx_Array
nx_Array_drop_prefix(nx_Array *self, u8 length)
{
	Assert(length <= self->length);
	return nx_Array_build(self->begin + length, (u8)(self->length - length));
}

static u8
nx_Array_get(nx_Array *self, u8 index)
{
	Assert(index < self->length);
	return *(self->begin + index);
}

static void
nx_Array_set(nx_Array *self, u8 index, u8 element)
{
	Assert(index < self->length);
	*(self->begin + index) = element;
}

static u8*
nx_Array_end(nx_Array *self)
{
	return self->begin + self->length;
}

static b8
nx_Array_is_equal(nx_Array *self, nx_Array *other)
{
	if (self->length != other->length)
		return 0;
	for (s32 i=0;i<self->length;++i) {
		if (*(self->begin+i) != *(other->begin+i))
			return 0;
	}
	return 1;
}

//------------------------------------------------------------------------------
// LabelArrayList
//------------------------------------------------------------------------------

static void
nx_LabelArrayList_init(nx_LabelArrayList *self, nx_Label *begin, u8 length, nx_LabelArrayList *next)
{
	nx_Array_init(&self->labels, begin, length);
	self->next = next;
}

// LabelArrayList LabelArrayList_build(Label *labels, u8 length, LabelArrayList *next) {
//	LabelArrayList result;
//	LabelArrayList_init(&result, labels, length, next);
//	return result;
// }

static nx_LabelArrayList
nx_LabelArrayList_build(nx_Array la, nx_LabelArrayList *next)
{
	nx_LabelArrayList result;
	nx_LabelArrayList_init(&result, la.begin, la.length, next);
	return result;
}

//-------------------------------------------------------------------------------
// List_NodeP
//-------------------------------------------------------------------------------

typedef nx_Node* nx_NodeP;

typedef struct {
	nx_NodeP *begin;
	nx_NodeP *end;
	nx_NodeP *capacity;
} nx_List_NodeP;

static nx_List_NodeP
nx_List_NodeP_build(nx_NodeP *begin, nx_NodeP *end, nx_NodeP *capacity)
{
	Assert(begin <= end && end <= capacity);
	nx_List_NodeP result;
	result.begin = begin;
	result.end = end;
	result.capacity = capacity;
	return result;
}

static void
nx_List_NodeP_init(nx_List_NodeP *self, nx_NodeP *begin, nx_NodeP *end, nx_NodeP *capacity)
{
	Assert(begin <= end && end <= capacity);
	self->begin = begin;
	self->end = end;
	self->capacity = capacity;
}

static nx_NodeP
nx_List_NodeP_get(nx_List_NodeP *self, s64 index)
{
	Assert(self->begin + index < self->end);
	return *(self->begin + index);
}

static nx_NodeP
nx_List_NodeP_back(nx_List_NodeP *self)
{
	Assert(self->begin < self->end);
	return *(self->end - 1);
}

static nx_NodeP
nx_List_NodeP_get_reverse(nx_List_NodeP *self, s64 index)
{
	Assert(self->begin + index < self->end);
	return *(self->end - 1 - index);
}

static nx_NodeP
nx_List_NodeP_front(nx_List_NodeP *self)
{
	Assert(self->begin < self->end);
	return *(self->begin);
}

static void
nx_List_NodeP_set(nx_List_NodeP *self, s64 index, nx_NodeP element)
{
	Assert(self->begin + index < self->end);
	*(self->begin + index) = element;
}

static void
nx_List_NodeP_set_reverse(nx_List_NodeP *self, s64 index, nx_NodeP element)
{
	Assert(self->begin + index < self->end);
	*(self->end - 1 - index) = element;
}

static s64
nx_List_NodeP_size(nx_List_NodeP *self)
{
	return (s64) (self->end - self->begin);
}

static s64
nx_List_NodeP_capacity(nx_List_NodeP *self)
{
	return self->capacity - self->begin;
}

static void
nx_List_NodeP_pop_back(nx_List_NodeP *self)
{
	Assert(self->end > self->begin);
	--self->end;
}

static void
nx_List_NodeP_push_back(nx_List_NodeP *self, nx_NodeP element)
{
	Assert(self->end < self->capacity);
	*self->end = element;
	++self->end;
}

static b8
nx_List_NodeP_empty(nx_List_NodeP *self)
{
	return self->begin == self->end;
}

static void
nx_List_NodeP_clear(nx_List_NodeP *self)
{
	self->end = self->begin;
}

static void
nx_List_NodeP_drop_prefix(nx_List_NodeP *self, s64 len)
{
	Assert(nx_List_NodeP_size(self) >= len);
	self->begin += len;
}

static void
nx_List_NodeP_drop_suffix(nx_List_NodeP *self, s64 len)
{
	Assert(nx_List_NodeP_size(self) >= len);
	self->end -= len;
}

//-------------------------------------------------------------------------------
// List_u8
//-------------------------------------------------------------------------------

typedef struct {
	u8 *begin;
	u8 *end;
	u8 *capacity;
} nx_List_u8;

static nx_List_u8
nx_List_u8_build(u8 *begin, u8 *end, u8 *capacity)
{
	Assert(begin <= end && end <= capacity);
	nx_List_u8 result;
	result.begin = begin;
	result.end = end;
	result.capacity = capacity;
	return result;
}

static void
nx_List_u8_init(nx_List_u8 *self, u8 *begin, u8 *end, u8 *capacity)
{
	Assert(begin <= end && end <= capacity);
	self->begin = begin;
	self->end = end;
	self->capacity = capacity;
}

static u8
nx_List_u8_get(nx_List_u8 *self, s64 index)
{
	Assert(self->begin + index < self->end);
	return *(self->begin + index);
}

static u8
List_u8_back(nx_List_u8 *self)
{
	Assert(self->begin < self->end);
	return *(self->end - 1);
}

static u8
nx_List_u8_get_reverse(nx_List_u8 *self, s64 index)
{
	Assert(self->begin + index < self->end);
	return *(self->end - 1 - index);
}

static u8
nx_List_u8_front(nx_List_u8 *self)
{
	Assert(self->begin < self->end);
	return *(self->begin);
}

static void
nx_List_u8_set(nx_List_u8 *self, s64 index, u8 element)
{
	Assert(self->begin + index < self->end);
	*(self->begin + index) = element;
}

static void
nx_List_u8_set_reverse(nx_List_u8 *self, s64 index, u8 element)
{
	Assert(self->begin + index < self->end);
	*(self->end - 1 - index) = element;
}

static s64
nx_List_u8_size(nx_List_u8 *self)
{
	return (s64) (self->end - self->begin);
}

static s64
nx_List_u8_capacity(nx_List_u8 *self)
{
	return self->capacity - self->begin;
}

static void
nx_List_u8_pop_back(nx_List_u8 *self)
{
	Assert(self->end > self->begin);
	--self->end;
}

static void
nx_List_u8_push_back(nx_List_u8 *self, u8 element)
{
	Assert(self->end < self->capacity);
	*self->end = element;
	++self->end;
}

static b8
nx_List_u8_empty(nx_List_u8 *self)
{
	return self->begin == self->end;
}

static void
nx_List_u8_clear(nx_List_u8 *self)
{
	self->end = self->begin;
}

static void
nx_List_u8_drop_prefix(nx_List_u8 *self, s64 len)
{
	Assert(nx_List_u8_size(self) >= len);
	self->begin += len;
}

static void
nx_List_u8_drop_suffix(nx_List_u8 *self, s64 len)
{
	Assert(nx_List_u8_size(self) >= len);
	self->end -= len;
}

//------------------------------------------------------------------------------
// DetailInfo
//------------------------------------------------------------------------------

// enum DetailType { DETAIL_c16p6, DETAIL_c0p22, DETAIL_c16pL, DETAIL_cLp16, DETAIL_cLpL };

typedef struct {
	/* flag if path is stored locally */
	u8  local_path : 1;
	/* flag if children is stored locally (0 is local and empty) */
	u8  local_children : 1;
	/* number of bits per label */
	u8  bits_per_label : 4; // 1 to 8 can be represented in 4 bits
	/* number of label slots on path trimmed to 128 */
	u8  path_capacity;
	/* number children slots available on current path storage < 256 */
	u16 children_capacity;
	/* number of bytes on the label slots storage */
	u16 path_storage_size;
	/* number of bytes on the children slots storage */
	u16 children_storage_size;
	/* type of detail */
	nx_DetailType detail_type;
} nx_DetailInfo;

static nx_DetailInfo nx_precomputed_detail_info[nx_DETAIL_COUNT][9];

/* this needs to be called beforehand */
static void
nx_initialize_precomputed_detail_info()
{
	for (s32 i=0;i<nx_DETAIL_COUNT;++i) {
		nx_DetailType detail_type = (nx_DetailType) i;
		for (s8 bits_per_label=1;bits_per_label<=8;++bits_per_label) {
			nx_DetailInfo *detail_info = &nx_precomputed_detail_info[i][bits_per_label];
			detail_info->bits_per_label = bits_per_label;
			if (detail_type == nx_DETAIL_c16p6) {
				/* all cases in binary trees should come this way */
				detail_info->detail_type = nx_DETAIL_c16p6;
				detail_info->local_path = 1;
				detail_info->local_children = 1;
				detail_info->path_storage_size = 6;
				detail_info->children_storage_size = 16;
				detail_info->children_capacity = 2;
				detail_info->path_capacity = (6 * 8) / bits_per_label;
			} else if (detail_type == nx_DETAIL_c0p22) {
				detail_info->detail_type = nx_DETAIL_c0p22;
				detail_info->local_path = 1;
				detail_info->local_children = 1;
				detail_info->path_storage_size = 22;
				detail_info->path_capacity = (22 * 8) / bits_per_label;
				detail_info->children_storage_size = 0;
				detail_info->children_capacity = 0;
			} else if (detail_type == nx_DETAIL_c16pL) {
				detail_info->detail_type = nx_DETAIL_c16pL;
				detail_info->local_path = 0;
				detail_info->local_children = 1;
				detail_info->children_storage_size = 16;
				detail_info->children_capacity = 2;
				/* defined when path_bytes is known */
				detail_info->path_storage_size = 0; // (u16) pt_normalize_msb2(path_bytes);
				detail_info->path_capacity = 0; // (u8) (detail_info.path_storage_size / bits_per_label);
			} else if (detail_type == nx_DETAIL_cLp16) {
				detail_info->detail_type = nx_DETAIL_cLp16;
				detail_info->local_path = 1;
				detail_info->local_children = 0;
				detail_info->path_storage_size = 16;
				detail_info->path_capacity = (16 * 8)/ bits_per_label;
				/* defined when children_bytes is known */
				detail_info->children_storage_size = 0; // (u16) pt_normalize_msb2(children_bytes);
				detail_info->children_capacity = 0; // (u8) (detail_info.children_storage_size / sizeof(nx_Child));
			} else if (detail_type == nx_DETAIL_cLpL) {
				detail_info->detail_type = nx_DETAIL_cLpL;
				detail_info->local_path = 0;
				detail_info->local_children = 0;
				/* defined when path_bytes and children_bytes are known */
				detail_info->path_storage_size = 0; // (u16) pt_normalize_msb2(path_bytes);
				detail_info->children_storage_size = 0; // (u16) pt_normalize_msb2(children_bytes);
				detail_info->children_capacity = 0; // (u8) (detail_info.children_storage_size / sizeof(nx_Child));
				detail_info->path_capacity = 0; //(u8) (detail_info.path_storage_size / bits_per_label);
			}
		}
	}
}


static inline void
nx_DetailInfo_precomputed_init(nx_DetailInfo *self, u32 path_bytes, u32 children_bytes, u8 bits_per_label)
{
	/* make sure we initialize all the fields of DetailInfo below */
	if (children_bytes <= 16 && path_bytes <= 6) {
		*self = nx_precomputed_detail_info[nx_DETAIL_c16p6][bits_per_label];
	} else if (children_bytes == 0 && path_bytes <= 22) {
		*self = nx_precomputed_detail_info[nx_DETAIL_c0p22][bits_per_label];
	} else if (children_bytes == 16 && path_bytes > 6) {
		*self = nx_precomputed_detail_info[nx_DETAIL_c16pL][bits_per_label];
		self->path_storage_size = (u16) pt_normalize_msb2(path_bytes);
		self->path_capacity     = (u8) (self->path_storage_size / bits_per_label);
	} else if (children_bytes > 16 && path_bytes <= 16) {
		*self = nx_precomputed_detail_info[nx_DETAIL_cLp16][bits_per_label];
		self->children_storage_size = (u16) pt_normalize_msb2(children_bytes);
		self->children_capacity     = (u16) (self->children_storage_size / sizeof(nx_Child));
	} else {
		*self = nx_precomputed_detail_info[nx_DETAIL_cLpL][bits_per_label];
		self->children_storage_size = (u16) pt_normalize_msb2(children_bytes);
		self->children_capacity     = (u16) (self->children_storage_size / sizeof(nx_Child));
		self->path_storage_size     = (u16) pt_normalize_msb2(path_bytes);
		self->path_capacity         = (u8) (self->path_storage_size / bits_per_label);
	}
}

#if 0
static nx_DetailInfo*
nx_DetailInfo_smart_init(nx_DetailInfo *self, s32 degree, u8 path_length, u8 bits_per_label)
{
	// if we find a cached vertion of the nx_DetailInfo we just
	// return it, since it won't be changed. Avoids copying
	// bytes in some cases

	u32 path_bytes     = (path_length * bits_per_label + 7) / 8;
	u32 children_bytes = (u32) (degree * sizeof(nx_Child));

	/* cannot be one child */
	Assert(children_bytes != sizeof(nx_Child));
	Assert(sizeof(nx_Child) == 8);
	Assert(sizeof(nx_Detail) == 22);
	Assert(bits_per_label <= 8);


	/* make sure we initialize all the fields of DetailInfo below */
	if (children_bytes <= 16 && path_bytes <= 6) {
		return &nx_precomputed_detail_info[nx_DETAIL_c16p6][bits_per_label];
	} else if (children_bytes == 0 && path_bytes <= 22) {
		return &nx_precomputed_detail_info[nx_DETAIL_c0p22][bits_per_label];
	} else if (children_bytes == 16 && path_bytes > 6) {
		*self = nx_precomputed_detail_info[nx_DETAIL_c16pL][bits_per_label];
		self->path_storage_size = (u16) pt_normalize_msb2(path_bytes);
		self->path_capacity = (u8) (self->path_storage_size / bits_per_label);
		return self;
	} else if (children_bytes > 16 && path_bytes <= 16) {
		*self = nx_precomputed_detail_info[nx_DETAIL_cLp16][bits_per_label];
		self->children_storage_size = (u16) pt_normalize_msb2(children_bytes);
		self->children_capacity = (u8) (self->children_storage_size / sizeof(nx_Child));
		return self;
	} else {
		*self = nx_precomputed_detail_info[nx_DETAIL_cLpL][bits_per_label];
		self->children_storage_size = (u16) pt_normalize_msb2(children_bytes);
		self->children_capacity     = (u16) (self->children_storage_size / sizeof(nx_Child));
		self->path_storage_size     = (u16) pt_normalize_msb2(path_bytes);
		self->path_capacity         = (u8) (self->path_storage_size / bits_per_label);
		return self;
	}
}
#endif

static void
nx_DetailInfo_init(nx_DetailInfo *self, s32 degree, u8 path_length, u8 bits_per_label)
{
	nx_pf_BEGIN_BLOCK("nx_Detail_info");

	u32 path_bytes     = (path_length * bits_per_label + 7) / 8;
	u32 children_bytes = (u32) (degree * sizeof(nx_Child));

	/* cannot be one child */
	Assert(children_bytes != sizeof(nx_Child));
	Assert(sizeof(nx_Child) == 8);
	Assert(sizeof(nx_Detail) == 22);
	Assert(bits_per_label <= 8);

	// Assert(sizeof(Detail) == 22);
	// Assert(sizeof(ptr::Ptr<Child>) == 6);

	// nx_DetailInfo detail_info;

	// NOTE(llins): the local path seems to be more cache friendly
#if 0
	/* make sure we initialize all the fields of DetailInfo below */
	self->bits_per_label = bits_per_label;
	if (children_bytes <= 16 && path_bytes <= 6) {
		/* all cases in binary trees should come this way */
		self->path_storage_size = 6;
		self->children_storage_size = 16;
		self->local_path = 1;
		self->local_children = 1;
		self->children_capacity = 2;
		self->path_capacity = (6 * 8) / bits_per_label;
		self->detail_type = nx_DETAIL_c16p6;
	} else if (children_bytes == 0 && path_bytes <= 22) {
		/* most leaves (0 children) shoud come this way */
		self->path_storage_size = 22;
		self->children_storage_size = 0;
		self->local_path = 1;
		self->local_children = 1;
		self->children_capacity = 0;
		self->path_capacity = (22 * 8) / bits_per_label;
		self->detail_type = nx_DETAIL_c0p22;
	} else if (children_bytes == 16 && path_bytes > 6) {
		self->path_storage_size = (u16) pt_normalize_msb2(path_bytes);
		self->children_storage_size = 16;
		self->local_path = 0;
		self->local_children = 1;
		self->children_capacity = 2;
		self->path_capacity = (u8) (self->path_storage_size / bits_per_label);
		self->detail_type = nx_DETAIL_c16pL;
	} else if (children_bytes > 16 && path_bytes <= 16) {
		self->path_storage_size = 16;
		self->children_storage_size = (u16) pt_normalize_msb2(children_bytes);
		self->local_path = 1;
		self->local_children = 0;
		self->children_capacity = (u8) (self->children_storage_size / sizeof(nx_Child));
		self->path_capacity = (16 * 8)/ bits_per_label;
		self->detail_type = nx_DETAIL_cLp16;
	} else {
		/* children_bytes > 16 && path_bytes > 6 */
		self->path_storage_size = (u16) pt_normalize_msb2(path_bytes);
		self->children_storage_size = (u16) pt_normalize_msb2(children_bytes);
		self->local_path = 0;
		self->local_children = 0;
		self->children_capacity = (u8) (self->children_storage_size / sizeof(nx_Child));
		self->path_capacity = (u8) (self->path_storage_size / bits_per_label);
		self->detail_type = nx_DETAIL_cLpL;
	}

#else
	nx_DetailInfo_precomputed_init(self, path_bytes, children_bytes, bits_per_label);
#endif

	nx_pf_END_BLOCK();
	// return detail_info;
}

//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

static void
nx_Node_set_degree(nx_Node *self, s32 degree)
{
	Assert(degree != 1 && degree >= 0 && degree <= 256);
	self->degree_ = (u8) ((degree > 0) ? (degree - 1) : 0);
}

static void
nx_Node_init(nx_Node *self, u8 path_length, s32 degree, b8 is_root)
{
	self->path_length = path_length;
	nx_Node_set_degree(self, degree);
	self->root = is_root;
	nx_Ptr_Node_set_null(&self->parent_p);
	// Note: leaving the self detail area in a dirty state!
}

static nx_Node*
nx_Node_parent(nx_Node *self)
{
	return nx_Ptr_Node_get(&self->parent_p);
}

static void
nx_Node_set_parent(nx_Node *self, nx_Node *parent)
{
	nx_Ptr_Node_set(&self->parent_p,parent);
}

static s32
nx_Node_degree(nx_Node *self)
{
	return (self->degree_ > 0) ? ((s32) self->degree_ + 1) : 0;
}

//------------------------------------------------------------------------------
// Auxiliar free functions
//------------------------------------------------------------------------------

static u8
nx_Node_check_common_path_length(nx_Node *self, nx_Array *labels, u8 bits_per_label, u8 suffix_length)
{
	// nx_pf_BEGIN_BLOCK("nx_Node_check_common_path_length");

	//
	// (case of shared shared suffixes)
	// consider only suffix_length of the labels in wnode
	//

	// @todo: optimize this?
	nx_DetailInfo detail_info;
	nx_DetailInfo_init(&detail_info, nx_Node_degree(self), self->path_length, bits_per_label);

	const char *compressed_labels = 0; //  (const char*)Node_detail(self);

	switch (detail_info.detail_type) {
	case nx_DETAIL_c16p6: {
		compressed_labels = (const char*) self->detail.c16p6.path;
	} break;
	case nx_DETAIL_c0p22: {
		compressed_labels = (const char*) self->detail.c0p22.path;
	} break;
	case nx_DETAIL_cLp16: {
		compressed_labels = (const char*) self->detail.cLp16.path;
	} break;
	case nx_DETAIL_c16pL: {
		compressed_labels = (const char*) nx_Ptr_Label_get(&self->detail.c16pL.path_p);
	} break;
	case nx_DETAIL_cLpL: {
		compressed_labels = (const char*) nx_Ptr_Label_get(&self->detail.cLpL.path_p);
	} break;
	default: {
		Assert(0 && "Not expected!");
	}
	}

	Assert(self->path_length >= suffix_length && "ooops assumption is wrong");

	u32 offset = (self->path_length > suffix_length
		      ? self->path_length - suffix_length
		      : 0) * bits_per_label;

	u8 n = Min(suffix_length, labels->length);
	u8 result = 0;


#if 1
	pt_BitStream istream;
	pt_BitStream_init(&istream,(void*)compressed_labels,1ull<<63,offset);
	for (u8 i=0;i<n;++i) {
		nx_Label lbl = pt_BitStream_read(&istream, bits_per_label);
		nx_Label input_label = nx_Array_get(labels, i);
		Assert((input_label < (1u<<bits_per_label)) && "input path label has too many bits");
		if (lbl != input_label) {
			break;
		}
		++result;
	}
#else
	for (u8 i=0;i<n;++i) {
		nx_Label lbl = 0;

		// nx_pf_BEGIN_BLOCK("pt_read_bits@chk_common_path_length");

		/*
		 * there is an overhead here that can be avoided
		 * iterate through fixed bit labels of a
		 * compressed array
		 */
		pt_read_bits2(compressed_labels, offset, bits_per_label, (char*) &lbl);

		// nx_pf_END_BLOCK();

		nx_Label input_label = nx_Array_get(labels, i);

		Assert((input_label < (1u<<bits_per_label))
		       && "input path label has too many bits");

		if (lbl != input_label) {
			break;
		}

		++result;
		offset += bits_per_label;
	}
#endif

	// nx_pf_END_BLOCK();

	return result;
}

//------------------------------------------------------------------------------
// Path
//------------------------------------------------------------------------------

static void
nx_Path_init(nx_Path *self, nx_Label *begin, u8 length, u8 capacity, nx_Bits bits_per_label)
{
	self->begin = begin;
	self->length = length;
	self->capacity = capacity;
	self->bits_per_label = bits_per_label;
	self->padding1 = 0;
	self->padding2 = 0;
}

static nx_Label
nx_Path_get(const nx_Path *self, u8 i)
{
	// nx_pf_BEGIN_BLOCK("nx_Path_get");
	Assert(i < self->length);
	nx_Label result = 0;
	pt_read_bits2((char*) self->begin, i * self->bits_per_label, self->bits_per_label, (char*) &result);
	// nx_pf_END_BLOCK();
	return result;
}

static void
nx_Path_set(nx_Path *self, u8 i, nx_Label label)
{
	Assert(i < self->length);
	pt_write_bits2((const char*) &label, i * self->bits_per_label, self->bits_per_label, (char*) self->begin);
}

static void
nx_Path_copy_range(nx_Path *self, const nx_Path *other, u8 offset, u8 length)
{
	Assert(self->capacity >= length);
	for (u8 i=0;i<length;++i) {
		nx_Path_set(self, i, nx_Path_get(other, offset + i));
	}
	self->length = length;
}

static inline void
nx_Path_copy(nx_Path *self, const nx_Path *other)
{
	nx_Path_copy_range(self, other, 0, other->length);
}

static inline nx_Bytes
nx_Path_bytes(const nx_Path *self)
{
	return (self->capacity * self->bits_per_label + 7u) / 8u;
}

static void
nx_Path_clear(nx_Path *self)
{
	u8 *end = (u8*) self->begin + nx_Path_bytes(self);
	u8 *it = self->begin;
	while (it != end) {
		*it = 0;
		++it;
	}
}




//------------------------------------------------------------------------------
// Child
//------------------------------------------------------------------------------

static void
nx_Child_init(nx_Child *self)
{
	self->shared = 0;
	self->suffix_length = 0;
	self->label = 0;
	nx_Ptr_Node_set_null(&self->node_p);
}

static inline nx_Node*
nx_Child_get_node(nx_Child *self)
{
	return nx_Ptr_Node_get(&self->node_p);
}

static inline void
nx_Child_set_node(nx_Child *self, nx_Node *node)
{
	nx_Ptr_Node_set(&self->node_p,node);
}

static inline b8
nx_Child_proper(nx_Child *self)
{
	return !self->shared;
}

static void
nx_Child_copy(nx_Child *self, nx_Child *other)
{
	self->shared = other->shared;
	self->suffix_length = other->suffix_length;
	self->label = other->label;
	nx_Ptr_Node_set(&self->node_p,nx_Ptr_Node_get(&other->node_p));
}

//------------------------------------------------------------------------------
// Children
//------------------------------------------------------------------------------

//
// Wraps a region reserved for a children w_node pointer with the functionality
// needed. Desgined to be used shortly every time there is a need to manipulate
// children (query or update)
//

static void
nx_Children_init(nx_Children *self, nx_Child *begin, s32 length, s32 capacity)
{
	Assert(length >= 0 && capacity >= length);
	self->begin    = begin;
	self->length   = length;
	self->capacity = capacity;
}

static inline nx_Child*
nx_Children_get_child(nx_Children *self, s32 i)
{
	Assert(i < self->length);
	return self->begin + i;
}

static inline nx_Node*
nx_Children_get_node(nx_Children *self, s32 i)
{
	Assert(i < self->length);
	return nx_Child_get_node(nx_Children_get_child(self,i));
}

static inline void
nx_Children_set_node(nx_Children *self, s32 i, nx_Node * node)
{
	Assert(i < self->length);
	nx_Child_set_node(nx_Children_get_child(self,i), node);
}

//
// it is not const on "other" because we copy pointers
//
static void
nx_Children_copy_range(nx_Children *self, nx_Children *other, s32 offset, s32 length)
{
	Assert(self->capacity >= length);
	self->length = length; // overwrite length
	for (s32 i=0;i<length;++i) {
		nx_Child_copy(nx_Children_get_child(self,i), nx_Children_get_child(other,offset+i));
		// set(i, other.get(offset + i));
	}
}

static inline void
nx_Children_copy(nx_Children *self, nx_Children *other)
{
	nx_Children_copy_range(self, other, 0, other->length);
}

static void
nx_Children_clear(nx_Children *self)
{
	for (s32 i=0;i<self->length;++i) {
		nx_Child_init(nx_Children_get_child(self,i));
	}
}

static s32
nx_Children_find(nx_Children *self, nx_Label start_label)
{
#if 0
	// [l,r)
	nx_Child *l = self->begin;
	nx_Child *r = self->begin + self->length;

	if (l == r) return -1;

	/* make sure l and r are always changing */
	while (r - l > 2) {
		/* m and m+1 are both different from l and r */
		nx_Child *m = l + (r-l)/2;
		if (start_label <= m->label) {
			r = m+1;
		} else {
			l = m;
		}
	}
	if (start_label < l->label) {
		return -1 - (l - self->begin);
	} else if (start_label == l->label) {
		return l - self->begin;
	} else {
		if (r - l == 2) {
			if (start_label < (l + 1)->label) {
				return -1 - (l + 1 - self->begin);
			} else if (start_label == (l + 1)->label) {
				return (l + 1) - self->begin; // this without parenthesis is weird
			}
		}
		return -1 - (r - self->begin);
	}
#else
	// [l,r)
	s32 l = 0;
	s32 r = self->length;
	nx_Child *x = self->begin;

	if (l == r) return -1;

	/* make sure l and r are always changing */
	while (r - l > 2) {
		/* m and m+1 are both different from l and r */
		s32 m = l + (r-l)/2;
		if (start_label <= x[m].label) {
			r = m+1;
		} else {
			l = m;
		}
	}
	u8 l_label = x[l].label;
	if (start_label < l_label) {
		return -1 - (l);
	} else if (start_label == l_label) {
		return l;
	} else {
		if (r - l == 2) {
			u8 l_plus_1_label = x[l+1].label;
			if (start_label < l_plus_1_label) {
				return -1 - (l+1);
			} else if (start_label == l_plus_1_label) {
				return l+1; // this without parenthesis is weird
			}
		}
		return -1 - (r);
	}
#endif
}

static nx_Child*
nx_Children_get_child_by_label(nx_Children *self, nx_Label label)
{
	s32 index = nx_Children_find(self, label);
	Assert(index >= 0);
	return nx_Children_get_child(self, index);
}


//
// this is still an ugly call: the label, suffix_length, shared flag,
// are all outdated
//

static void
nx_Children_insert(nx_Children *self, s32 index, nx_Child *new_child_info)
{
	Assert(self->capacity > self->length);
	++self->length; // slot at new last position is dirty
	if (self->length > 0) {
		for (s32 i=self->length-1;i>index;--i) {
			nx_Child_copy(nx_Children_get_child(self,i), nx_Children_get_child(self,i-1));
		}
	}
	nx_Child_copy(nx_Children_get_child(self,index), new_child_info);
}

static inline s32
nx_Children_available_capacity(nx_Children *self)
{
	return self->capacity - self->length;
}

//------------------------------------------------------------------------------
// INode
//------------------------------------------------------------------------------

static void
nx_INode_init(nx_INode *self, u8 path_length, s32 degree, b8 is_root)
{
	nx_Node_init((nx_Node*) self, path_length, degree, is_root);
	nx_Ptr_Node_set_null(&self->content_p);
}

static nx_Node*
nx_INode_content(nx_INode *self)
{
	return nx_Ptr_Node_get(&self->content_p);
}

static void
nx_INode_share_content(nx_INode *self, nx_INode *other)
{
	nx_Ptr_Node_set(&self->content_p,nx_INode_content(other));
}

static void
nx_INode_own_content(nx_INode *self, nx_Node *content)
{
	nx_Ptr_Node_set(&self->content_p,content);
}

//------------------------------------------------------------------------------
// NodeWrap
//------------------------------------------------------------------------------

static void
nx_NodeWrap_init(nx_NodeWrap *self, nx_Node *raw_node, const nx_DetailInfo *detail_info)
{
	nx_pf_BEGIN_BLOCK("nx_NodeWrap_init");

	self->raw_node   = raw_node;

	self->children_class = detail_info->local_children ? 0 : detail_info->children_storage_size;

	self->path_class = detail_info->local_path ? 0 : detail_info->path_storage_size;

	self->detail_type = detail_info->detail_type;

	if (detail_info->detail_type == nx_DETAIL_c16p6) {

		nx_Children_init(&self->children, raw_node->detail.c16p6.children, nx_Node_degree(raw_node), detail_info->children_capacity);

		nx_Path_init(&self->path, raw_node->detail.c16p6.path, raw_node->path_length, detail_info->path_capacity, detail_info->bits_per_label);

	} else if (detail_info->detail_type == nx_DETAIL_c0p22) {

		nx_Children_init(&self->children, (nx_Child*) &raw_node->detail.raw, 0, 0);

		nx_Path_init(&self->path, raw_node->detail.c0p22.path, raw_node->path_length, detail_info->path_capacity, detail_info->bits_per_label);

	} else if (detail_info->detail_type == nx_DETAIL_c16pL) {

		nx_Children_init(&self->children, raw_node->detail.c16pL.children, nx_Node_degree(raw_node), detail_info->children_capacity);

		nx_Path_init(&self->path, nx_Ptr_Label_get_not_null(&raw_node->detail.c16pL.path_p), raw_node->path_length, detail_info->path_capacity, detail_info->bits_per_label);

	} else if (detail_info->detail_type == nx_DETAIL_cLp16) {

		// @BUG there is an issue while loading the mts
		// dataset around ~6.3m records where this
		// assertion is being triggered
		Assert(nx_Ptr_Child_is_not_null(&raw_node->detail.cLp16.children_p));

		nx_Children_init(&self->children, nx_Ptr_Child_get_not_null(&raw_node->detail.cLp16.children_p), nx_Node_degree(raw_node), detail_info->children_capacity);

		nx_Path_init(&self->path, raw_node->detail.cLp16.path, raw_node->path_length, detail_info->path_capacity, detail_info->bits_per_label);

	} else if (detail_info->detail_type == nx_DETAIL_cLpL) {

		nx_Children_init(&self->children, nx_Ptr_Child_get_not_null(&raw_node->detail.cLpL.children_p), nx_Node_degree(raw_node), detail_info->children_capacity);

		nx_Path_init(&self->path, nx_Ptr_Label_get_not_null(&raw_node->detail.cLpL.path_p), raw_node->path_length, detail_info->path_capacity, detail_info->bits_per_label);

	} else {

		nx_Children_init(&self->children,0,0,0);

		nx_Path_init(&self->path,0,0,0,0);

		Assert(0 && "Bug");

	}

	nx_pf_END_BLOCK();
}

//------------------------------------------------------------------------------
// PNode
//------------------------------------------------------------------------------

static void
nx_PNode_init(nx_PNode *self, u8 path_length, s32 degree, b8 is_root, void *payload_context)
{
	nx_Node_init((nx_Node*)self, path_length, degree, is_root);
	nx_PayloadAggregate_init(&self->payload_aggregate, payload_context);
}

static void
nx_PNode_payload_insert(nx_PNode *self, void *payload_unit, void *payload_context)
{
	nx_PayloadAggregate_insert(&self->payload_aggregate, payload_unit, payload_context);
}

static void
nx_PNode_payload_share(nx_PNode *self, nx_PNode *other, void *payload_context)
{
	nx_PayloadAggregate_share(&self->payload_aggregate, &other->payload_aggregate, payload_context);
}

//------------------------------------------------------------------------------
// Caches
//------------------------------------------------------------------------------

static inline u32
nx_size_class_to_index(nx_SizeClass s)
{
	u32 msb = pt_msb32(s);
	return (s > 1) * ( (msb-1) * 2 - ( ( (1 << (msb-2)) & s ) == 0 ) );
}

static char*
nx_print_u32(char *begin, char *end, u32 x)
{
	Assert(begin <= end);
	char *it = begin;
	while (x > 0 && it != end) {
		u32 d = x % 10;
		x = x/10;
		*it = (char)('0' + d);
		++it;
	}
	// reverse
	s32 i = (s32) (it - begin)/2 - 1;
	while (i >= 0) {
		char c = *(begin + i);
		*(begin + i) = *(it - 1 - i);
		*(it - 1 - i) = c;
		--i;
	}
	return it;
}

static char*
nx_print_cstr(char *begin, char *end, const char *cstr)
{
	Assert(begin <= end);
	char *it = begin;
	while (it != end && *cstr != 0) {
		*it = *cstr;
		++it;
		++cstr;
	}
	if (it != end) { *it = 0;}
	else { *(end-1) = 0; }
	return it;
}

static char*
nx_print_cstr_u32(char *begin, char *end, const char *cstr, u32 x)
{
	Assert(begin <= end);
	// @todo improve this
	char *it = nx_print_cstr(begin, end, cstr);
	it = nx_print_u32(it, end, x);
	if (it != end) { *it = 0;}
	else { *(end-1) = 0; }
	return it;
}

static void
nx_Caches_init(nx_Caches *self, al_Allocator *allocator)
{
	Assert(allocator && "Caches_init problem");
	char name[32];
	for (u32 i=0u;i<nx_Caches_NumClasses;++i) {
		nx_print_cstr_u32(name, name + 32, "detail", (u32) nx_Caches_Classes[i]);

		u32 s = Max(8u,nx_Caches_Classes[i]);  // smaller classes have 8 bytes
		al_Ptr_Cache_set(&self->caches[i], al_Allocator_create_cache(allocator, name, s));
	}
	self->initialized = 1;
}

static void*
nx_Caches_alloc(nx_Caches *self, nx_SizeClass s)
{
	// std::cerr << "Caches::alloc(" << s << ")" << std::endl;
	// figure out size class index
	u32 index = nx_size_class_to_index(s);
	Assert(index < nx_Caches_NumClasses);
	return al_Cache_alloc(al_Ptr_Cache_get(&self->caches[index]));
}

static void
nx_Caches_free(nx_Caches *self, nx_SizeClass s, void *p)
{
	// std::cerr << "Caches::free(" << s << "," << p << ")" << std::endl;
	u32 index = nx_size_class_to_index(s);
	Assert(index < nx_Caches_NumClasses);
	al_Cache_free(al_Ptr_Cache_get_not_null(&self->caches[index]), p);
}

//
// singleton should only be called assuming:
//
// 1. all threads are in the same dimension
// 2. they are all minimally finer version (ie. one additional constraint in
//    a single dimension) of the same address.
//
// For a w_node u and a prefix_length l_u (where l_u is
// equal or smaller than the path of u). Let
// depth_list(u,l_u) be the list of of depths
// to each dimension's root_p going upstream towards
// the global root_p of the nested hierarchy.
//
// Note that depth_list of the heads (and corresponding
// prefix_lengths) are lists of equal length (since
// they start in the same dimension).
//
// Proposition: There is a dominant thread starting at w_node
// u and l_u if and only if one depth_list(u,l_u) is entry-wise
// smaller or equal to all other depth_list(v,l_v) that are heads
// in the Threads
//
//

typedef struct {
	s32 length;
	s32 index;
	s32 *depths; // max dimension
} nx_DepthList;


// Fills the buffer prefix with a certain amount of numbers
// and returns the next available char* in the buffer.
// It crashes if not enough buffer memory is available.
static char*
nx_DepthList_init(nx_DepthList *self, s32 index, nx_Node* u, s32 initial_prefix, char *buffer_begin, char *buffer_end)
{
	s32 *begin    = (s32*) (RAlign((u64) buffer_begin, sizeof(s32)));
	s32 *capacity = (s32*) (LAlign((u64) buffer_end,   sizeof(s32)));
	Assert(begin < capacity);

	s32 len = 0;
	s32 *it = begin;
	*it = initial_prefix;
	if (u->root) {
		++len;
		++it;
		// prepare next slot with zero if we have
		// enough memory for next slot
		Assert(it < capacity);
		*it = 0;
	}
	nx_Node *node = nx_Node_parent(u);
	while (node) {
		*it += node->path_length;
		if (node->root) {
			++len;
			++it;
			Assert(it < capacity);
			*it = 0;
		}
		node = nx_Node_parent(node);
	}

	// returns position of the buffer
	self->index  = index;
	self->length = len;
	self->depths = begin;

	// pointer to the last used resource
	return (char*) it;
}

//
// following the same spec as std algorithm library
// [begin,nbegin) and [nbegin,end) are valid ranges
//
static void
nx_DepthList_rotate(nx_DepthList *begin, nx_DepthList *nbegin, nx_DepthList *end)
{
	nx_DepthList tmp;
	if (nbegin == end)
		return;
	nx_DepthList* next = nbegin;
	while (begin != next) {
		tmp    = *begin;
		*begin = *next;
		*next  = tmp;

		++begin;
		++next;

		if (next == end) {
			next = nbegin;
		} else if (begin == nbegin) {
			nbegin = next;
		}
	}
}


// internal void
// nx_prepare_depth_list(nx_Node *u, s32 initial_prefix, char *buffer_)
// {
// }
//
// internal void
// nx_DepthList_init(nx_DepthList *self)
// {
// 	self->length = 0;
// }
//
// internal void
// nx_DepthList_reset(nx_DepthList *self, nx_Node *u, s32 initial_prefix)
// {
// 	self->length = 0;
// 	self->depth[self->length] = initial_prefix;
// 	if (u->root) {
// 		++self->length;
// 		self->depth[self->length] = 0;
// 	}
// 	nx_Node *node = nx_Node_parent(u);
// 	while (node) {
// 		self->depth[self->length] += node->path_length;
// 		if (node->root) {
// 			++self->length;
// 			self->depth[self->length] = 0;
// 		}
// 		node = nx_Node_parent(node);
// 	}
// }
//

static b8
nx_DepthList_entry_wise_coarser_or_equal(nx_DepthList *self, nx_DepthList *other)
{
	// assumes lengths are the same, crash if not the same
	Assert(self->length == other->length);
	if (self->length != other->length) return 0;
	for (s32 i = 0; i < self->length; ++i) {
		if (self->depths[i] > other->depths[i])
			return 0;
	}
	return 1;
}

// all entries are <= with at least one begin <
#define nx_DepthList_CMP_COARSER -1
// all entries are ==
#define nx_DepthList_CMP_EQUAL 0
// all entries are >= with at least one begin >
#define nx_DepthList_CMP_FINER 1
// none of the above
#define nx_DepthList_CMP_NONE 3
static s32
nx_DepthList_compare(nx_DepthList *self, nx_DepthList *other)
{
	// assumes lengths are the same, crash if not the same
	Assert(self->length == other->length);
	s32 len  = self->length;
	s32 sign = nx_DepthList_CMP_EQUAL;
	for (s32 i = 0; i < len; ++i) {
		if (self->depths[i] < other->depths[i]) {
			if (sign == nx_DepthList_CMP_FINER) {
				return nx_DepthList_CMP_NONE;
			} else {
				sign = nx_DepthList_CMP_COARSER;
			}
		} else if (self->depths[i] > other->depths[i]) {
			if (sign == nx_DepthList_CMP_COARSER) {
				return nx_DepthList_CMP_NONE;
			} else {
				sign = nx_DepthList_CMP_FINER;
			}
		}
	}
	return sign;
}

static void
nx_DepthList_swap(nx_DepthList *a, nx_DepthList *b)
{
	nx_DepthList tmp = *a;
	*a = *b;
	*b = tmp;
}

//------------------------------------------------------------------------------
// NanocubeIndex basic service
//------------------------------------------------------------------------------

//
// @NOTE(llins): 2017-06-20T16:06
//
// this is the 1st. highest bottleneck in test run using perf top
//
static nx_NodeWrap
nx_NanocubeIndex_to_node(nx_NanocubeIndex *self, nx_Node *raw_node, s32 index)
{
	nx_NodeWrap node_wrap;
	// @NOTE(llins): 2017-06-19T16:54
	// running 'perf top' we find that the way we are getting the detailed info
	// of a node from the precomuted stuff is in the critical path of the
	// insertion procedure
	//
	//
	//
	nx_pf_BEGIN_BLOCK("to_node");

	nx_pf_BEGIN_BLOCK("detail_info");
	nx_DetailInfo detail_info;
	nx_DetailInfo_init(&detail_info, nx_Node_degree(raw_node), raw_node->path_length, self->bits_per_label[index]);
	nx_pf_END_BLOCK();

	nx_pf_BEGIN_BLOCK("init_node_wrap");
	nx_NodeWrap_init(&node_wrap, raw_node, &detail_info);
	nx_pf_END_BLOCK();

	nx_pf_END_BLOCK();

	return node_wrap;
}

//--------
// Thread
//--------

/*
 * Will be used as a minimally finer path.
 * Null pointers on the path will represent
 * dimension jumps
 *
 * at any moment, we are positioned from 1 ot "n"
 * labels into the path of the current whead. The
 * is when the whead is the root_p of a hierarchy.
 * in that case we might be positioned 0 labels
 * into the root_p path (which might have length 0).
 *
 * when traversing shared edges, the suffix length might
 * be shorter current w_node's length, offset indicates
 * where we should be start consuming labels of
 * the w_node's path.
 *
 * note that this start position might even go
 * the parent of the current w_node
 */

#define nx_Thread_ARRAY_CAPACITY 1000

typedef struct {

	/* hierarchy */
	nx_NanocubeIndex *hierarchy;

	/* dimension of the thread's head */
	s32  head_dimension;

	/* dimension of the 0-th entry in prefix_size and head_depth */
	s32  start_dimension;

	/* numbers of labels used by the head node at the i-th dimension */
	u8   head_used_labels_by_dim[nx_MAX_PATH_LENGTH];

	/* depth of the head at i-th dimension */
	u8   head_depth_by_dim[nx_MAX_PATH_LENGTH];

	/* offset on prefix_size and head_depth where the head is located */
	u8   dim_offset;

	/* offset on path where this thread's head is located */
	u16  head_index;

	/*
	 * Number of labels we skip at the i-th node before consuming labels.
	 * Used when we traverse a shared edge (see suffix length on nx_Child).
	 */
	u8   offset[nx_Thread_ARRAY_CAPACITY];

	/*
	 * Sequence of nodes starting in start dimension forward.
	 * 0 is unsed a a separator.
	 */
	nx_Node *path[nx_Thread_ARRAY_CAPACITY]; // at most 128 dimensions of length 256 and 127 separators

} nx_Thread;

static void
nx_Thread_init(nx_Thread *self, nx_NanocubeIndex *h, nx_Node *start, s32 start_dimension)
{
	Assert(start);
	Assert(start->root);

	self->hierarchy	                 = h;
	self->head_index                 = 0;
	self->dim_offset                 = 0;
	self->head_used_labels_by_dim[0] = 0;
	self->head_depth_by_dim[0]       = 0;

	// offset: a positive number from the current w_node's
	// initial symbol.
	self->offset[0]	  = 0;
	self->path[0]	  = start;

	self->head_dimension  = start_dimension;
	self->start_dimension = start_dimension;
}

/* Plan to use this routine for sanity checks only */
static void
nx_Thread_head_path_labels(nx_Thread *self, nx_List_u8 *output)
{
	Assert(output);
	nx_List_u8_clear(output);

	s32 i = self->head_index;
	for (;;) {
		nx_Node *u = self->path[i];
		u8 u_off = self->offset[i];
		u8 u_len = (i == self->head_index)
			? self->head_used_labels_by_dim[self->dim_offset]
			: (u->path_length - u_off);

		nx_NodeWrap u_wrap = nx_NanocubeIndex_to_node(self->hierarchy, u, self->head_dimension);

		for (s32 j=u_off + u_len - 1; j >= u_off; --j) {
			nx_Label lbl = nx_Path_get(&u_wrap.path, j);
			nx_List_u8_push_back(output, lbl);
		}

		--i;
		/* if underflow or previous node is null, we are done */
		/* could also check using the head depth info */
		if (i < 0 || self->path[i] == 0)
			break;
	}
	pt_reverse((char*) output->begin, (char*) output->end);
}

/* Plan to use this routine for sanity checks only */
static void
nx_NanocubeIndex_proper_path_to_root(nx_NanocubeIndex *self, u8 dimension, nx_Node *node, u8 node_prefix, nx_List_u8 *output)
{
	Assert(output);
	nx_List_u8_clear(output);

	nx_Node *u = node;
	Assert(u);
	u8 u_len = node_prefix;
	Assert(u->path_length >= u_len);
	for (;;) {
		nx_NodeWrap u_wrap = nx_NanocubeIndex_to_node(self, u, dimension);
		for (s32 j=u_len - 1; j >= 0; --j) {
			nx_Label lbl = nx_Path_get(&u_wrap.path, j);
			nx_List_u8_push_back(output, lbl);
		}

		if (u->root)
			break;

		u = nx_Ptr_Node_get(&u->parent_p);
		Assert(u);
		u_len = u->path_length;
	}
	pt_reverse((char*) output->begin, (char*) output->end);
}

static inline u8
nx_Thread_head_depth(nx_Thread *self)
{
	return self->head_depth_by_dim[self->dim_offset];
}

static inline u8
nx_Thread_head_prefix_size(nx_Thread *self)
{
	return self->head_used_labels_by_dim[self->dim_offset];
}

static inline u8
nx_Thread_head_offset(nx_Thread *self)
{
	return self->offset[self->head_index];
}

static inline nx_Node*
nx_Thread_head(nx_Thread *self)
{
	return self->path[self->head_index];
}

static void
nx_Thread_path_append(nx_Thread *self, nx_Node *node, u8 node_offset)
{
	Assert(self->head_index + 1 < nx_Thread_ARRAY_CAPACITY);
	++self->head_index;
	self->path[self->head_index]   = node;
	self->offset[self->head_index] = node_offset;
}

/*
 * the requested path will always exist since the current
 * record was already inserted in the finer address
 */
static void
nx_Thread_advance(nx_Thread *self, nx_Array array)
{
	if (array.length == 0)
		return;

	u8* it  = array.begin;
	u8* end = nx_Array_end(&array);

	nx_NodeWrap head = nx_NanocubeIndex_to_node(self->hierarchy, self->path[self->head_index], self->head_dimension);

	u8 no_labels	= (u8) (end - it);

	/* advance head_depth by len in a single shot */
	self->head_depth_by_dim[self->dim_offset] += no_labels;

	u8* head_used_labels = &self->head_used_labels_by_dim[self->dim_offset];

	while (it != end) {

		u8 offset = self->offset[self->head_index]; // current offset

		//
		// | <--- offset ---> | <--- prefix_size ---> | <--- no_labels ---> |
		//
		// | <---------------------- path_length -------------------------> |
		//
		//

		s32 excess_labels = (s32) (offset + *head_used_labels + no_labels) - (s32) head.path.length;

		if (excess_labels <= 0) {

			*head_used_labels += no_labels;

			it = end;

		} else {

			u8 used_labels = head.path.length - *head_used_labels - offset;

			no_labels -= used_labels;

			it += used_labels;

			//
			// might be traversing a shared edge (assumes it always works)
			//
			nx_Child *child_slot = nx_Children_get_child_by_label(&head.children, *it);

			nx_Node *child = nx_Child_get_node(child_slot); // walking through a shorter suffix...

			Assert(child->path_length >= child_slot->suffix_length);

			u8 new_offset = child->path_length - child_slot->suffix_length;

// 			s32 new_offset = 0; // prepare next offset
//
// 			/* This is messed up! */
// 			if (child_slot->shared) {
// 				// normalize location in case we overflow going backwards
// 				s32 no_labels_upstream = (s32) child_slot->suffix_length;
// 				new_offset = (s32) child->path_length - no_labels_upstream;
//
// 				while (new_offset < 0) {
// 					no_labels_upstream -= (s32) child->path_length;
// 					child = nx_Node_parent(child);
// 					new_offset = (s32) child->path_length - no_labels_upstream;
// 				}
// 			}

			nx_Thread_path_append(self, child, new_offset);

			head = nx_NanocubeIndex_to_node(self->hierarchy, child, self->head_dimension);

			/*
			 * Replace used labels on the head node by 0.
			 * Haven't consumed any label of the new head just appended.
			 */
			*head_used_labels = 0;
		}
	}

	Assert(*head_used_labels > 0);

}

static void
nx_Thread_rewind(nx_Thread *self, u8 len)
{
	nx_Node *head = self->path[self->head_index];

	u8* head_used_labels = &self->head_used_labels_by_dim[self->dim_offset];

	/* rewind head_depth by len in a single shot */
	Assert(self->head_depth_by_dim[self->dim_offset] >= len);
	self->head_depth_by_dim[self->dim_offset] -= len;

	while (len > 0) {

		if (len < *head_used_labels) {

			*head_used_labels -= len;

			break;

		} else if (len == *head_used_labels && (self->head_index == 0 || self->path[self->head_index-1] == 0)) {

			// special case when we are rewinding to the root_p of a hierarchy
			*head_used_labels = 0;

			break;

		} else {
			Assert(self->head_index > 0);

			/* rewind to previous node using all labels after offset */
			--self->head_index;

			Assert(self->path[self->head_index] != 0);

			u8 offset = self->offset[self->head_index];
			head      = self->path[self->head_index];

			len -= *head_used_labels; // subtract prefix size

			*head_used_labels = (s32) head->path_length - (s32) offset;
		}
	}
}

static void
nx_Thread_goto_next_dimension(nx_Thread *self)
{
	Assert(self->head_dimension < self->hierarchy->dimensions-1);
	nx_Node *new_head = nx_INode_content((nx_INode*) self->path[self->head_index]);

	/* append separation tag on path */
	nx_Thread_path_append(self, 0, (u8) 0xffu);

	/* append root of the new dimension. offset of roots are always zero. */
	nx_Thread_path_append(self, new_head, 0);

	++self->head_dimension;
	++self->dim_offset;

	self->head_depth_by_dim[self->dim_offset]  = 0;
	self->head_used_labels_by_dim[self->dim_offset] = 0;
}

static void
nx_Thread_goto_prev_dimension(nx_Thread *self)
{
	Assert(self->head_used_labels_by_dim[self->dim_offset] == 0
	       && self->head_index > 1 && self->path[self->head_index-1] == 0);
	--self->dim_offset;
	--self->head_dimension;
	self->head_index -= 2;
}

// @todo 2017-06-29T23:04
// consider implementing a compare function equivalent
// to the nx_DepthList_compare. What extra data
// needs to be kept in an nx_Thread?
//
#if 0
// all entries are <= with at least one begin <
#define nx_DepthList_CMP_COARSER -1
// all entries are ==
#define nx_DepthList_CMP_EQUAL 0
// all entries are >= with at least one begin >
#define nx_DepthList_CMP_FINER 1
// none of the above
#define nx_DepthList_CMP_NONE 3
static s32
nx_Thread_compare(nx_Thread *self, nx_Thread *other)
{
}
#endif

//---------
// Threads
//---------

typedef struct {
	nx_NanocubeIndex *hierarchy;
	nx_Thread threads[nx_MAX_NUM_DIMENSIONS];
	u32    size: 31;
	u32    initialized: 1;
} nx_Threads;

static void
nx_Threads_init(nx_Threads *self, nx_NanocubeIndex *h)
{
	Assert(self->initialized == 0);
	self->hierarchy = h;
	self->size = 0;
	self->initialized = 1;
}

static void
nx_Threads_goto_next_dimension(nx_Threads *self)
{
	for (u32 i=0;i<self->size;++i) {
		nx_Thread_goto_next_dimension(&self->threads[i]);
	}
}

static void
nx_Threads_goto_prev_dimension(nx_Threads *self)
{
	for (u32 i=0;i<self->size;++i) {
		nx_Thread_goto_prev_dimension(&self->threads[i]);
	}
}

static void
nx_Threads_pop_thread(nx_Threads *self)
{
	Assert(self->size > 0);
	--self->size;
}

static nx_Thread*
nx_Threads_back(nx_Threads *self)
{
	Assert(self->size > 0);
	return &self->threads[self->size - 1];
}

static void
nx_Threads_advance(nx_Threads *self, nx_Array labels)
{
	for (s32 i=0;i<(s32)self->size;++i) {
		nx_Thread_advance(&self->threads[i], labels);
	}
}

static void
nx_Threads_rewind(nx_Threads *self, u8 len)
{
	for (s32 i=0;i<(s32)self->size;++i) {
		nx_Thread_rewind(&self->threads[i], len);
	}
}

static void
nx_Threads_push_thread(nx_Threads *self, nx_Node *root, s32 dimension)
{
	Assert(self->size + 1 < nx_MAX_NUM_DIMENSIONS);
	nx_Thread_init(&self->threads[self->size], self->hierarchy, root, dimension);
	++self->size;
}

//
// @Performance 2017-06-20T16:06
// This is the 2nd. highest bottleneck in test run using perf top.
// The evidence is that the majority of the time is spent initializing
// the depth lists.
//
static nx_Thread*
nx_Threads_singleton_naive_stack(nx_Threads *self, BilinearAllocator *temp_storage)
{
	Assert(self->size > 0);
	if (self->size == 1) {
		return &self->threads[0];
	}

	s32 depths[2][nx_MAX_NUM_DIMENSIONS+1];
	nx_DepthList depth_list[2];
	depth_list[0].depths = depths[0];
	depth_list[1].depths = depths[1];

	nx_DepthList *dominant_list   = depth_list;
	nx_DepthList *candidate_list  = depth_list + 1;

	for (u32 i=0;i<self->size;++i) {
		nx_Thread    *dominant_thread = self->threads + i;

		s32 *dominant_buffer_begin = (s32*) dominant_list->depths;
		s32 *dominant_buffer_end   = dominant_buffer_begin + nx_MAX_NUM_DIMENSIONS + 1;
		nx_DepthList_init(dominant_list,
				  i,
				   nx_Thread_head(dominant_thread),
				   nx_Thread_head_offset(dominant_thread) + nx_Thread_head_prefix_size(dominant_thread),
				   (char*) dominant_buffer_begin,
				   (char*) dominant_buffer_end);

		for (u32 j=0;j<self->size;++j) {
			if (i == j)
				continue;
			nx_Thread *candidate_thread = self->threads + j;

			// should work even without this test:
			if (nx_Thread_head(candidate_thread) == nx_Thread_head(dominant_thread))
				continue; // pointing to the same w_node

			s32 *candidate_buffer_begin = (s32*) candidate_list->depths;
			s32 *candidate_buffer_end   = candidate_buffer_begin + nx_MAX_NUM_DIMENSIONS + 1;
			nx_DepthList_init(candidate_list,
					  j,
					  nx_Thread_head(candidate_thread),
					  nx_Thread_head_offset(candidate_thread) + nx_Thread_head_prefix_size(candidate_thread),
					  (char*) candidate_buffer_begin,
					  (char*) candidate_buffer_end);

// 			nx_DepthList_reset(candidate_list,
// 					   nx_Thread_head(candidate_thread),
// 					   nx_Thread_head_offset(candidate_thread) + nx_Thread_head_prefix_size(candidate_thread));

			if (!nx_DepthList_entry_wise_coarser_or_equal(dominant_list,candidate_list)) {
				dominant_thread = 0;
				break;
			}
		}
		if (dominant_thread)
			return dominant_thread;
	}
	return 0;

}

static nx_Thread*
nx_Threads_singleton_naive_temp_storage(nx_Threads *self, BilinearAllocator *temp_storage)
{
	Assert(self->size > 0);
	if (self->size == 1) {
		return &self->threads[0];
	}

	// prepare all depth lists
	BilinearAllocatorCheckpoint temp_storage_check_point = BilinearAllocator_left_checkpoint(temp_storage);
	nx_DepthList *depth_lists = (nx_DepthList*) BilinearAllocator_alloc_left_aligned(temp_storage, self->size * sizeof(nx_DepthList), 8);

	// use the bilinear allocate free memory to initialize the
	// depth lists of variable sizes
	{
		MemoryBlock free_memblock = BilinearAllocator_free_memblock(temp_storage);
		char *begin = (char*) RAlign( (u64) free_memblock.begin, 8 );
		char *end   = (char*) LAlign( (u64) free_memblock.end,   8 );
		Assert(begin < end);
		char *it    = begin;
		for (u32 i=0;i<self->size;++i) {
			nx_Thread *thread = self->threads + i;
			char *next_it = nx_DepthList_init(depth_lists + i,
							  i,
							  nx_Thread_head(thread),
							  nx_Thread_head_offset(thread) + nx_Thread_head_prefix_size(thread),
							  it, end);
			it = next_it;
		}
	}

	nx_Thread *result = 0;

	for (u32 i=0;i<self->size;++i) {
		nx_DepthList *dominant_list   = depth_lists + i;
		nx_Thread    *dominant_thread = self->threads + i;
		for (u32 j=0;j<self->size;++j) {
			if (i == j) {
				continue;
			}
			nx_DepthList *candidate_list   = depth_lists + j;
			nx_Thread    *candidate_thread = self->threads + j;
			// should work even without this test:
			if (nx_Thread_head(candidate_thread) == nx_Thread_head(dominant_thread)) {
				continue; // pointing to the same w_node
			}
			if (!nx_DepthList_entry_wise_coarser_or_equal(dominant_list,candidate_list)) {
				dominant_thread = 0;
				break;
			}
		}
		if (dominant_thread) {
			result = dominant_thread;
			break;
		}
	}

	// clearup the temp storage used
	BilinearAllocator_rewind(temp_storage, temp_storage_check_point);

#if 0
	// check for a match they need to match
	nx_Thread *check = nx_Threads_singleton_naive_stack(self, temp_storage);
	Assert(result == check);
#endif

	return result;
}


static nx_Thread*
nx_Threads_singleton_optimzed_temp_storage(nx_Threads *self, BilinearAllocator *temp_storage)
{
#if 0
	return nx_Threads_singleton_naive_stack_storage(self, temp_storage);
#else
	Assert(self->size > 0);
	if (self->size == 1) {
		return &self->threads[0];
	}

	// on this tentative, we want to decide more efficiently
	// by using a more expressive comparison procedure

	// prepare all depth lists
	BilinearAllocatorCheckpoint temp_storage_check_point = BilinearAllocator_left_checkpoint(temp_storage);
	nx_DepthList *depth_lists = (nx_DepthList*) BilinearAllocator_alloc_left_aligned(temp_storage, self->size * sizeof(nx_DepthList), 8);
	{
		// initalize depth_lists
		// use the bilinear allocate free memory to initialize the
		// depth lists of variable sizes
		MemoryBlock free_memblock = BilinearAllocator_free_memblock(temp_storage);
		char *begin = (char*) RAlign( (u64) free_memblock.begin, 8 );
		char *end   = (char*) LAlign( (u64) free_memblock.end,   8 );
		Assert(begin < end);
		char *it    = begin;
		for (u32 i=0;i<self->size;++i) {
			nx_Thread *thread = self->threads + i;
			char *next_it = nx_DepthList_init(depth_lists + i,
							  i, // <-- index
							  nx_Thread_head(thread),
							  nx_Thread_head_offset(thread) + nx_Thread_head_prefix_size(thread),
							  it, end);
			it = next_it;
		}
	}

	//
	// [ candidates ] [ test ] [ trash ]
	// test are the ones that cannot be finer than everyone
	// else, but need to be tested.
	// trash are the ones that are being represented in
	// candidates and test and do not alter the result
	//
	// searching for the finest depth list

	nx_Thread *result = 0;

	s32 end_candidates=self->size;
	s32 end_test=self->size;
	s32 end_all=self->size;

	// candidate is always on the zero index
	// chasing a thread that is coarser than all the other threads
	while (end_candidates > 1) {
		s32 cmp = nx_DepthList_compare(depth_lists,depth_lists+1);
		if (cmp == nx_DepthList_CMP_COARSER || cmp == nx_DepthList_CMP_EQUAL) {
			// trash the current comparison guy
			// rotate one left, --end_candidates and
			// --end_test
			nx_DepthList_swap(depth_lists+1, depth_lists+end_candidates-1);
			nx_DepthList_swap(depth_lists+end_candidates-1,depth_lists+end_test-1);
			--end_candidates;
			--end_test;
		} else if (cmp == nx_DepthList_CMP_FINER) {
			// swap current candidate with the one being
			// tested and rotate as in the finer case
			nx_DepthList_swap(depth_lists, depth_lists+1);
			nx_DepthList_swap(depth_lists+1, depth_lists+end_candidates-1);
			nx_DepthList_swap(depth_lists+end_candidates-1,depth_lists+end_test-1);
			--end_candidates;
			--end_test;
		} else if (cmp == nx_DepthList_CMP_NONE) {
			// rotate twice until the test end
			nx_DepthList_rotate(depth_lists, depth_lists + 2, depth_lists + end_test);
			end_candidates-=2;
		} else {
			Assert(0); // unexpected branch: probably some memory corruption
		}
	}

	if (end_candidates == 1) {
		result = self->threads + depth_lists->index;
		for (s32 i=end_candidates;i<end_test;++i) {
			s32 cmp = nx_DepthList_compare(depth_lists, depth_lists+i);
			if (cmp != nx_DepthList_CMP_COARSER) {
				result = 0;
				break;
			}
		}
	} else {
		Assert(end_candidates == 0);
	}

	// clearup the temp storage used
	BilinearAllocator_rewind(temp_storage, temp_storage_check_point);

#if 0
	// check for a match they need to match
	nx_Thread *check = nx_Threads_singleton_naive_stack(self, temp_storage);
	Assert(result == check);
#endif

	return result;
#endif
}

#ifdef nx_SINGLETON_NAIVE_STACK
#define nx_Threads_singleton nx_Threads_singleton_naive_stack
#elif defined nx_SINGLETON_OPTIMIZED_TEMP_STORAGE
#define nx_Threads_singleton nx_Threads_singleton_optimzed_temp_storage
#else
#define nx_Threads_singleton nx_Threads_singleton_naive_temp_storage
#endif

//-------------------------------------------------------------------------------
// nx_InsertionContext
//-------------------------------------------------------------------------------

typedef struct {
	nx_NanocubeIndex  *nanocube;
	void              *payload_unit;
	void              *payload_context;
	nx_Threads        *mfthreads;
	BilinearAllocator temp_storage;
} nx_InsertionContext;


//------------------------------------------------------------------------------
// NestedHierarchy
//------------------------------------------------------------------------------

// quick hack for debugging
static s32 COUNT = -1;

static void
nx_NanocubeIndex_init(nx_NanocubeIndex *self, al_Allocator *allocator, nx_Array bits_per_label)
{
	Assert(allocator);

	COUNT = -1; // @todo static hack

	nx_Caches_init(&self->detail_caches, allocator);

	// create w_node cache (nodes are uniformly sized!)
	char name[al_MAX_CACHE_NAME_LENGTH+1];

	nx_Ptr_Node_set_null(&self->root_p); // root_p is initially empty

	self->initialized = 1;
	self->dimensions = bits_per_label.length;
	self->number_of_records = 0;

	nx_print_cstr(name, name + sizeof(name), "INode");
	al_Ptr_Cache_set(&self->inode_cache_p, al_Allocator_create_cache(allocator, name, (u32) sizeof(nx_INode)));

	nx_print_cstr(name, name + sizeof(name), "PNode");
	al_Ptr_Cache_set(&self->pnode_cache_p, al_Allocator_create_cache(allocator, name, (u32) sizeof(nx_PNode)));

	Assert(self->dimensions < nx_MAX_NUM_DIMENSIONS && "too many dimensions");

	pt_copy_bytes((const char*) bits_per_label.begin,
		      (const char*) nx_Array_end(&bits_per_label),
		      (char*) self->bits_per_label,
		      (char*) self->bits_per_label
		      + sizeof(self->bits_per_label));
}


// @bug clone path needs to create a copy of nodes with 256 nodes as degree
// make sure it is working, this is hapenning on the dns example.
static nx_NodeWrap
nx_NanocubeIndex_allocate_node(nx_NanocubeIndex *self, s32 index, u8 path_length, s32 degree, b8 is_root, b8 clear, void *payload_context)
{
	nx_Node *node;
	if (index < self->dimensions - 1) {
		nx_INode *inode = (nx_INode*) al_Cache_alloc(al_Ptr_Cache_get_not_null(&self->inode_cache_p));
		nx_INode_init(inode, path_length, degree, is_root);
		node = (nx_Node*) inode;
	} else {
		nx_PNode *pnode = (nx_PNode*) al_Cache_alloc(al_Ptr_Cache_get_not_null(&self->pnode_cache_p));
		nx_PNode_init(pnode, path_length, degree, is_root, payload_context);
		node = (nx_Node*) pnode;
	}

	nx_DetailInfo detail_info;
       	nx_DetailInfo_init(&detail_info, degree, path_length, self->bits_per_label[index]);

	// initialize self's external storage if needed
	switch (detail_info.detail_type) {
	case nx_DETAIL_c16pL: {
		nx_Ptr_Label_set(&node->detail.c16pL.path_p, (nx_Label*) nx_Caches_alloc(&self->detail_caches, detail_info.path_storage_size));
	} break;
	case nx_DETAIL_cLp16: {
		nx_Ptr_Child_set(&node->detail.cLp16.children_p, (nx_Child*) nx_Caches_alloc(&self->detail_caches, detail_info.children_storage_size));
	} break;
	case nx_DETAIL_cLpL: {
		/* Ptr_Label_set(&node->detail.c16pL.path_p, */
		nx_Ptr_Label_set(&node->detail.cLpL.path_p, (nx_Label*) nx_Caches_alloc(&self->detail_caches, detail_info.path_storage_size));
		/* Ptr_Child_set(&node->detail.cLp16.children_p, */
		nx_Ptr_Child_set(&node->detail.cLpL.children_p, (nx_Child*) nx_Caches_alloc(&self->detail_caches, detail_info.children_storage_size));
	} break;
	default:
		break;
	}

	nx_NodeWrap wnode;
	nx_NodeWrap_init(&wnode, node, &detail_info);
	if (clear) {
		/*
		 * @check this should not be needed (unitialized path
		 * and children entries shouldn't affect anything else).
		 */
		nx_Path_clear(&wnode.path);
		nx_Children_clear(&wnode.children);
	}
	return wnode;
}

#if 0
static s32 COUNT_ALLOCATE_LEAF = 0;
#endif

static nx_NodeWrap
nx_NanocubeIndex_allocate_leaf(nx_NanocubeIndex *self,
			       s32 index,
			       b8 is_root,
			       nx_Array path,
			       void *payload_context)
{
#if 0
	printf("COUNT_ALLOCATE_LEAF: %d\n",COUNT_ALLOCATE_LEAF);
#endif
	nx_NodeWrap node = nx_NanocubeIndex_allocate_node(self, index, path.length, 0, is_root, 0, payload_context); // don't clean path
	for (u8 i=0;i<path.length;++i) {
		u8 label = nx_Array_get(&path,(u8) i);
		nx_Path_set(&node.path, i, label);
		Assert(nx_Path_get(&node.path,i) == label);
	}
#if 0
	++COUNT_ALLOCATE_LEAF;
#endif
	return node;
}

static void
nx_Path_convert_to_suffix(nx_Path *self, u8 suffix_length)
{
	Assert(self->length >= suffix_length);
	if (self->length == suffix_length)
		return;
	u8 i = self->length - suffix_length;
	for (u8 j = 0; j < suffix_length; ++j, ++i) {
		nx_Path_set(self, j, nx_Path_get(self, i));
	}
	self->length = suffix_length;
}

static nx_NodeWrap
nx_NanocubeIndex_shrink_path(nx_NanocubeIndex *self, s32 index, nx_NodeWrap w_node, u8 suffix_length)
{
	/* make adjustments so that path is trimmed to a proper non-zero length suffix */
	Assert(suffix_length > 0 && w_node.path.length > suffix_length);

	nx_Node *node = w_node.raw_node;
	nx_DetailInfo new_detail_info;
	nx_DetailInfo_init(&new_detail_info, nx_Node_degree(node), suffix_length, self->bits_per_label[index]);

	nx_Detail *detail = &node->detail;

	nx_DetailType t0 = w_node.detail_type;
	nx_DetailType t1 = new_detail_info.detail_type;
	b8 same_path_storage_capacity =
		w_node.path_class == new_detail_info.path_storage_size;

	/*
	 * Given degree and path_bytes of a self, a unique detail_type is
	 * determined to be one of these:
	 *
	 * c16p6, c0p22, cLp16, c16pL cLpL
	 *
	 * When path shrinking, the allowed transitions are:
	 *
	 * c16p6 ---> c16p6 (only this case happens)
	 * c0p22 ---> c0p22, c16p6	(if p > 6 becomes less than 6 bytes)
	 * c16pL ---> c16pL, c0p22, c16p6
	 * cLp16 ---> cLp16
	 * cLpL  ---> cLpL, cLp16
	 */

	/* adjust the "detail" segment */
	if (t0 == nx_DETAIL_c16p6) {
		Assert(t1 == nx_DETAIL_c16p6);
		nx_Path_convert_to_suffix(&w_node.path, suffix_length);
	} else if (t0 == nx_DETAIL_c0p22) {
		if (t1 == nx_DETAIL_c0p22) {
			nx_Path_convert_to_suffix(&w_node.path, suffix_length);
		} else if (t1 == nx_DETAIL_c16p6) {
			nx_Path_convert_to_suffix(&w_node.path, suffix_length);
			nx_Label *src = detail->c0p22.path;
			nx_Label *dst = detail->c16p6.path;
			for (u8 i = 0; i < 6; ++i) {
				*(dst + i) = *(src + i);
			}
		} else {
			Assert(0 && "Case not expected! Stop everything.");
		}
	} else if (t0 == nx_DETAIL_c16pL) {
		if (t1 == nx_DETAIL_c16pL) {
			if (same_path_storage_capacity) {
				nx_Path_convert_to_suffix(&w_node.path,
							  suffix_length);
			} else {
				nx_Path path = w_node.path;
				nx_Label *new_path_storage = (nx_Label*) nx_Caches_alloc(&self->detail_caches, new_detail_info .path_storage_size);
				nx_Path suffix;
				nx_Path_init(&suffix, new_path_storage, 0, new_detail_info.path_capacity, new_detail_info.bits_per_label);
				nx_Path_copy_range(&suffix, &path, path.length - suffix_length, suffix_length);
				nx_Caches_free(&self->detail_caches, w_node.path_class, path.begin);
				nx_Ptr_Label_set(&detail->c16pL.path_p, new_path_storage);
			}
		} else if (t1 == nx_DETAIL_c0p22) {
			nx_Path path = w_node.path;
			nx_Path suffix;
			nx_Path_init(&suffix, detail->c0p22.path, 0, new_detail_info.path_capacity, new_detail_info.bits_per_label);
			nx_Path_copy_range(&suffix, &path, path.length - suffix_length, suffix_length);
			nx_Caches_free(&self->detail_caches, w_node.path_class, path.begin);
		} else if (t1 == nx_DETAIL_c16p6) {
			nx_Path path = w_node.path;
			nx_Path suffix;
			nx_Path_init(&suffix, detail->c16p6.path, 0, new_detail_info.path_capacity, new_detail_info.bits_per_label);
			nx_Path_copy_range(&suffix, &path, path.length - suffix_length, suffix_length);
			nx_Caches_free(&self->detail_caches, w_node.path_class, path.begin);
		} else {
			Assert(0 && "Case not expected! Stop everything.s");
		}
	} else if (t0 == nx_DETAIL_cLp16) {
		Assert(t1 == nx_DETAIL_cLp16 && "Case not expected");
		nx_Path_convert_to_suffix(&w_node.path, suffix_length); // in-place
	} else if (t0 == nx_DETAIL_cLpL) {
		if (t1 == nx_DETAIL_cLpL) {
			if (same_path_storage_capacity) {
				nx_Path_convert_to_suffix(&w_node.path, suffix_length);
			} else {
				nx_Path path = w_node.path;
				nx_Label *new_path_storage = (nx_Label*) nx_Caches_alloc(&self->detail_caches, new_detail_info .path_storage_size);
				nx_Path suffix;
				nx_Path_init(&suffix, new_path_storage, 0, new_detail_info.path_capacity, new_detail_info.bits_per_label);
				nx_Path_copy_range(&suffix, &path, path.length - suffix_length, suffix_length);
				nx_Caches_free(&self->detail_caches, w_node.path_class, path.begin);
				nx_Ptr_Label_set(&detail->cLpL.path_p, new_path_storage);
			}
		} else if (t1 == nx_DETAIL_cLp16) {
			nx_Path path = w_node.path;
			nx_Path suffix;
			nx_Path_init(&suffix, detail->cLp16.path, 0, new_detail_info.path_capacity, new_detail_info.bits_per_label);
			nx_Path_copy_range(&suffix, &path, path.length - suffix_length, suffix_length);
			nx_Caches_free(&self->detail_caches, w_node.path_class, path.begin);
		} else {
			Assert(0 && "Case not expected! Stop everything!");
		}
	}
	/* end: adjust the "detail" segment */

	/* adjust length of self */
	node->path_length = suffix_length;

	nx_NodeWrap w_new_node;
	nx_NodeWrap_init(&w_new_node, node, &new_detail_info);

	return w_new_node;
}


//
// Right after adding a slot, we need a new child
// should be inserted to preserve the degree
// consistency of the detail_type of a self
//
static nx_NodeWrap
nx_NanocubeIndex_add_child_slot(nx_NanocubeIndex *self, s32 index, nx_NodeWrap w_node)
{
	Assert(w_node.children.length == w_node.children.capacity);

	nx_Node *node = w_node.raw_node;

	// update
	nx_DetailInfo new_detail_info;
	nx_DetailInfo_init(&new_detail_info, nx_Node_degree(node)+1, node->path_length, self->bits_per_label[index]);

	nx_DetailType t0 = w_node.detail_type;
	nx_DetailType t1 = new_detail_info.detail_type;
	// auto same_children_storage_capacity = w_node.children_class == new_detail_info.children_storage_size;

	nx_Detail *detail = &node->detail;

	/*
	 *  Given degree and path_bytes of a self, a unique detail_type is
	 *  determined to be one of these:
	 *
	 *  c16p6, c0p22, cLp16, c16pL cLpL
	 *
	 *  When path adding child slot to a full-capacity self, the allowed transitions are:
	 *
	 *  c16p6 ---> cLp16
	 *  c0p22 ---> c16pL (cannot go to c16p6 since path bytes is greater than 6)
	 *  c16pL ---> cLpL, cLp16 (might bring path to local again)
	 *  cLp16 ---> cLp16
	 *  cLpL  ---> cLpL
	 */
	/* adjust the "detail" segment */
	if (t0 == nx_DETAIL_c16p6) {
		Assert(t1 == nx_DETAIL_cLp16);
		// copy last 6 bytes containig local path to
		nx_Child *new_children_storage = (nx_Child*) nx_Caches_alloc(&self->detail_caches, new_detail_info .children_storage_size);
		nx_Children new_children;
		nx_Children_init(&new_children, new_children_storage, 0, new_detail_info.children_capacity);
		nx_Children_copy(&new_children, &w_node.children);

		// copy the last 6 bytes
		nx_Label *src = detail->c16p6.path;
		nx_Label *dst = detail->cLp16.path;
		for (u8 i = 0; i < 6; ++i) {
			*(dst + i) = *(src + i);
		}

		nx_Ptr_Child_set(&detail->cLp16.children_p, new_children_storage);
	} else if (t0 == nx_DETAIL_c0p22) {
		Assert(t1 == nx_DETAIL_c16pL);

		// have to copy path to an external block
		nx_Label *new_path_storage = (nx_Label*) nx_Caches_alloc(&self->detail_caches, new_detail_info.path_storage_size);
		for (u8 i = 0; i < 22; ++i) {
			*(new_path_storage + i) = detail->raw[i];
		}
		nx_Ptr_Label_set(&detail->c16pL.path_p,new_path_storage);
	} else if (t0 == nx_DETAIL_c16pL) {
		if (t1 == nx_DETAIL_cLpL) {
			nx_Child *new_children_storage = (nx_Child*) nx_Caches_alloc(&self->detail_caches, new_detail_info .children_storage_size);
			nx_Children new_children;
			nx_Children_init(&new_children, new_children_storage, 0, new_detail_info.children_capacity);
			nx_Children_copy(&new_children, &w_node.children);

			nx_Ptr_Child_set(&detail->cLpL.children_p, new_children_storage);
			nx_Ptr_Label_set(&detail->cLpL.path_p, w_node.path.begin);
		} else if (t1 == nx_DETAIL_cLp16) {
			nx_Child *new_children_storage = (nx_Child*) nx_Caches_alloc(&self->detail_caches, new_detail_info .children_storage_size);
			nx_Children new_children;
			nx_Children_init(&new_children, new_children_storage, 0, new_detail_info.children_capacity);
			nx_Children_copy(&new_children, &w_node.children);

			nx_Label *path_storage = detail->cLp16.path;
			u32 path_bytes = ((node->path_length * new_detail_info.bits_per_label) + 7) / 8;
			for (u32 i = 0; i < path_bytes; ++i) {
				*(path_storage + i) = *(w_node.path.begin + i);
			}
			nx_Caches_free(&self->detail_caches, w_node.path_class, w_node.path.begin);
		} else {
			Assert(0 && "Case not expected! Stop everything.s");
		}
	} else if (t0 == nx_DETAIL_cLp16) {
		Assert(t1 == nx_DETAIL_cLp16 && "Case not expected");

		nx_Child *new_children_storage = (nx_Child*) nx_Caches_alloc(&self->detail_caches, new_detail_info.children_storage_size);
		nx_Children new_children;
		nx_Children_init(&new_children, new_children_storage, 0, new_detail_info.children_capacity);
		nx_Children_copy(&new_children, &w_node.children);

		nx_Caches_free(&self->detail_caches, w_node.children_class, w_node.children.begin);
		nx_Ptr_Child_set(&detail->cLp16.children_p, new_children_storage);
	} else if (t0 == nx_DETAIL_cLpL) {
		Assert(t1 == nx_DETAIL_cLpL && "Case not expected! Stop everything!");

		nx_Child *new_children_storage = (nx_Child*) nx_Caches_alloc(&self->detail_caches, new_detail_info.children_storage_size);
		nx_Children new_children;
		nx_Children_init(&new_children, new_children_storage, 0, new_detail_info.children_capacity);
		nx_Children_copy(&new_children, &w_node.children);

		/*
		 * @BUG(Y): running test with 2 bits, 7 levels, 2 dim
		 * at around 22% (823MB) this assertion failed
		 */
		nx_Caches_free(&self->detail_caches, w_node.children_class, w_node.children.begin);
		nx_Ptr_Child_set(&detail->cLpL.children_p,new_children_storage);
	}
	/* end: adjust the "detail" segment */

	nx_NodeWrap w_new_node;
	nx_NodeWrap_init(&w_new_node, node, &new_detail_info);

	return w_new_node;
}


//
// Object that collects info on path to root_p
//
typedef enum {
	nx_NOT_CONTAIN_COMPATIBLE=0,
	nx_CONTAIN_COMPATIBLE_NOT_PROVED=1,
	nx_CONTAIN_COMPATIBLE_PROVED=2
} nx_ContainCompatible;

typedef struct {
	nx_Label	     path_storage[nx_MAX_PATH_LENGTH];
	nx_Node*	     nodes_storage[nx_MAX_PATH_LENGTH];
	nx_List_NodeP    nodes;
	nx_List_u8	     path;
} nx_PathToRootInfo;

static void
nx_PathToRootInfo_init(nx_PathToRootInfo *self)
{
	nx_List_NodeP_init(&self->nodes, &self->nodes_storage[0], &self->nodes_storage[0], &self->nodes_storage[nx_MAX_PATH_LENGTH]);
	nx_List_u8_init(&self->path, &self->path_storage[0], &self->path_storage[0], &self->path_storage[nx_MAX_PATH_LENGTH]);
}

static void
nx_PathToRootInfo_reset(nx_PathToRootInfo *self, nx_NanocubeIndex *hierarchy, s32 index, nx_Node *node, s32 start_prefix_length)
{
	Assert(node);

	nx_List_NodeP_clear(&self->nodes);
	nx_List_u8_clear(&self->path);

	nx_NodeWrap w_node = nx_NanocubeIndex_to_node(hierarchy, node, index);

	Assert(start_prefix_length <= node->path_length);

	nx_List_NodeP_push_back(&self->nodes,node);
	for (s32 i=start_prefix_length-1;i>=0;--i)
		nx_List_u8_push_back(&self->path, nx_Path_get(&w_node.path,(u8) i));

	if (node->root)
		return;

	nx_Node *u = nx_Node_parent(node);
	while (u) {
		nx_NodeWrap u_wnode = nx_NanocubeIndex_to_node(hierarchy, u,index);
		nx_List_NodeP_push_back(&self->nodes,u);
		for (s32 i=u_wnode.path.length-1;i>=0;--i)
			nx_List_u8_push_back(&self->path,nx_Path_get(&u_wnode.path,(u8) i));
		if (u->root) break;
		else u = nx_Node_parent(u);
	}
}

static nx_ContainCompatible
nx_PathToRootInfo_contain_compatible(nx_PathToRootInfo *self, nx_PathToRootInfo *other)
{

	if (nx_List_u8_size(&self->path) > nx_List_u8_size(&other->path))
		return nx_NOT_CONTAIN_COMPATIBLE;

	// check if paths are contain compatible
	for (u8 i=0;i<nx_List_u8_size(&self->path);++i) {
		if (nx_List_u8_get_reverse(&self->path,i) != nx_List_u8_get_reverse(&other->path,i))
			return nx_NOT_CONTAIN_COMPATIBLE;
	}

	//
	// it is contain compatible up to this point
	// it will be proved if there is a common w_node in
	// the nodes path
	//
	if (nx_List_NodeP_get_reverse(&self->nodes,0) != nx_List_NodeP_get_reverse(&other->nodes,0))
		return nx_CONTAIN_COMPATIBLE_NOT_PROVED;
	else
		return nx_CONTAIN_COMPATIBLE_PROVED;
}


//
// dimension index
//
static b8
nx_NanocubeIndex_a_contains_b(nx_NanocubeIndex *self,
			      s32 index, nx_Node *a,
			      s32 a_prefix_size,
			      nx_Node *b,
			      s32 b_prefix_size)
{
	if (a == b && a_prefix_size <= b_prefix_size) return 1;

	//
	// while we don't find a common w_node, path in a needs
	// to be contain-compatible with path in b
	//
	// layout the w_node and label path up to the root_p
	//

	nx_PathToRootInfo a_info;
	nx_PathToRootInfo_init(&a_info);
	nx_PathToRootInfo b_info;
	nx_PathToRootInfo_init(&b_info);

	for (;;) {
		nx_PathToRootInfo_reset(&a_info, self, index, a, a_prefix_size);
		nx_PathToRootInfo_reset(&b_info, self, index, b, b_prefix_size);

		nx_ContainCompatible status = nx_PathToRootInfo_contain_compatible(&a_info,&b_info);

		if (status == nx_NOT_CONTAIN_COMPATIBLE) {
			return 0;
		}
		else if (status == nx_CONTAIN_COMPATIBLE_PROVED) {
			return 1;
		}
		else if (status == nx_CONTAIN_COMPATIBLE_NOT_PROVED) {
			if (index == 0) { // proved!
				return 1;
			}
			a = nx_Node_parent(nx_List_NodeP_back(&a_info.nodes));
			b = nx_Node_parent(nx_List_NodeP_back(&b_info.nodes));
			a_prefix_size = a->path_length;
			b_prefix_size = b->path_length;
			--index;
		}
	}
}





//--------------------------------------------------------------------------------
// position
//--------------------------------------------------------------------------------

typedef enum
{
	nx_UNDEFINED=0,
	nx_EXACT=1,
	nx_BRANCH=2,
	nx_SPLIT=3,
	nx_SHARED_NO_SPLIT=4,
	nx_SHARED_SPLIT=5,
	nx_SHARED_SUFFIX_WAS_SPLIT=6,
	nx_SINGLETON=7
} nx_PositionType;

static const char *nx_pos_text[] =
{
	"UNDEFINED",
	"EXACT",
	"BRANCH",
	"SPLIT",
	"SHARED_NO_SPLIT",
	"SHARED_SPLIT",
	"SHARED_SUFFIX_WAS_SPLIT",
	"SINGLETON"
};

static const char*
nx_PositionType_str(nx_PositionType pt)
{
	return nx_pos_text[(s32) pt];
}

typedef struct {
	nx_List_NodeP      path;
	nx_List_u8	   child_indices;
	nx_List_u8	   lengths;

	nx_Node*	   _path_storage[nx_MAX_PATH_LENGTH];
	u8		   _child_indices_storage[nx_MAX_PATH_LENGTH];
	u8		   _lengths_storage[nx_MAX_PATH_LENGTH];

	nx_PositionType    position_type;

	u8		   length_sum;
	s32		   child_newchild_index;

	nx_NanocubeIndex   *nanocube;
} nx_HierarchyPosition;

static void
nx_HierarchyPosition_init(nx_HierarchyPosition *self, nx_NanocubeIndex *nanocube)
{
	nx_List_NodeP_init(&self->path, self->_path_storage,
			   self->_path_storage,
			   self->_path_storage + nx_MAX_PATH_LENGTH);
	nx_List_u8_init(&self->child_indices, self->_child_indices_storage,
			self->_child_indices_storage,
			self->_child_indices_storage + nx_MAX_PATH_LENGTH);
	nx_List_u8_init(&self->lengths, self->_lengths_storage,
			self->_lengths_storage,
			self->_lengths_storage + nx_MAX_PATH_LENGTH);
	self->length_sum = 0;
	self->position_type = nx_UNDEFINED;
	self->child_newchild_index = -1;

	self->nanocube = nanocube;
}

//
// drop last w_node from last search
//
static void
nx_HierarchyPosition_pop_back(nx_HierarchyPosition *self)
{
	u8 len = 1;
	Assert(nx_List_NodeP_size(&self->path) >= len);
	self->length_sum -= List_u8_back(&self->lengths);
	nx_List_NodeP_drop_suffix(&self->path, len);
	nx_List_u8_drop_suffix(&self->lengths, len);
	nx_List_u8_drop_suffix(&self->child_indices,
			       Min(len, nx_List_u8_size(&self->child_indices)));
}

static inline nx_Node*
nx_HierarchyPosition_child(nx_HierarchyPosition *self)
{
	return nx_List_NodeP_get_reverse(&self->path,0);
}

static inline nx_Node*
nx_HierarchyPosition_parent(nx_HierarchyPosition *self)
{
	/* this was wrong! */
	return (nx_List_NodeP_size(&self->path) > 1)
		? nx_List_NodeP_get_reverse(&self->path, 1)
		: 0;
}


/*
 * Search for current_labels given the current state
 * of the nx_HierarchyPosition object.
 */
static void
nx_HierarchyPosition_run(nx_HierarchyPosition *self, s32 index, nx_Array current_labels)
{

	//     Node *parent = HierarchyPosition_parent(self);
	nx_Node *child  = nx_HierarchyPosition_child(self);

	Assert(index < self->nanocube->dimensions);
	u8 bits_per_label = self->nanocube->bits_per_label[index];

	for (;;) {

		// check common prefix length
		u8 n = nx_Node_check_common_path_length(child, &current_labels, bits_per_label, child->path_length);

		self->length_sum += n;
		nx_List_u8_push_back(&self->lengths, n);

		if (n == current_labels.length) {
			/*
			 * Consumed the whole current_labels path.
			 * Needs to be an exact match.
			 */
			Assert(n == child->path_length);
			self->position_type = nx_EXACT;
			self->child_newchild_index = -1;
			break;

		} else if (n == child->path_length) {

			/*
			 * Consumed the whole path of the current child, but not
			 * done yet. Check if there is a child of the current child
			 * (a grandchild) whose first labels matches current_labels[n],
			 * otherwise report an nx_BRANCH. If there is a label match, then
			 * two cases are possible: the grandchild is shared, or it is proper.
			 */
			nx_NodeWrap wnode = nx_NanocubeIndex_to_node(self->nanocube, child, index);

			nx_Label input_next_label = nx_Array_get(&current_labels, n);

			Assert(input_next_label < (1u << bits_per_label) && "input path label has too many bits");

			s32 pos = nx_Children_find(&wnode.children, input_next_label);

			if (pos >= 0) {
				/* labels match! */

				nx_List_u8_push_back(&self->child_indices, (s32) pos);

				nx_Child *child_slot = nx_Children_get_child(&wnode.children, (s32) pos);

				child = nx_Child_get_node(child_slot);

				nx_List_NodeP_push_back(&self->path, child);

				//
				// what to do here: there is a real case where sharing
				// with a suffix length that is greater than the updated
				// length child labels.
				//
				// some kind of rewind is needed here!
				//
				if (child_slot->shared) {

					if (child->path_length < child_slot->suffix_length) {
						/*
						 * Special case: the path of the shared child was
						 * split since it was associated to the child node.
						 *
						 * @PROVE: this happened in the finer cases of the
						 * current insertion and labeling this as a special
						 * case makes it easier for the insertion algorithm
						 * to figure out what to do.
						 */
						nx_List_u8_push_back(&self->lengths, (u8) 0); // special case here
						self->length_sum += 0;
						self->position_type = nx_SHARED_SUFFIX_WAS_SPLIT;
						break;
					} else {
						/*
						 * If the current_labels match all the shared grandchild
						 * labels, then we have a nx_SHARED_NO_SPLIT case, otherwise
						 * we are the the nx_SHARED_SPLIT case and we set the
						 * length_sum at granchild as the common labels
						 * remaining (note that it needs to be > 0).
						 */
						nx_Array remaining_labels = nx_Array_drop_prefix(&current_labels, (u8)n);
						u8 nn = nx_Node_check_common_path_length(child,
											 &remaining_labels,
											 bits_per_label,
											 child_slot->suffix_length);
						Assert(nn > 0);
						nx_List_u8_push_back(&self->lengths, nn);
						self->length_sum += nn;
						b8 shared_split = nn < child_slot->suffix_length;
						self->position_type = (shared_split) ? nx_SHARED_SPLIT : nx_SHARED_NO_SPLIT;
						break;
					}
				} else {
					/* iterate with grandchild as the new child */
					current_labels = nx_Array_drop_prefix(&current_labels, n);
				}

			}
			else {
				/* branch */
				self->child_newchild_index = -pos - 1;
				self->position_type = nx_BRANCH;
				break;
			}
		} else {
			/* split */
			self->position_type = nx_SPLIT;
			break;
		}
	}
}


static void
nx_HierarchyPosition_find_continue(nx_HierarchyPosition *self,
				   s32 index, nx_Array labels)
{

	//
	// Assuming things are aligned.
	// Parent is the rightmost element and the
	// ramining_labels should start
	// looking at a child w_node of parent
	//

	nx_Node *parent = nx_List_NodeP_get_reverse(&self->path,0);
	nx_NodeWrap parent_wnode = nx_NanocubeIndex_to_node(self->nanocube, parent, index);

	Assert(index < self->nanocube->dimensions);
	u8 bits_per_label = self->nanocube->bits_per_label[index];

	s32 pos = nx_Children_find(&parent_wnode.children,
				   nx_Array_get(&labels, (u8)0));

	if (pos >= 0) {

		nx_Child *child_slot =
			nx_Children_get_child(&parent_wnode.children,
					      (s32) pos);

		nx_Node* child = nx_Child_get_node(child_slot);

		nx_List_NodeP_push_back(&self->path, child); // every time a new child shows up

		nx_List_u8_push_back(&self->child_indices, (u8) pos);

		// check if it is a shared w_node (insert on path and break if it is)
		if (child_slot->shared) {
			if (child->path_length < child_slot->suffix_length) {
				nx_List_u8_push_back(&self->lengths, (u8) 0);
				self->length_sum += 0;
				self->position_type
					= nx_SHARED_SUFFIX_WAS_SPLIT;
				return;
			}
			else {
				// check if there is a split or there is no split
				u8 n = nx_Node_check_common_path_length(child, &labels, bits_per_label, child_slot->suffix_length);
				nx_List_u8_push_back(&self->lengths, n);
				self->length_sum += n;
				b8 shared_split = n < child_slot->suffix_length;
				self->position_type = (shared_split) ? nx_SHARED_SPLIT : nx_SHARED_NO_SPLIT;
				return;
			}
		}

	}
	else { // branch
		self->child_newchild_index = -pos - 1;
		self->position_type = nx_BRANCH;
		return;
	}

	// continue drilling down
	nx_HierarchyPosition_run(self, index, labels);

}


/*
 * Searches for path described by current_labels on the
 * hierarchy rooted at "root".
 *
 * clear flag indicates if should clear current status
 * of the HierarchyPosition info and start from scratch.
 * (use false if in a continuation process with the careful
 * alignment guaranteed by the user)
 */
static void
nx_HierarchyPosition_find_first(nx_HierarchyPosition *self,
				nx_NanocubeIndex* h,
				s32 index,
				nx_Node* root,
				nx_Array current_labels,
				b8 clear)
{
	if (clear) {
		self->length_sum = 0;
		nx_List_NodeP_clear(&self->path);
		nx_List_u8_clear(&self->child_indices);
		nx_List_u8_clear(&self->lengths);
	}

	nx_List_NodeP_push_back(&self->path, root);

	nx_HierarchyPosition_run(self, index, current_labels);

}

//
// if we want to generate an insertion log file, uncomment line below
//
#define nx_LOG_remove_this_suffix_to_activate

#ifdef nx_LOG
#include "nanocube_index_debug.c"
#else
#define nx_LOG_NODE_EVENT(a,b,c,d)
#define nx_LOG_START(a)
#define nx_LOG_FINISH(a)
#define nx_LOG_PUSH_RECORD(a)
#define nx_LOG_PUSH_INSERT_CASE(a,b,c,d,e)
#define nx_LOG_APPEND_PATH(a)
#define nx_LOG_POP(a)
#define nx_LOG_MFTHREADS_ADVANCE(a,b)
#define nx_LOG_MFTHREADS_REWIND(a,b)
#define nx_LOG_MFTHREADS_ENTER(a)
#define nx_LOG_MFTHREADS_EXIT(a)
#define nx_LOG_MFTHREADS_PUSH(a)
#define nx_LOG_MFTHREADS_POP(a)
#define nx_LOG_MFTHREADS_CLONE_PATH(a,b)
#define nx_LOG_UPSTREAM_INSERT(a,b,c,d,e)
#define nx_LOG_PRINT_MFHEADS(a,b,c)
#define nx_LOG_PRINT_STR(a,b)
#define nx_LOG_PRINT_PATH(a,b,c,d,e)
#define nx_LOG_RESET_COUNTERS
#endif

static nx_Node*
nx_NanocubeIndex_allocate_singleton(nx_NanocubeIndex *self, s32 start_index, b8 start_root,
				    nx_LabelArrayList* path_list, void *payload_unit, void *payload_context)
{
	nx_Node* result = 0;
	nx_Node* prev = 0;
	s32      i = start_index;
	while (path_list) {
		b8 is_root = (i == start_index) ? start_root : 1;
		nx_NodeWrap wnode = nx_NanocubeIndex_allocate_leaf(self, i, is_root, path_list->labels, payload_context);

		if (i == self->dimensions - 1) {
			nx_PNode_payload_insert((nx_PNode*)wnode.raw_node, payload_unit, payload_context);
		}

		if (!prev) { // first dimension?
			result = wnode.raw_node;
			prev = result;
		} else {
			//
			// following dimensions get tagged with a parent w_node that owns
			// the next dimension w_node
			//
			nx_INode_own_content((nx_INode*)prev, wnode.raw_node);
			//((INode*)prev)->own_content(wnode.self());
			nx_Node_set_parent(wnode.raw_node, prev);


			prev = wnode.raw_node;
		}
		path_list = path_list->next;
		++i;
	}

#ifdef nx_LOG
	nx_Node *it    = prev;
	s32     it_dim = i;
	while (it) {
		--it_dim;
		// ----- log -----
		nx_LOG_NODE_EVENT(self, it, it_dim, "create");
		// ----- log -----
		it = nx_Node_parent(it);
	}
#endif

	return result;
}

// internal nx_Node*
// nx_insert_recursive(nx_NanocubeIndex *self,
// 				  nx_Node* context,
// 				  nx_Node* r,
// 				  b8 shared,
// 				  s32 index,
// 				  nx_LabelArrayList* insert_path_list,
// 				  nx_Threads *mfthreads,
// 				  void *payload_unit,
// 				  void *payload_context);

static nx_Node*
nx_insert_recursive(nx_InsertionContext *ctx,
				  nx_Node* context,
				  nx_Node* r,
				  b8 shared,
				  s32 index,
				  nx_LabelArrayList* insert_path_list);

/*
 * Note that upstream is called with a list of nodes whose
 * contents are still outdated by the current record being
 * inserted.
 */
// internal void
// nx_Nanocube_upstream_insert(nx_NanocubeIndex  *self,
// 			    nx_LabelArrayList *insert_path_list,
// 			    s32               index,
// 			    nx_Threads        *mfthreads,
// 			    void              *payload_unit,
// 			    void              *payload_context,
// 			    nx_Node           *mfthread,
// 			    s32               num_shared,
// 			    nx_List_u8        *lengths,
// 			    nx_List_NodeP     *path)
static void
nx_Nanocube_upstream_insert(nx_InsertionContext  *ctx,
			    nx_LabelArrayList    *insert_path_list,
			    s32                  index,
			    nx_Node              *mfthread,
			    s32                  num_shared,
			    nx_List_u8           *lengths,
			    nx_List_NodeP        *path)
{
	Assert(path->begin != 0 && path->end != 0 && path->begin <= path->end && "invalid path");

	//
	// assumes a path of proper nodes at distance exactly position.distance_to_root()
	//
	// Assumes
	//
	// 1. path is a top-down list of raw nodes proper to this hierarchy.
	//	  this list should be processed bottom up and recursively
	//	  inserting the new record in the content of all path nodes
	//
	// 2. The bottom "num_shared" nodes in path contain shared content
	//	  and should be flagged as such in the recursive insertion call.
	//
	// 3. The ctx->mfthreads should already be set for the upstream insertion
	//	  process:
	//
	//		  (*) goto next dimension
	//		   -> insert recursive
	//		   -> goto previous dimension
	//		   -> rewind correct length
	//		   -> if not root_p repeat (*)
	//

#ifdef nx_SANITY_CHECKS
	for (s32 i = 0; i < nx_List_NodeP_size(path); ++i) {
		nx_Node* u = nx_List_NodeP_get_reverse(path,i);
		Assert(!(u->root == 0 && u->path_length == 0));
	}
#endif

// 	if (nx_List_NodeP_size(path) > 1 && index > 0) {
// 		//------ log ------
// 		nx_LOG_PRINT_STR(index, "weird!");
// 		//------ log ------
// 	}

	for (s32 i = 0; i < nx_List_NodeP_size(path); ++i) {

		nx_Node* u = nx_List_NodeP_get_reverse(path,i);

		//------ log ------
		nx_LOG_UPSTREAM_INSERT(index, (i + 1), (s32) nx_List_NodeP_size(path), ctx->nanocube, u);
		//------ log ------

		if (index < ctx->nanocube->dimensions - 1) {
			nx_LOG_MFTHREADS_ENTER(index);
			nx_Threads_goto_next_dimension(ctx->mfthreads);
		}

		if (index < ctx->nanocube->dimensions - 1) {

			if (mfthread) {
				nx_LOG_MFTHREADS_PUSH(index);
				nx_Threads_push_thread(ctx->mfthreads, mfthread, index + 1);
			}

			nx_Node* uc = nx_insert_recursive (ctx, u, nx_INode_content((nx_INode*)u), i < num_shared, index + 1, insert_path_list->next);

			if (mfthread) {
				nx_LOG_MFTHREADS_POP(index);
				nx_Threads_pop_thread(ctx->mfthreads);
			}

			nx_INode_own_content((nx_INode*)u, uc);
			nx_Node_set_parent(uc, u);

			mfthread = nx_INode_content((nx_INode*)u);

		}
		else {
			/*
			 * @BUG a node without initialized payload shows
			 * up here when inserting the 5,119,969 record
			 * of yellow cabs dataset june 2016 (src and dst).
			 */
			nx_PNode_payload_insert((nx_PNode*)u, ctx->payload_unit, ctx->payload_context);

		}

		// ----- log -----
		nx_LOG_NODE_EVENT(ctx->nanocube, u, index, "update_content");
		// ----- log -----

		if (index < ctx->nanocube->dimensions - 1) {
			nx_LOG_MFTHREADS_EXIT(index);
			nx_Threads_goto_prev_dimension(ctx->mfthreads);
		}

		u8 len = nx_List_u8_get_reverse(lengths, i); // ctx->nanocube->to_node(u, index).path_length();

		if (len > 0) {
			nx_LOG_MFTHREADS_REWIND(index, len);
			nx_Threads_rewind(ctx->mfthreads, len); // @todo: optimize this call to to_node
		}

		//------ log ------
		nx_LOG_POP();
		//------ log ------
	}

	// make sure the whead dimension of the ctx->mfthreads
	// is in ther right dimension

	// @next mfthread

}

// update "length" first nodes on path with a clone version
typedef enum {
	nx_CLONE_ALL,
	nx_CLONE_EXCLUDE_LAST_CHILD
} nx_CloneMode;

// internal void
// nx_clone_path(nx_NanocubeIndex* self,
// 	      s32 index,
// 	      nx_Node* context,
// 	      nx_HierarchyPosition *pos,
// 	      nx_CloneMode mode,
// 	      void *payload_context)
static void
nx_clone_path(nx_InsertionContext* ctx,
	      s32 index,
	      nx_Node* context,
	      nx_HierarchyPosition *pos,
	      nx_CloneMode mode)
{
	nx_pf_BEGIN_BLOCK("nx_clone_path");

	//
	// pos gives the current position we want to clone
	//

	nx_List_NodeP path = pos->path;
	nx_List_u8 child_indices = pos->child_indices;
	if (mode == nx_CLONE_EXCLUDE_LAST_CHILD) {
		nx_List_NodeP_drop_suffix(&path, 1);
		if (nx_List_u8_size(&child_indices) > 0) {
			nx_List_u8_drop_suffix(&child_indices, 1);
		}
	}

	s64 length = nx_List_NodeP_size(&path);

	nx_LOG_MFTHREADS_CLONE_PATH(index, (s32) length);

	nx_Node* cloned_child = 0;
	for (u32 i = 0; i<length; ++i) {
		nx_NodeWrap u  = nx_NanocubeIndex_to_node (ctx->nanocube, nx_List_NodeP_get_reverse(&path,i), index);
		nx_NodeWrap uu = nx_NanocubeIndex_allocate_node(ctx->nanocube, index, u.path.length, u.children.length, u.raw_node->root, 0, ctx->payload_context);

		nx_Path_copy(&uu.path, &u.path);
		nx_Children_copy(&uu.children, &u.children);
		Assert(!(uu.raw_node->root == 0 && uu.raw_node->path_length == 0));

		for (s32 j = 0; j<uu.children.length; ++j) {
			nx_Child* child = nx_Children_get_child(&uu.children,j);
			if (!child->shared) {
				child->shared = 1;
				child->suffix_length =
					nx_Child_get_node(child)->path_length;
			}
		}

		if (index < ctx->nanocube->dimensions - 1) {
			nx_INode_share_content((nx_INode*) uu.raw_node, (nx_INode*) u.raw_node); // copy content
		} else {
			nx_PNode_payload_share((nx_PNode*)uu.raw_node, (nx_PNode*)u.raw_node, ctx->payload_context);
		}

		if (cloned_child != 0) {
			u8 child_index = nx_List_u8_get_reverse(&child_indices, i-1);

			nx_Child* child_slot = nx_Children_get_child(&uu.children, child_index);

			child_slot->shared = 0;
			nx_Child_set_node(child_slot, cloned_child);
			/* set parent of cloned child */
			nx_Node_set_parent(cloned_child, uu.raw_node);
		}
		nx_List_NodeP_set_reverse(&path, i, uu.raw_node);
		cloned_child = uu.raw_node;
	}


#ifdef nx_LOG
	nx_Node *it    = cloned_child;
	while (it) {
		// ----- log -----
		nx_LOG_NODE_EVENT(ctx->nanocube, it, index, "create_as_clone");
		// ----- log -----
		it = nx_Node_parent(it);
	}
#endif

	if (length > 0) {
		/* set context of root_p */
		nx_Node_set_parent(nx_List_NodeP_get(&path,0), context);
	}

	nx_pf_END_BLOCK();
}


// static volatile u64 INSERT_RECURSIVE_COUNT = 0;
static volatile u64 nx_INSERT_EXACT = 0;
static volatile u64 nx_INSERT_BRANCH = 0;
static volatile u64 nx_INSERT_SPLIT = 0;
static volatile u64 nx_INSERT_SHARED_SUFFIX_WAS_SPLIT = 0;
static volatile u64 nx_INSERT_SHARED_SPLIT = 0;
static volatile u64 nx_INSERT_SHARED_NO_SPLIT = 0;


#define nx_INSERT_CASE(name) \
static nx_Node* \
name(nx_InsertionContext *ctx,\
		nx_Node *context,\
		nx_Node *root,\
		b8 shared,\
		s32 index,\
		nx_LabelArrayList *insert_path_list,\
		nx_HierarchyPosition *pos)

nx_INSERT_CASE(nx_insert_exact)
{
	// nx_pf_BEGIN_BLOCK("nx_insert_exact");

	++nx_INSERT_EXACT;

	/*
	 * found the exact path described by "insert_path_list->labels"
	 * on the hierarchy rooted at "r".
	 */

	//----- log -----
	nx_LOG_PUSH_INSERT_CASE(pos->position_type, shared, index, ctx->nanocube, root);
	//----- log -----

	nx_Array current_labels = insert_path_list->labels;

	if (shared) {
		/*
		 * clone path starting at root "r" described
		 * by "current_labels". Branches become shared
		 * sub-hierarchies.
		 */
		nx_clone_path(ctx, index, context, pos, nx_CLONE_ALL);
	}

	// ----- log -----
	nx_LOG_MFTHREADS_ADVANCE(index, nx_Array_prefix(&current_labels, pos->length_sum));
	// ----- log -----

	nx_Threads_advance(ctx->mfthreads, nx_Array_prefix(&current_labels, pos->length_sum));

	// ----- log -----
	nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
	// ----- log -----

	nx_Nanocube_upstream_insert(ctx,
				    insert_path_list,
				    index,
				    0,
				    shared ? (s32) nx_List_NodeP_size(&pos->path) : 0,
				    &pos->lengths,
				    &pos->path);

	//----- log -----
	nx_LOG_POP();
	//----- log -----

	// nx_pf_END_BLOCK();

	return nx_List_NodeP_get(&pos->path,0);
}

nx_INSERT_CASE(nx_insert_branch)
{
	// nx_pf_BEGIN_BLOCK("nx_insert_branch");

	++nx_INSERT_BRANCH;

	//----- log -----
	nx_LOG_PUSH_INSERT_CASE(pos->position_type, shared, index, ctx->nanocube, root);
	//----- log -----

	nx_Array current_labels = insert_path_list->labels;

	// clone all nodes in the path (the upstream insert will be shared)
	if (shared) {
		nx_clone_path(ctx, index, context, pos, nx_CLONE_ALL);
	}

	// should work since path is the same as position.path
	// @todo make it more explicit who is child and parent
	// (if shared, they were replaced in the loop above)
	nx_Node* child = nx_List_NodeP_get_reverse(&pos->path, 0);

	nx_NodeWrap child_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, child, index);

	nx_Array suffix_labels = nx_Array_drop_prefix(&current_labels, pos->length_sum);

	// check if there is a singleton
	nx_LabelArrayList remaining_path_list = nx_LabelArrayList_build(suffix_labels, insert_path_list->next);

	// new w_node
	nx_Node*      new_node = 0;
	u8            new_node_suffix_length = 0;
	b8            new_node_shared = 0;

	if (ctx->mfthreads->size == 0) {
		const b8 is_root = 0;

		//----- log -----
		nx_LOG_APPEND_PATH(&remaining_path_list);
		//----- log -----

		// new w_node and its content is done
		new_node = nx_NanocubeIndex_allocate_singleton(ctx->nanocube, index, is_root, &remaining_path_list, ctx->payload_unit, ctx->payload_context);

		// this indicates that it is everything
		new_node_suffix_length = remaining_path_list.labels.length;
		new_node_shared = 0;
	} else {
		// ----- log -----
		nx_LOG_MFTHREADS_ADVANCE(index, current_labels);
		// ----- log -----

		nx_Threads_advance(ctx->mfthreads, current_labels);

		// ----- log -----
		nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
		// ----- log -----

		nx_Thread* singleton = nx_Threads_singleton(ctx->mfthreads, &ctx->temp_storage);
		Assert(singleton && "there needs to be a singleton here!!!");

		//----- log -----
		nx_LOG_PRINT_STR(index, "BRANCH using a SHARED child");
		//----- log -----

		new_node_shared = 1;
		new_node_suffix_length = nx_Thread_head_depth(singleton) - pos->length_sum;
		new_node = nx_Thread_head(singleton);

		nx_LOG_MFTHREADS_REWIND(index, current_labels.length);

		nx_Threads_rewind(ctx->mfthreads, current_labels.length);
	}


	//
	// @todo make this a single call
	// it is also used on the branch of a shared (split/no split)
	//
	if (!nx_Children_available_capacity(&child_wnode.children)) {
		/*
		 * @BUG(Y): running test with 2 bits, 7 levels, 2 dim
		 * at around 22% (823MB) this assertion failed
		 */
		child_wnode = nx_NanocubeIndex_add_child_slot(ctx->nanocube, index, child_wnode);
	}

	nx_Child child_to_insert;
	nx_Child_init(&child_to_insert);
	child_to_insert.label = nx_Array_get(&suffix_labels,(u8) 0);
	child_to_insert.shared = new_node_shared;
	child_to_insert.suffix_length = new_node_suffix_length;
	nx_Child_set_node(&child_to_insert, new_node);

	Assert(pos->child_newchild_index >= 0);

	nx_Children_insert(&child_wnode.children, (s32) pos->child_newchild_index, &child_to_insert);

	nx_Node_set_degree(child, nx_Node_degree(child) + 1); // ->degree += 1; // update degree of child

	//child_wnode.children().append_and_rotate(new_node, pos->child_newchild_index);
	//child_wnode.children().child(pos->child_newchild_index)
	//	.label(suffix_labels.get(0))
	//	.shared(new_node_shared)
	//	.suffix_length(new_node_suffix_length);
	//child->children_length(child->children_length() + 1);

	if (!new_node_shared) {
		nx_Node_set_parent(new_node, child);
	}

	// ----- log -----
	nx_LOG_NODE_EVENT(ctx->nanocube, child_wnode.raw_node, index, "branch");
	// ----- log -----

	// ----- log -----
	nx_LOG_MFTHREADS_ADVANCE(index, nx_Array_prefix(&current_labels, pos->length_sum));
	// ----- log -----

	nx_Threads_advance(ctx->mfthreads, nx_Array_prefix(&current_labels, pos->length_sum));

	// ----- log -----
	nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
	// ----- log -----

	nx_Nanocube_upstream_insert(ctx,
				    insert_path_list,
				    index,
				    (index < ctx->nanocube->dimensions - 1) ? nx_INode_content((nx_INode*)new_node) : 0,
				    shared ? (s32) nx_List_NodeP_size(&pos->path) : 0,
				    &pos->lengths,
				    &pos->path);

	//----- log -----
	nx_LOG_POP();
	//----- log -----

	// nx_pf_END_BLOCK();

	return nx_List_NodeP_get(&pos->path,0);

}


//
// There is a proper parent-child with path length>1 that needs a surgery to
// include a split node in the middle. Expect three updates:
//
// 1. either a totally new branch node or a shared existing branch
// 2. new split node
// 3. update path, parent info on the current child node
//
nx_INSERT_CASE(nx_insert_split)
{
	// nx_pf_BEGIN_BLOCK("nx_insert_split");

	++nx_INSERT_SPLIT;

	//----- log -----
	nx_LOG_PUSH_INSERT_CASE(pos->position_type, shared, index, ctx->nanocube, root);
	//----- log -----

	nx_Array current_labels = insert_path_list->labels;

	// suffix labels
	nx_Array          suffix_labels       = nx_Array_drop_prefix(&current_labels, pos->length_sum);
	nx_LabelArrayList remaining_path_list = nx_LabelArrayList_build(suffix_labels, insert_path_list->next);

	// some distance numbers
	u8 depth_branch = pos->length_sum;
	u8 depth_parent = depth_branch - nx_List_u8_get_reverse(&pos->lengths, 0);
	u8 dist_pb      = depth_branch - depth_parent;
	u8 dist_bn      = current_labels.length - depth_branch;

	// new w_node
	nx_Node*	  new_node = 0;
	//	       PathLength new_node_prefix_length = 0;
	b8	     new_node_shared = 0;

	if (ctx->mfthreads->size == 0) {
		const b8 is_root = 0;

		//----- log -----
		nx_LOG_APPEND_PATH(&remaining_path_list);
		//----- log -----

		new_node = nx_NanocubeIndex_allocate_singleton(ctx->nanocube, index, is_root, &remaining_path_list, ctx->payload_unit, ctx->payload_context);

		// new w_node and its content is done
		// new_node_prefix_length = dist_bn; // this indicates that it is everything
		new_node_shared = 0;

	} else {

		// figure out shared path
		nx_LOG_PRINT_STR(index, "mfheads before advancing...");
		nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);

		// @todo: check this please!

		// ----- log -----
		nx_LOG_MFTHREADS_ADVANCE(index, current_labels);
		// ----- log -----

		nx_Threads_advance(ctx->mfthreads, current_labels);

		// ----- log -----
		nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
		// ----- log -----

		nx_Thread* singleton = nx_Threads_singleton(ctx->mfthreads, &ctx->temp_storage);

		Assert(singleton && "There needs to be a singleton here!");

		new_node = nx_Thread_head(singleton);
		//		   new_node_prefix_length = Thread_head_depth(singleton) - pos->length_sum; // @check in general it might not match!
		new_node_shared = 1;

		nx_LOG_MFTHREADS_REWIND (index, current_labels.length);
		nx_Threads_rewind (ctx->mfthreads, current_labels.length);
	}

	nx_NodeWrap split_wnode;
	nx_NodeWrap child_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, nx_List_NodeP_get_reverse(&pos->path,0), index); // if shared, this child w_node won't change

	// surgery on the resulting hierarchy
	if (!shared) { // proper split

		nx_Node* parent = nx_HierarchyPosition_parent(pos);
		nx_Node* child = nx_HierarchyPosition_child(pos);

		u8 depth_child = depth_parent + child_wnode.path.length;
		u8 dist_bc = depth_child - depth_branch; // new_path depth from split

		// allocate prefix replace w_node
		split_wnode = nx_NanocubeIndex_allocate_node(ctx->nanocube, index, dist_pb, 2, child->root, 1, ctx->payload_context);
		nx_Path_copy_range (&split_wnode.path, &child_wnode.path, 0, dist_pb);

		if (dist_bc < child_wnode.path.length) { // update child w_node
			child_wnode = nx_NanocubeIndex_shrink_path(ctx->nanocube, index, child_wnode, dist_bc);
		}

		if (index == ctx->nanocube->dimensions - 1) { // copy set as initial content of prefix w_node
			// shared content that should be replaced...
			nx_PNode_payload_share ((nx_PNode*) split_wnode.raw_node, (nx_PNode*) child_wnode.raw_node, ctx->payload_context);
		} else {
			nx_INode_share_content ((nx_INode*) split_wnode.raw_node, (nx_INode*) child_wnode.raw_node);
			// ((INode*)split_wnode.self())->share_content_from((INode*)child_wnode.self()); // shared content that will be replaced in the upstream update
		}

		nx_Label start_label_new_node          = nx_Array_get (&remaining_path_list.labels,(u8) 0);
		nx_Label start_label_suffix_child_node = nx_Path_get (&child_wnode.path,0);

		// set the two children
		s32 i_new = (start_label_suffix_child_node < start_label_new_node);
		s32 i_child = 1 - i_new;

		nx_Child* new_slot = nx_Children_get_child(&split_wnode.children, i_new);
		nx_Child_set_node(new_slot, new_node);
		new_slot->label = start_label_new_node;
		new_slot->suffix_length = dist_bn;
		new_slot->shared = new_node_shared ? 1 : 0;

		// set parent
		if (!new_node_shared) {
			nx_Node_set_parent(new_node, split_wnode.raw_node);
		}

		nx_Child* child_slot = nx_Children_get_child(&split_wnode.children, i_child);
		nx_Child_set_node(child_slot, child);
		child_slot->label         = start_label_suffix_child_node;
		child_slot->suffix_length = dist_bc;
		child_slot->shared        = 0;

		nx_Node_set_parent(child, split_wnode.raw_node);

		child->root = 0;

		if (parent) {
			nx_NodeWrap parent_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, parent, index);
			nx_Child* parent_split_node_slot = nx_Children_get_child(&parent_wnode.children, nx_List_u8_get_reverse (&pos->child_indices,0));
			nx_Child_set_node(parent_split_node_slot, split_wnode.raw_node);
			parent_split_node_slot->shared = 0;
			parent_split_node_slot->suffix_length = split_wnode.path.length;
			// label should be the same
			nx_Node_set_parent(split_wnode.raw_node, parent);
		} else {
			nx_Node_set_parent(split_wnode.raw_node, context);
		}

		nx_List_NodeP_set_reverse(&pos->path, 0, split_wnode.raw_node);

		// ----- log ------
		nx_LOG_NODE_EVENT(ctx->nanocube, child, index, "shrink_path_update_parent");
		nx_LOG_NODE_EVENT(ctx->nanocube, split_wnode.raw_node, index, "create_split_node");
		// ----- log ------

	} else { // splitting on a solid edge on a shared hierarchy

		// clone path up to parent of branch edge
		nx_clone_path(ctx, index, context, pos, nx_CLONE_EXCLUDE_LAST_CHILD);

		nx_Node* parent = nx_HierarchyPosition_parent(pos);
		nx_Node* child	= nx_HierarchyPosition_child(pos);

		// p = parent, b = branch, c = existing child, dist = distance
		u8 depth_child = depth_parent + child->path_length;
		u8 dist_bc = depth_child - depth_branch;

		//
		// allocate new prefix w_node, and initialize it in the proper way
		//
		split_wnode = nx_NanocubeIndex_allocate_node(ctx->nanocube, index, dist_pb, 2, child->root, 1, ctx->payload_context);
		nx_Path_copy_range(&split_wnode.path, &child_wnode.path, 0, dist_pb);
		// split_wnode.path().copy(child_wnode.path(), 0, dist_pb); // copy path

		if (index == ctx->nanocube->dimensions - 1) { // copy set as initial content of prefix w_node
			nx_PNode_payload_share((nx_PNode*) split_wnode.raw_node, (nx_PNode*) child_wnode.raw_node, ctx->payload_context);
		} else {
			nx_INode_share_content((nx_INode*) split_wnode.raw_node, (nx_INode*) child_wnode.raw_node);
		}

		nx_Label start_label_new_node = nx_Array_get(&remaining_path_list.labels, (u8) 0);
		nx_Label start_label_suffix_child_node = nx_Path_get(&child_wnode.path, dist_pb);

		s32 i_new = (start_label_suffix_child_node < start_label_new_node);
		s32 i_child = 1 - i_new;

		nx_Child* new_slot = nx_Children_get_child (&split_wnode.children, i_new);
		nx_Child_set_node(new_slot, new_node);
		new_slot->label = start_label_new_node;
		new_slot->suffix_length = dist_bn;
		new_slot->shared = new_node_shared ? 1 : 0;

		// set parent
		if (!new_node_shared) {
			nx_Node_set_parent(new_node, split_wnode.raw_node);
		}

		nx_Child* child_slot = nx_Children_get_child(&split_wnode.children, i_child);
		nx_Child_set_node(child_slot, child);
		child_slot->label = start_label_suffix_child_node;
		child_slot->suffix_length = dist_bc;
		child_slot->shared = 1; // child is owned

		if (parent) {
			nx_NodeWrap parent_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, parent, index);
			nx_Child* parent_split_node_slot = nx_Children_get_child(&parent_wnode.children, nx_List_u8_get_reverse(&pos->child_indices,0));
			nx_Child_set_node(parent_split_node_slot, split_wnode.raw_node);
			parent_split_node_slot->shared = 0;
			parent_split_node_slot->suffix_length = split_wnode.path.length;
			// label should be the same
			nx_Node_set_parent(split_wnode.raw_node, parent);
		} else {
			nx_Node_set_parent(split_wnode.raw_node, context);
		}

		nx_List_NodeP_set_reverse(&pos->path, 0, split_wnode.raw_node); // replace child with the split w_node
		// for upstream updates

		// ----- log ------
		nx_LOG_NODE_EVENT(ctx->nanocube, child, index, "shrink_path_update_parent");
		nx_LOG_NODE_EVENT(ctx->nanocube, split_wnode.raw_node, index, "create_split_node");
		// ----- log ------
	}

	// generic procedude
	// first call is special
	// maybe here the only mfthread should be the new_node content?
	// @todo find right argument
	//
	// ----- log -----
	nx_LOG_MFTHREADS_ADVANCE(index, nx_Array_prefix(&current_labels, pos->length_sum));
	// ----- log -----

	nx_Threads_advance(ctx->mfthreads, nx_Array_prefix(&current_labels, pos->length_sum));

	// ----- log -----
	nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
	// ----- log -----

	nx_Nanocube_upstream_insert(ctx,
				    insert_path_list,
				    index,
				    (index < ctx->nanocube->dimensions - 1) ? nx_INode_content((nx_INode*)new_node) : 0,
				    shared ? (s32) nx_List_NodeP_size(&pos->path) : 1,
				    &pos->lengths,
				    &pos->path);


	//
	// 2017-03-22T17:02
	// what if the minimally finer is a payload????
	// maybe that is the case that we are seeing on bug4e?
	//


	//----- log -----
	nx_LOG_POP();
	//----- log -----

	// nx_pf_END_BLOCK();

	return nx_List_NodeP_get(&pos->path, 0);
}

/*
 * The path that we want to insert hits a shared edge that
 * that was split while inserting the current record in
 * finer levels. A sharing adjustment needs to be done.
 */
nx_INSERT_CASE(nx_insert_shared_suffix_was_split)
{
	++nx_INSERT_SHARED_SUFFIX_WAS_SPLIT;

	//----- log -----
	nx_LOG_PUSH_INSERT_CASE(pos->position_type, shared, index, ctx->nanocube, root);
	//----- log -----

	nx_Array current_labels = insert_path_list->labels;

	if (shared) {
		nx_clone_path(ctx, index, context, pos, nx_CLONE_EXCLUDE_LAST_CHILD);
	}

	nx_Node* child = nx_HierarchyPosition_child(pos);
	{
		// link to parent of previous shared w_node
		nx_Node* parent = nx_HierarchyPosition_parent(pos);

		nx_NodeWrap parent_node = nx_NanocubeIndex_to_node(ctx->nanocube, parent, index);
		nx_Child* child_slot = nx_Children_get_child(&parent_node.children, nx_List_u8_get_reverse(&pos->child_indices, 0));

		u8 new_len = child_slot->suffix_length - child->path_length;
		nx_Child_set_node(child_slot, nx_Node_parent(child));
		child_slot->suffix_length = new_len;

		child = nx_Node_parent(child);

		// ----- log -----
		nx_LOG_NODE_EVENT(ctx->nanocube, parent, index, "update_shared_child");
		// ----- log -----
	}

	nx_HierarchyPosition_pop_back(pos); // child shared w_node is solved

	// go to parent

	// ----- log -----
	nx_LOG_MFTHREADS_ADVANCE(index, nx_Array_prefix(&current_labels, pos->length_sum));
	// ----- log -----

	nx_Threads_advance(ctx->mfthreads, nx_Array_prefix(&current_labels, pos->length_sum));

	// ----- log -----
	nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
	// ----- log -----

	nx_Nanocube_upstream_insert(ctx,
				    insert_path_list,
				    index,
				    (index < ctx->nanocube->dimensions - 1) ? nx_INode_content((nx_INode*)child) : 0,
				    shared ? (s32)nx_List_NodeP_size(&pos->path) : 0,
				    &pos->lengths,
				    &pos->path);


	//----- log -----
	nx_LOG_POP();
	//----- log -----


	return nx_List_NodeP_get(&pos->path,0);

}

/*
 * The path that we want to insert hits a shared edge.
 * The new content requires to either
 * (1) branch this shared edge: shared_split
 * (2) replace completely this shared edge: shared_no_split
 */
nx_INSERT_CASE(nx_insert_shared_split_or_shared_no_split)
{
	if (pos->position_type == nx_SHARED_SPLIT) {
		++nx_INSERT_SHARED_SPLIT;
	} else {
		++nx_INSERT_SHARED_NO_SPLIT;
	}

	//----- log -----
	nx_LOG_PUSH_INSERT_CASE(pos->position_type, shared, index, ctx->nanocube, root);
	//----- log -----

	nx_Array current_labels = insert_path_list->labels;

	//
	// bail
	// std::cerr << "[log_insert] bailing on position.shared()..." << std::endl;
	// return path.get(0);
	//

	//
	// clone path except for destination w_node
	// of shared edge
	//

	/* @BUG(X) shared == false skips nx_clone_path */
	if (shared) {
		nx_clone_path(ctx, index, context, pos, nx_CLONE_EXCLUDE_LAST_CHILD);
	}

	//
	// setup start values for a loop that should drill down
	// to child
	//
	// keep copying down a new path for each shared edge we find
	// along the way (always checking the mfthreads as if we already
	// done the right insertion job somewhere else)
	//

	s32 new_proper_nodes = 0;

	while (1) {

		u8 depth_parent = pos->length_sum - nx_List_u8_get_reverse(&pos->lengths ,0);

		nx_Array remaining_labels = nx_Array_drop_prefix(&current_labels, (u8) depth_parent);

		nx_Node* parent = nx_HierarchyPosition_parent(pos);
		nx_Node* child  = nx_HierarchyPosition_child(pos);

		// ------ log ------
		nx_LOG_MFTHREADS_ADVANCE(index, nx_Array_prefix(&current_labels, pos->length_sum));
		// ------ log ------

		nx_Threads_advance(ctx->mfthreads, nx_Array_prefix(&current_labels,pos->length_sum));

		// ------ log ------
		nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
		// ------ log ------

		nx_Thread *singleton = nx_Threads_singleton(ctx->mfthreads, &ctx->temp_storage);

		/*
		 * Sanity check: if singleton, verify that current_labels up
		 * to length_sum are the same on the singleton path.
		 */
#if 0
		if (singleton) {
			u8 buf[nx_MAX_PATH_LENGTH];
			nx_List_u8 thread_path;
			nx_List_u8_init(&thread_path, buf, buf, buf + sizeof(buf));
			nx_Thread_head_path_labels(singleton, &thread_path);

			b8 c1 = pt_compare_memory((char*) current_labels.begin,
					(char*)	current_labels.begin + pos->length_sum,
					(char*)	thread_path.begin, (char*) thread_path.end);
			Assert(c1 == 0);

			u8 buf2[nx_MAX_PATH_LENGTH];
			nx_List_u8 thread_head_proper_path;
			nx_List_u8_init(&thread_head_proper_path, buf2, buf2, buf2 + sizeof(buf2));
			nx_NanocubeIndex_proper_path_to_root(ctx->nanocube, singleton->head_dimension,
					nx_Thread_head(singleton),
					nx_Thread_head_offset(singleton) + nx_Thread_head_prefix_size(singleton),
					&thread_head_proper_path);
			b8 c2 = pt_compare_memory((char*) thread_head_proper_path.begin,
					(char*)	thread_head_proper_path.end,
					(char*)	thread_path.begin, (char*) thread_path.end);
			Assert(c2 == 0);
		}
#endif

		b8 link_to_singleton =
			singleton
			?  (nx_NanocubeIndex_a_contains_b
					(ctx->nanocube, index,
					 nx_Thread_head(singleton),
					 nx_Thread_head_offset(singleton) + nx_Thread_head_prefix_size(singleton),
					 child, child->path_length))
			: 0;

		/* @BUG(X) singleton == false */
		if (singleton) {
			// ------ log ------
			nx_LOG_PRINT_PATH(index, ctx->nanocube, "a==singleton: ", nx_Thread_head(singleton),
					nx_Thread_head_offset (singleton) + nx_Thread_head_prefix_size (singleton));
			nx_LOG_PRINT_PATH(index, ctx->nanocube, "b==child:     ", child, child->path_length);
			nx_LOG_PRINT_STR(index, (link_to_singleton) ? "a contains b: yes" : "a contains b: no");
			// ------ log ------
		}

		if (link_to_singleton) {

			nx_NodeWrap parent_node = nx_NanocubeIndex_to_node(ctx->nanocube, parent, index);

			//----- log -----
			nx_LOG_PRINT_STR(index, "solve by linking to SHARED child");
			//----- log -----

			nx_Child* child_slot = nx_Children_get_child(&parent_node.children, nx_List_u8_get_reverse(&pos->child_indices,0));

			// @todo check suffix sizes etc. not sure what needs to be done
			nx_Child_set_node(child_slot, nx_Thread_head(singleton));

			// what is the correct suffix length in the case of a
			// shared split:
			child_slot->suffix_length = nx_List_u8_get_reverse(&pos->lengths,0);
			child_slot->shared = 1;

			//child_slot.suffix_length(pos->lengths.get_reverse(0));
			//child_slot.shared(true);

			//----- log -----
			nx_LOG_NODE_EVENT(ctx->nanocube, parent, index, "update_shared_child");
			nx_LOG_MFTHREADS_REWIND(index, pos->length_sum);
			//----- log -----

			nx_Threads_rewind(ctx->mfthreads,pos->length_sum);
			nx_HierarchyPosition_pop_back(pos); // everything is up to date

			break;

		} else {

			/* the split case is reached with 00 00, 00 01, 11 00 */
			/* @BUG(X) follows this path: !link_to_singleton */

			nx_LOG_MFTHREADS_REWIND(index, pos->length_sum);
			nx_Threads_rewind(ctx->mfthreads, pos->length_sum);

			if (pos->position_type == nx_SHARED_SPLIT) { // shared split

				++new_proper_nodes;

				//----- log -----
				nx_LOG_PRINT_STR(index, "there will be a split on a currently shared child: new intermediate branch node with two shared children");
				//----- log -----

				nx_NodeWrap parent_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, parent, index);
				nx_NodeWrap child_wnode  = nx_NanocubeIndex_to_node(ctx->nanocube, child, index);
				nx_Child* parent_child_slot = nx_Children_get_child(&parent_wnode.children, nx_List_u8_get_reverse(&pos->child_indices,0));

				u8 prev_suffix_length = parent_child_slot->suffix_length;
				u8 dist_pb = nx_List_u8_get_reverse(&pos->lengths, 0); // where to cut it
				u8 dist_bc = prev_suffix_length - dist_pb;

				nx_NodeWrap split_wnode = nx_NanocubeIndex_allocate_node(ctx->nanocube, index, dist_pb, 2, 0, 1, ctx->payload_context);
				nx_Path_copy_range(&split_wnode.path, &child_wnode.path, child->path_length - prev_suffix_length, dist_pb);

				if (index == ctx->nanocube->dimensions - 1) { // copy set as initial content of prefix w_node
					nx_PNode_payload_share((nx_PNode*)split_wnode.raw_node, (nx_PNode*)child_wnode.raw_node, ctx->payload_context);
				} else {
					nx_INode_share_content((nx_INode*)split_wnode.raw_node, (nx_INode*)child); // shared content
				}

				nx_Label start_label_new_node	       = nx_Array_get(&current_labels, pos->length_sum );
				nx_Label start_label_suffix_child_node = nx_Path_get(&child_wnode.path, child->path_length - dist_bc);

				// set the two children
				s32 i_new   = (start_label_suffix_child_node < start_label_new_node);
				s32 i_child = 1 - i_new;

				nx_Child* child_slot = nx_Children_get_child(&split_wnode.children, i_child);
				nx_Child_set_node(child_slot, child);
				child_slot->label = start_label_suffix_child_node;
				child_slot->suffix_length = dist_bc;
				child_slot->shared = 1;

				// ----- log -----
				nx_LOG_MFTHREADS_ADVANCE(index, current_labels);
				// ----- log -----

				nx_Threads_advance(ctx->mfthreads, current_labels);

				// ----- log -----
				nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
				// ----- log -----

				nx_Thread* singleton2 = nx_Threads_singleton(ctx->mfthreads, &ctx->temp_storage);

				Assert(singleton2 && "there needs to be a singleton here!!!");

				//
				// just link with the singleton and we are done
				// align for upstream insertions
				//
				// auto new_node = Thread_head(singleton);
				u8 new_node_depth = nx_Thread_head_depth(singleton2); //  +new_node->path_length - Thread_head_prefix_size(singleton);
				// auto prefix		= singleton->head_prefix_size();
				u8 new_node_suffix = new_node_depth - pos->length_sum;

				nx_Child* new_slot = nx_Children_get_child(&split_wnode.children, i_new);
				nx_Child_set_node(new_slot, nx_Thread_head(singleton2));
				new_slot->label = start_label_new_node;
				new_slot->suffix_length = new_node_suffix;
				new_slot->shared = 1;

				// fix parent child link
				nx_Child_set_node(parent_child_slot, split_wnode.raw_node);
				parent_child_slot->shared = 0;
				parent_child_slot->suffix_length = split_wnode.path.length;

				nx_Node_set_parent(split_wnode.raw_node, parent);

				nx_List_NodeP_set_reverse(&pos->path, 0, split_wnode.raw_node);

				// ----- log -----
				nx_LOG_NODE_EVENT(ctx->nanocube, split_wnode.raw_node, index, "create_split_node");
				nx_LOG_NODE_EVENT(ctx->nanocube, parent_wnode.raw_node, index, "update_child");
				nx_LOG_MFTHREADS_REWIND(index, current_labels.length);
				// ----- log -----

				nx_Threads_rewind(ctx->mfthreads, current_labels.length);

				break;

				// end of shared split
			} else {
				// shared no split

				//----- log -----
				nx_LOG_PRINT_STR(index, "replace exact shared branch with a proper one and insert new record on that branch");
				//----- log -----

				++new_proper_nodes;

				nx_NodeWrap child_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, child, index);

				u8 suffix_length = nx_List_u8_get_reverse(&pos->lengths,0);

				nx_NodeWrap child_copy_node = nx_NanocubeIndex_allocate_node(ctx->nanocube, index, suffix_length, nx_Node_degree(child), 0, 0, ctx->payload_context);

				nx_Node* child_copy = child_copy_node.raw_node;

				Assert(child->path_length >= suffix_length && "oooops not considered!");

				nx_Path_copy_range(&child_copy_node.path, &child_wnode.path, child->path_length - suffix_length, suffix_length); // copy path

				nx_Children *children_copy = &child_copy_node.children; // copy children make them
				nx_Children_copy(children_copy, &child_wnode.children);
				for (s32 i=0; i<children_copy->length; ++i)
					nx_Children_get_child(children_copy, i)->shared = 1; // make all shared

				nx_Node_set_parent(child_copy, parent);

				if (index < ctx->nanocube->dimensions - 1) { // set content
					nx_INode_share_content((nx_INode*)child_copy, (nx_INode*)child);
				} else {
					nx_PNode_payload_share((nx_PNode*)child_copy, (nx_PNode*)child, ctx->payload_context);
				}

				nx_NodeWrap parent_node = nx_NanocubeIndex_to_node(ctx->nanocube, parent, index);
				nx_Child* child_slot = nx_Children_get_child(&parent_node.children, nx_List_u8_get_reverse(&pos->child_indices,0));
				nx_Child_set_node(child_slot, child_copy);
				child_slot->shared = 0;


				// ----- log -----
				nx_LOG_NODE_EVENT(ctx->nanocube, child_copy, index, "create_child_proper_copy");
				nx_LOG_NODE_EVENT(ctx->nanocube, parent_node.raw_node, index, "update_child");
				nx_LOG_MFTHREADS_REWIND(index, current_labels.length);
				// ----- log -----

				nx_HierarchyPosition_pop_back(pos);

				nx_HierarchyPosition_find_continue(pos, index, remaining_labels);

				// it will be a branch down here!
				if (pos->position_type == nx_EXACT) {
					Assert(nx_Node_degree(child_copy) == 0 && "sanity check if we are at a leaf");
					break;
				} else if (pos->position_type == nx_BRANCH) {
					nx_Node* branch = nx_HierarchyPosition_child(pos);
					nx_NodeWrap branch_wnode = nx_NanocubeIndex_to_node(ctx->nanocube, branch, index);

					// ----- log -----
					nx_LOG_MFTHREADS_ADVANCE(index, current_labels);
					// ----- log -----

					nx_Threads_advance(ctx->mfthreads, current_labels);

					// ----- log -----
					nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
					// ----- log -----

					nx_Thread* singleton2 = nx_Threads_singleton(ctx->mfthreads, &ctx->temp_storage);

					Assert(singleton2 && "there needs to be a singleton here!!!");

					nx_Child shared_child_to_insert;
					nx_Child_init(&shared_child_to_insert);
					shared_child_to_insert.label = nx_Array_get(&current_labels,pos->length_sum);
					shared_child_to_insert.shared = 1;
					shared_child_to_insert.suffix_length = nx_Thread_head_depth(singleton2) - pos->length_sum;
					nx_Child_set_node(&shared_child_to_insert, nx_Thread_head(singleton2));

					if (!nx_Children_available_capacity(&branch_wnode.children)) {
						branch_wnode = nx_NanocubeIndex_add_child_slot(ctx->nanocube, index, branch_wnode);
					}

					Assert(pos->child_newchild_index >= 0);

					nx_Children_insert(&branch_wnode.children, pos->child_newchild_index, &shared_child_to_insert);
					nx_Node_set_degree(branch, nx_Node_degree(branch) + 1);
					// branch->degree += 1; // update degree of child

					// ----- log -----
					nx_LOG_NODE_EVENT(ctx->nanocube, branch, index, "new_shared_branch");
					nx_LOG_MFTHREADS_REWIND(index, current_labels.length);
					// ----- log -----

					nx_Threads_rewind(ctx->mfthreads, current_labels.length);

					break;
				} else if (pos->position_type == nx_SHARED_NO_SPLIT) {
					continue;
				} else if (pos->position_type == nx_SHARED_SPLIT) {
					continue;
				} else {
					Assert(0 && "case not expected/implemented");
				}
			}
		}
	}

	/*
	 * @TODO(llins): what is the assumption with respect to
	 * pos when we get to this point. After the shared split
	 * I haven't seen a "pos" update after the surgeries.
	 */
	new_proper_nodes = shared ? (s32) nx_List_NodeP_size(&pos->path) : new_proper_nodes;

	// insert in all nodes upstream of path
	// @todo: revisit this nullptr here!
	//----- log -----
	nx_LOG_MFTHREADS_ADVANCE(index, nx_Array_prefix(&current_labels, pos->length_sum));
	//----- log -----

	nx_Threads_advance(ctx->mfthreads, nx_Array_prefix(&current_labels, pos->length_sum));

	// ----- log -----
	nx_LOG_PRINT_MFHEADS(index, ctx->nanocube, ctx->mfthreads);
	// ----- log -----

	/*
	 * @BUG is happening in this path on the
	 * yellow cabs dataset of june 2016
	 */
	nx_Nanocube_upstream_insert(ctx,
				    insert_path_list,
				    index,
				    0,
				    new_proper_nodes,
				    &pos->lengths,
				    &pos->path);

	//----- log -----
	nx_LOG_POP();
	//----- log -----

	return nx_List_NodeP_get(&pos->path,0);
}

/*
 * The "shared" flag indicates the hierarchy rooted at
 * "r" is not proper to the node in the previous
 * dimension "index-1" that is being updated with
 * the new record. A proper root and possible path
 * will need to be created accordingly.
 */
static nx_Node*
nx_insert_recursive(nx_InsertionContext *ctx,
				  nx_Node *context,
				  nx_Node *root,
				  b8 shared,
				  s32 index,
				  nx_LabelArrayList* insert_path_list)
{
// 	++INSERT_RECURSIVE_COUNT;
// 	if (INSERT_RECURSIVE_COUNT == 124552554) {
// 		INSERT_RECURSIVE_COUNT += 2;
// 	}

	Assert(root != 0 || ctx->mfthreads->size == 0); // there cannot be any thread when current root_p is null

	if (root == 0) {

		//----- log -----
		nx_LOG_PUSH_INSERT_CASE(nx_SINGLETON, shared, index, ctx->nanocube, root);
		//----- log -----

		//----- log -----
		nx_LOG_APPEND_PATH(insert_path_list);
		//----- log -----

		nx_Node *result = nx_NanocubeIndex_allocate_singleton (ctx->nanocube, index, 1, insert_path_list, ctx->payload_unit, ctx->payload_context);

		//----- log -----
		nx_LOG_POP();
		//----- log -----

		return result;

	} else {

		/*
		 * trace path at dimension "index" starting on root "root"
		 * until we either find a shared parent-child link, a dead-end,
		 * or the exact path we were looking for described in "current_labels"
		 */
		nx_HierarchyPosition pos;
		nx_HierarchyPosition_init(&pos, ctx->nanocube);
		nx_HierarchyPosition_find_first(&pos, ctx->nanocube, index, root, insert_path_list->labels, 1);

		if (pos.position_type == nx_EXACT) {

			return nx_insert_exact(ctx, context, root,
					shared, index, insert_path_list,
					&pos);

		} else if (pos.position_type == nx_BRANCH) { // branch on an existing w_node

			return nx_insert_branch(ctx, context, root,
					shared, index, insert_path_list,
					&pos);

		} else if (pos.position_type == nx_SPLIT) {

			return nx_insert_split(ctx, context, root,
					shared, index, insert_path_list,
					&pos);

		} else if (pos.position_type == nx_SHARED_SUFFIX_WAS_SPLIT) {

			return nx_insert_shared_suffix_was_split(ctx,
					context, root, shared, index, insert_path_list,
					&pos);

		} else if (pos.position_type == nx_SHARED_NO_SPLIT
				|| pos.position_type == nx_SHARED_SPLIT) {

			return nx_insert_shared_split_or_shared_no_split(ctx,
					context, root, shared, index, insert_path_list,
					&pos);

		}
		Assert(0 && "Problem!");
		return 0;
	}

}

// should use the span template from the C++ guidelines
static void
nx_NanocubeIndex_insert(nx_NanocubeIndex *self, nx_LabelArrayList* labels, void *payload_unit, void *payload_context, MemoryBlock buffer)
{

	nx_pf_BEGIN_BLOCK("nx_NanocubeIndex_insert")

	++COUNT;

	// ------ log ------
	nx_LOG_PUSH_RECORD(1 + COUNT);
	// ------ log ------

#if 0
	Print *print = &debug_request->print;
	Print_clear(print);
	static u64 last_context = 0;
	u64 context = (u64) payload_context;
	if (context != last_context) {
		Print_u64(print, (u64) payload_context);
		Print_cstr(print, "\n");
		Request_print(debug_request, print);
		last_context = context;
	}
#endif

	nx_InsertionContext ctx;
	ctx.nanocube = self;
	ctx.payload_unit = payload_unit;
	ctx.payload_context = payload_context;
	BilinearAllocator_init(&ctx.temp_storage, buffer.begin, buffer.end - buffer.begin);

	ctx.mfthreads = (nx_Threads*) BilinearAllocator_alloc_left_aligned(&ctx.temp_storage, sizeof(nx_Threads), 8);
	ctx.mfthreads->initialized = 0;
	nx_Threads_init(ctx.mfthreads, self);

	nx_Node* r = nx_insert_recursive(&ctx, 0, nx_Ptr_Node_get(&self->root_p), 0, 0, labels);
	nx_Ptr_Node_set(&self->root_p,r);

	++self->number_of_records;

	// ------ log ------
	nx_LOG_POP();
	// ------ log ------

	nx_pf_END_BLOCK();
}

/* NOTE(llins): nx_initialize() should be called before using nx_ services */
static void
nx_start()
{
	nx_initialize_precomputed_detail_info();
	//----- log -----
	nx_LOG_START();
	//----- log -----
}

static void
nx_finish()
{
	//----- log -----
	nx_LOG_FINISH();
	//----- log -----
}


