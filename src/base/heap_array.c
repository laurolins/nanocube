//
// Simple Stretchy Buffer infra-structure
//
// head
#define ha__head(a) ((s32*) (a) - 2)
// capacity
#define ha__cap(a) ha__head(a)[0]
// num items
#define ha__size(a) ha__head(a)[1]
#define ha_HEAD_SIZE 2*sizeof(s32)
//
#define ha_cap(a)    ((a) ? ha__cap(a) : 0)
#define ha_size(a)   ((a) ? ha__size(a) : 0)

// push may fail which should be tested in the calling site
#define ha_push(a,v) ((a) && (ha__size(a) < ha__cap(a)) ? ((a)[ha__size(a)++] = (v), 1) : 0)
#define ha_pop(a) ((a) && (ha__size(a) > 0) ? (--ha__size(a), 1) : 0)

// note we use the platform to allocate raw bytes and doesn't use an arena
#define ha_new(a,c)  ((a)=(void*)((char*)platform.allocate_memory_raw(c*sizeof(*a)+ha_HEAD_SIZE,0)+ha_HEAD_SIZE),ha__size(a)=0,ha__cap(a)=c,1)
#define ha_new_from_arena(a,c,arena) ((a)=(void*)((char*)a_push((arena),c*sizeof(*a)+ha_HEAD_SIZE,8,0)+ha_HEAD_SIZE), (a) ? (ha__size(a)=0,ha__cap(a)=c,1) : 0)

#define ha_clear(a)  ((a) ? ((ha__size(a)=0), 1) : 1)
#define ha_free(a)  ((a) ? (platform.free_memory_raw(ha__head(a)),1) : 1)

#define ha_set_size(a,s) ((a)?((s<=ha__cap(a))?((ha__size(a)=s),1):0):0)

