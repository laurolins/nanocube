/*
 *
 * ntp_ is time parser moduel
 *
 * depends on
 *
 *      tm_ time module
 *      nt_ tokenizer module
 */

#ifdef sort_UNIT_TEST
#include "platform.c"
#endif


/* this should move to algorithm module */

typedef struct {
	u64 key;
	u32 index;
} sort_Entry;


inline void
sort_swap(sort_Entry *a, sort_Entry *b)
{
	sort_Entry tmp = *b;
	*b = *a;
	*a = tmp;
}

static void
sort_radixsort(sort_Entry *begin, sort_Entry *tmp, u32 count)
{
	sort_Entry *source = begin;
	sort_Entry *dest   = tmp;

	for (u32 byte_index=0;byte_index<64;byte_index+=8) {

		u32 sort_key_offsets[256] = { 0 };
		// pt_fill((char*) &sort_key_offsets, (char*) &sort_key_offsets + ArrayCount(sort_key_offsets), 0);

		for (u32 i=0;i<count;++i) {
			u32 radix_piece = (u32) (source[i].key >> byte_index) & 0xff;
			++sort_key_offsets[radix_piece];
		}

		u32 total = 0;
		for (u32 i=0;i<ArrayCount(sort_key_offsets);++i) {
			u32 c = sort_key_offsets[i];
			sort_key_offsets[i] = total;
			total += c;
		}

		for (u32 i=0;i<count;++i) {
			u32 radix_piece = (u32) (source[i].key >> byte_index) & 0xff;
			dest[sort_key_offsets[radix_piece]] = source[i];
			++sort_key_offsets[radix_piece];
		}

		sort_Entry *tmp = dest;
		dest = source;
		source = tmp;
	}

	Assert(source == begin);
}

// 	if (count == 1) {
// 		return;
// 	} else if (count == 2) {
// 		sort_Entry *a = begin;
// 		sort_Entry *b = begin + 1;
// 		if (a->key > b->key) {
// 			sort_Swap(a, b);
// 		}
// 	} else {
// 	}

// http://alienryderflex.com/quicksort/
static void
sort_quick_s32(s32 *arr, s32 elements) {
	s32 piv, beg[300], end[300], i=0, L, R, swap;
	beg[0]=0; end[0]=elements;
	while (i>=0) {
		L=beg[i]; R=end[i]-1;
		if (L<R) {
			piv=arr[L];
			while (L<R) {
				while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
				while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L];
			}
			arr[L]=piv;
			beg[i+1]=L+1;
			end[i+1]=end[i];
			end[i++]=L;
			if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
				swap=beg[i]; beg[i]=beg[i-1]; beg[i-1]=swap;
				swap=end[i]; end[i]=end[i-1]; end[i-1]=swap;
			}
		} else {
			i--;
		}
	}
}

// assumes sorted array of s32
static s32
sort_pack_s32(s32 *arr, s32 elements) {
	if (elements == 0) return elements;
	s32 i=0;
	s32 j=1;
	for (;;) {
		while (j<elements && arr[j] == arr[i]) ++j;
		if (j == elements) return i+1;
		++i;
		arr[i] = arr[j];
		++j;
	}
}

static s32
sort_indexof_s32(s32 *arr, s32 elements, s32 value)
{
	s32 l = 0;
	s32 r = elements;
	//
	// l     r
	// 0 1 2 3
	//
	s32 n = r - l;
	while (n > 2) {
		s32 m = (r + l) / 2;
		if (arr[m] < value) l = m;
		else r = m + 1;
		n = r - l;
	}
	// n is either 0, 1 or 2
	if (n == 0) return -1;
	else if (value < arr[l]) return -(l+1);
	else if (value == arr[l]) return l;
	else if (n == 1 || value < arr[l+1]) return -(l+2);
	else if (value == arr[l+1]) return l+1;
	else return -(l+3);
}



#ifdef sort_UNIT_TEST

#include <stdio.h>

int
main()
{
	s32 data[] = { 10, 10, 3, 3, 5, 5, 1, 2, 4 };
	s32 n = ArrayCount(data);

	sort_quick_s32(data, n);

	printf("sorted array: ");
	for (s32 i=0;i<n;++i)
		printf("%d ", data[i]);
	printf("\n");

	n = sort_pack_s32(data, n);

	printf("packed array: ");
	for (s32 i=0;i<n;++i)
		printf("%d ", data[i]);
	printf("\n");

	s32 x = 6;
	s32 index = sort_indexof_s32(data, n, x);
	printf("indexof of %d is %d\n", x, index);
}

#endif




