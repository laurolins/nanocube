// need a ring of variable length each item needs to save its length

/*

initial deque

  R
  L
[ - - - ]

push right 1

     R
   L
 [ 1 - - ]

push right 2 (deque is full) ((R + 1) % C == L)

       R
   L
 [ 1 2 - ]

pop left

       R
     L
 [ - 2 - ]

push left 3 (deque is full)

       R
   L
 [ 3 2 - ]

pop left

       R
     L
 [ - 2 - ]

push right 4

   R
     L
 [ - 2 4 ]

*/

// head
#define hq__head(a) ((u32*) (a) - 4)
#define hq__cap(a)   hq__head(a)[0]
#define hq__left(a)  hq__head(a)[1]
#define hq__right(a) hq__head(a)[2]
#define hq__full(a)  (((hq__right(a) + 1) % hq__cap(a)) == hq__left(a))
#define hq__empty(a) (hq__left(a) == hq__right(a))

#define hq_HEAD_SIZE 4*sizeof(s32)
//
#define hq_cap(a)    ((a) ? hq__cap(a) : 0)
#define hq_left(a)   ((a) ? hq__left(a) : 0)
#define hq_right(a)  ((a) ? hq__right(a) : 0)
#define hq_size(a)   ((a) ? ( (hq__right(a) >= hq__left(a)) ? (hq__right(a) - hq__left(a)) : (hq__cap(a) + hq__right(a) - hq__left(a)) ) : 0)
#define hq_full(a)   ((a) ? ( ((hq__right(a) + 1) % hq__cap(a)) == hq__left(a)) : 0)
#define hq_last(a)   (((a) && (hq__left(a) != hq__right(a))) ? ((a) + (hq__right(a) > 0 ? (hq__right(a)-1) : (hq__cap(a)-1))) : 0)
#define hq_first(a)  ((a) ? ((a) + hq__left(a)) : 0)
// 0 is the head and so on
#define hq_get(a,i)     ((a) ? ((i) < hq__size(a) ? ((a) + (hq__left(a) + i) % hq__cap(a)): 0)
#define hq_enqueue(a,v) (((a) && !hq__full(a))  ? ((a)[hq__right(a)++] = (v), hq__right(a) %= hq__cap(a), 1) : 0)
#define hq_dequeue(a)   (((a) && !hq__empty(a)) ? (hq__left(a)++, hq__left(a) %= hq__cap(a), 1) : 0)

// same as unqueue
#define hq_push_last(a,v)  (((a) && !hq__full(a))  ? ((a)[hq__right(a)++] = (v), hq__right(a) %= hq__cap(a), 1) : 0)
// same as dequeue
#define hq_pop_first(a)    (((a) && !hq__empty(a)) ? (hq__left(a)++, hq__left(a) %= hq__cap(a), 1) : 0)

#define hq_push_first(a,v) (((a) && !hq__full(a)) ? ( (hq__left(a) > 0) ?  ((a)[--hq__left(a)] = (v), 1) : ((a)[(hq__left(a)=(hq__cap(a)-1))] = (v), 1) : 0))
#define hq_pop_last(a)     (((a) && !hq__empty(a)) ? (hq__right(a)++, hq__right(a) %= hq__cap(a), 1) : 0)

// push may fail which should be tested in the calling site
// #define hq_push(a,v) ((a) && (hq__size(a) < hq__cap(a)) ? ((a)[hq__size(a)++] = (v), 1) : 0)
// #define hq_pop(a,v) ((a) && (hq__size(a) > 0) ? (--hq__size(a), 1) : 0)

// note we use the expected platform to allocate raw bytes
// this is not in an arena, but it cound be
#define hq_new(a,c)  ((a)=(void*)((char*)platform.allocate_memory_raw(c*sizeof(*a)+hq_HEAD_SIZE,0)+hq_HEAD_SIZE),hq__left(a)=0,hq__right(a)=0,hq__cap(a)=c,1)
#define hq_new_from_arena(a,c,arena) ((a)=(void*)((char*)a_push((arena),c*sizeof(*a)+hq_HEAD_SIZE,8,0)+hq_HEAD_SIZE), (a) ? (hq__left(a)=0,hq__right(a)=0,hq__cap(a)=c,1) : 0)

#define hq_clear(a)  ((a) ? (hq__left(a)=0, hq__right(a)=0, 1) : 1);

