//------------------------------------------------------------------------------
// Ptr
//------------------------------------------------------------------------------

//
// 6 bytes Offset pointer to enable position independent
// data structures
//
typedef struct {
	u16 data[3];
} Ptr;

#define PTR_SPECIALIZED_TYPE(name) typedef struct { Ptr ptr; } name ;

//
// check macro on nanocube_platform.c to simplify the use
// of specialized pointers:
//
//     PTR_SPECIALIZED_TYPE(name)
//     PTR_SPECIALIZED_SERVICES(name)
//
//------------------------------------------------------------------------------
// Ptr
//------------------------------------------------------------------------------

static void
Ptr_reset(Ptr *self, s64 offset)
{
	if (offset > 0) {
		// *((u32*) &self->data)     = *  ( (u32*) &offset     );
	        u16 *ptr = (u16*) &offset;
		self->data[0] = *( ptr + 0 );
		self->data[1] = *( ptr + 1 );
		self->data[2] = *( ptr + 2 );
	}
	else {
		offset = -offset;
	        u16 *ptr = (u16*) &offset;
		self->data[0] = *( ptr + 0 );
		self->data[1] = *( ptr + 1 );
		self->data[2] = *( ptr + 2 ) + (u16) 0x8000;
// 		*((u32*) &self->data)     = *  ( (u32*) &offset     );
// 		self->data[2] = *( (u16*) &offset + 2 ) + (u16) 0x8000;
	}
}

static void
Ptr_set_null(Ptr *self)
{
	self->data[0] = 1;
	self->data[1] = 0;
	self->data[2] = 0;
}

static void
Ptr_set(Ptr *self, void* p)
{
	if (p) {
		s64 offset = (s64) ((char*) p - (char*) self);
		Ptr_reset(self, offset);
	} else {
		Ptr_set_null(self);
	}
}


// NOTE(llins): 2017-06-20T16:20
// How can we make a 6-bytes pointer more efficient
// in some experiments this func was taking on average
// 71 sycles per hit
static s64
Ptr_offset(Ptr *self)
{

	// pf_BEGIN_BLOCK("Ptr_offset");

	/* this is always a positiver number */
	s64 offset = ((s64)self->data[0]) +
		     ((s64)self->data[1] << 16) +
		     ((s64)self->data[2] << 32);

	/* but if bit 48 */
	if (offset & 0x800000000000ll) {
		offset = -(offset & 0x7fffffffffffll);
	}

	// pf_END_BLOCK();

	return offset;
}

static void*
Ptr_get_not_null(Ptr *self)
{
	return ((char*) self + Ptr_offset(self));
}


static b8
Ptr_is_null(Ptr *self)
{
	return self->data[0] == 1 && self->data[1] == 0 && self->data[2] == 0;
}

static b8
Ptr_is_not_null(Ptr *self)
{
	return !Ptr_is_null(self);
}

static void*
Ptr_get(Ptr *self)
{
	return Ptr_is_not_null(self) ? Ptr_get_not_null(self) : 0;
}

static void
Ptr_copy(Ptr *self, Ptr *other)
{
	Ptr_set(self, Ptr_get(other));
}

static void
Ptr_swap(Ptr *self, Ptr *other)
{
	void *a = Ptr_get(self);
	void *b = Ptr_get(other);
	Ptr_set(self, b);
	Ptr_set(other, a);
}

// trick gcc -E -CC x.c to preserve comments and see the macro in multiple lines
#define PTR_SPECIALIZED_SERVICES(name, base) /*
*/ void name##_reset(name *self, s64 offset) { Ptr_reset(&self->ptr, offset); } /*
*/ void name##_set_null(name *self) { Ptr_set_null(&self->ptr); } /*
*/ void name##_set(name *self, base *p) { Ptr_set(&self->ptr, p); } /*
*/ b8 name##_is_not_null(name *self) { return Ptr_is_not_null(&self->ptr); } /*
*/ b8 name##_is_null(name *self) { return Ptr_is_null(&self->ptr); } /*
*/ base * name##_get(name *self) { return ((base *) Ptr_get(&self->ptr)); } /*
*/ base * name##_get_not_null(name *self) { return (base *) Ptr_get_not_null(&self->ptr); } /*
*/ void name##_copy(name *self, name *other) { Ptr_copy(&self->ptr, &other->ptr); } /*
*/ void name##_swap(name *self, name *other) { Ptr_swap(&self->ptr, &other->ptr); }

static void
pt_rotate_Ptr(Ptr *begin, Ptr *middle, Ptr *end)
{
	Ptr* next = middle;
	while (begin != next) {
		Ptr_swap(begin,next);
		++begin;
		++next;
		if (next == end) {
			next = middle;
		} else if (begin == middle) {
			middle = next;
		}
	}
}

#define pt_DEFINE_ARRAY(name, base) \
typedef struct { \
	base *begin; \
	base *end; \
	base *capacity; \
} name;

#define pt_DEFINE_ARRAY_SERVICES(name, base) \
static void \
name ## _init(name *self, base *begin, base *capacity) { \
	self->begin = begin; \
	self->end   = begin; \
	self->capacity = capacity; \
} \
static void \
name ## _append(name *self, base value) { \
	Assert(self->end != self->capacity); \
	*self->end = value; \
	++self->end; \
} \
static base \
name ## _get(name *self, u64 index) { \
	Assert(self->begin + index < self->end); \
	return self->begin[index]; \
} \
static void \
name ## _set(name *self, u64 index, base value) { \
	Assert(self->begin + index < self->end); \
	self->begin[index] = value; \
} \
static void \
name ## _clear(name *self) { \
	self->end = self->begin; \
} \
static u64 \
name ## _size(name *self) { \
	return (u64) (self->end - self->begin); \
}

/* define ArrayMemoryBlock */
pt_DEFINE_ARRAY(ArrayMemoryBlock, MemoryBlock)
pt_DEFINE_ARRAY_SERVICES(ArrayMemoryBlock, MemoryBlock)

/* define ArrayMemoryBlock */
pt_DEFINE_ARRAY(Array_u64, u64)
pt_DEFINE_ARRAY_SERVICES(Array_u64, u64)

/* define ArrayMemoryBlock */
pt_DEFINE_ARRAY(Array_u128, u128)
pt_DEFINE_ARRAY_SERVICES(Array_u128, u128)
