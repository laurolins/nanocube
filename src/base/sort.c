
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

internal void
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

