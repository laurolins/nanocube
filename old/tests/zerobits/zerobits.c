
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <x86intrin.h>

typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;
typedef float f32;

typedef struct {
	u32 hits;
	f32 min;
	f32 max;
	f32 sum;
} Stats;

static void
Stats_init(Stats *self)
{
	self->hits = 0;
	self->min  =  FLT_MAX;
	self->max  = FLT_MIN;
	self->sum  = 0;
}

static void
Stats_insert(Stats *self, f32 value)
{
	++self->hits;
	if (value < self->min) self->min = value;
	if (value > self->max) self->max = value;
	self->sum += value;
}

static void
Stats_report(Stats *self)
{
	printf("h %-4d  sum.cy %-6.0f  min.cy %-6.0f cy/h %-6.0f max.cy %-6.0f\n",
	       self->hits,
	       self->sum,
	       self->min,
	       self->sum/self->hits,
	       self->max);
}

int main()
{
	Stats lz_stats;
	Stats tz_stats;

	Stats_init(&lz_stats);
	Stats_init(&tz_stats);

	u64 tz_sum = 0;
	u64 lz_sum = 0;
	for (u32 i=0;i<10000000;++i) {
		u64 t0 = __rdtsc();
		u32 leading_zeros  = __builtin_clz(i);
		// s32 leading_zeros  = __lzcnt32(i);
		t0 = __rdtsc() - t0;
		Stats_insert(&lz_stats, (f32) t0);

		u64 t1 = __rdtsc();
		u32 trailing_zeros = __builtin_ctz(i);
		// s32 trailing_zeros = __tzcnt32(i);
		t1 = __rdtsc() - t1;
		Stats_insert(&tz_stats, (f32) t1);
	}

	// printf(".... %d\n", i);
	printf("leading zeros stats\n");
	Stats_report(&lz_stats);
	printf("trailing zeros stats\n");
	Stats_report(&tz_stats);
	// printf("trailing zeros %3d in %5llu cy\n", trailing_zeros, t1); 
}
