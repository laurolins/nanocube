
//
// some utility functions to deal with string and null terminated string
//

static char*
cstr_end(char *begin)
{
	while(*begin != 0)
		++begin;
	return begin;
}

static s64
cstr_len(char *begin)
{
	char *it = begin;
	while(*it != 0)
		++it;
	return it - begin;
}


static s64
cstr_len0(char *begin)
{
	char *it = begin;
	while(*it != 0)
		++it;
	return it - begin + 1;
}

//
// this will truncate if not enough space on destination
// return 0 if there was truncation, 0 otherwise
//
static s32
cstr_copy_and_null_terminate(char *src, char *dst, s64 dst_size)
{
	while (*src != 0 && dst_size > 1) {
		*dst = *src;
		++dst;
		++src;
		--dst_size;
	}
	*dst = 0;
	return (*src == 0);
}

static void
cstr_copy_prefix_and_null_terminate(char *src, char *dst, s32 count)
{
	while (count > 0) {
		*dst = *src;
		++dst;
		++src;
		--count;
	}
	*dst = 0;
}

static char*
cstr_to_upper(char *st)
{
	char *it = st;
	s32 shift = (s32) 'A' - (s32) 'a';
	while (*it != 0) {
		if (*it >= 'a' && *it <= 'z') {
			*it = (s32) (*it) + shift;
		}
		++it;
	}
	return st;
}

static u64
cstr_count_char(char *st, char c)
{
	u64 count = 0;
	while (*st) {
		if (*st == c) {
			++count;
		}
		++st;
	}
	return count;
}

static char*
cstr_replace(char *st, char old, char new)
{
	char *it = st;
	while (*it) {
		if (*it == old) {
			*it = new;
		}
		++it;
	}
	return st;
}

// erase characters in 'set'
// if 'not', then erase characters not in 'set'
static char*
cstr_erase(char *st, char *set, s32 not)
{
	char *it  = st;
	char *end = it;
	while (*it) {
		s32 it_is_in_set = 0;
		char *it_test = set;
		while (*it_test) {
			if (*it_test == *it)  {
				it_is_in_set = 1;
				break;
			} else {
				++it_test;
			}
		}

		s32 erase = it_is_in_set;
		if (not) erase = !erase;

		if (erase) {
			++it;
		} else {
			*end = *it;
			++end;
			++it;
		}
	}
	return end;
}

static u64
cstr_length(char *a)
{
	u64 len = 0;
	while (*a) {
		++a;
		++len;
	}
	return len;
}

static s32
cstr_compare(char *a, char *b)
{
	for (;;) {
		char ca = *a;
		char cb = *b;
		if (ca == 0) {
			if (cb == 0) {
				return 0;
			} else {
				return -1;
			}
		} else if (cb == 0) {
			return 1;
		} else if (ca < cb) {
			return -1;
		} else if (ca > cb) {
			return 1;
		} else {
			++a;
			++b;
		}
	}
	return 0;
}

static s32
cstr_match_n(char *a, char *b, s32 n)
{
	while (n > 0) {
		if (*a != *b) {
			return 0;
		}
		++a;
		++b;
		--n;
	}
	return 1;
}

static s32
cstr_starts_with(char *st, char *prefix)
{
	return cstr_match_n(st, prefix, (s32)cstr_length(prefix));
}

static s32
cstr_ends_with(char *st, char *suffix)
{
	s32 n_st = cstr_len(st);
	s32 n_suffix = cstr_len(suffix);

	if (n_suffix > n_st) {
		return 0;
	} else if (n_suffix==0) {
		return 1;
	}
	char *it_st     = st + n_st - 1;
	char *it_suffix = suffix + n_suffix - 1;
	for (;;) {
		if (*it_st != *it_suffix) {
			return 0;
		}
		if (suffix == it_suffix) {
			return 1;
		} else {
			--it_st;
			--it_suffix;
		}
	}
}

#define cstr_match(cstrA,cstrB) (cstr_compare(cstrA,cstrB)==0)

//------------------------------------------------------------------------------
//  string utility
//------------------------------------------------------------------------------

static s64
cstr_compare_memory(char* b1, char* e1, char* b2, char* e2)
{
	char *i1 = b1;
	char *i2 = b2;
	// abs value will be 1 + index of where differs
	// signal is negative if first differing byte is smaller
	// on
	while (i1 != e1 && i2 != e2)
	{
		s64 diff = (s64) *i1 - (s64) *i2;
		if (diff < 0) { return -(1 + (i1-b1)); }
		else if (diff > 0) { return (1 + (i1-b1)); }
		++i1; ++i2;
	}
	if (i1 != e1) { return (1 + (i1-b1)); }
	else if (i2 != e2) { return -(1 + (i1-b1)); }
	return 0;
}

static s64
cstr_compare_memory_n(char* b1, char* e1, char* b2, char* e2, s64 n)
{
	if (e1 - b1 > n)
		e1 = b1 + n;
	if (e2 - b2 > n)
		e2 = b2 + n;
	return cstr_compare_memory(b1, e1, b2, e2);
}

static inline s64
cstr_match_str(char *cstr, char *str, s64 str_length)
{
	Assert(cstr);
	s64 n = str_length;
	char *it_cstr = cstr;
	char *it_str  = str;
	for (;;) {
		if (*it_cstr == 0) return n==0;
		else if (n == 0) return 0;
		else if (*it_cstr != *it_str) return 0;
		++it_cstr;
		++it_str;
		--n;
	}
}

static inline s64
cstr_compare_memory_cstr(char* b1, char* e1, char* cstr)
{
	return cstr_compare_memory(b1, e1, cstr, cstr_end(cstr));
}

static inline s64
cstr_compare_memory_n_cstr(char* b1, char* e1, char* cstr, s64 n)
{
	// compare at most n characters
	return cstr_compare_memory_n(b1, e1, cstr, cstr_end(cstr), n);
}

static s64
cstr_find_first_match(MemoryBlock *begin, MemoryBlock *end, char *content_begin, char *content_end)
{
	Assert(begin < end);
	MemoryBlock *it = begin;
	while (it != end) {
		if (cstr_compare_memory(it->begin, it->end, content_begin, content_end) != 0) {
			++it;
		} else {
			return (it - begin);
		}
	}
	return -1;
}


////////////////////////////////////////////////////////////////////////////////
// convert_uri_to_ascii
////////////////////////////////////////////////////////////////////////////////


// TODO(llins): Denormalize HTTP request-target in a more general way (%00-%FF <-> ascii)
// GET /taxi.b("pickup_location",dive(10)); HTTP/1.1
// method SP request-tar get SP HTTP-version CRLF

//
static inline s32
cstr_hex_digit(char ch)
{
// 	ascii code	48	0	(number zero)
// 	ascii code	65	A	(Capital A)
// 	ascii code	97	a	(Lowercase a)
	if (ch < 48 || ch >= 97 + 6) {
		return -1;
	} else if (ch < 48 + 10) {
		return ch - 48;
	} else if (ch < 65) {
		return -1;
	} else if (ch < 65 + 6) {
		return 10 + (ch - 65);
	} else if (ch < 97) {
		return -1;
	} else { // if (ch < 97 + 6) {
		return 10 + (ch - 97);
	}
}


static MemoryBlock
cstr_convert_uri_to_ascii(MemoryBlock uri)
{
	char *dst = uri.begin;
	char *src = uri.begin;
	MemoryBlock result;
	result.begin = dst;

	// no %DD possible output is the same as input
	if (uri.end - src < 3) {
		return uri;
	}
	char *max_src = uri.end - 3;

loop:
	if (src[0] == '%') {
		s32 h = cstr_hex_digit(src[1]);
		if (h < 0)
			goto normal_advance;
		s32 l = cstr_hex_digit(src[2]);
		if (l < 0)
			goto normal_advance;
		*dst = (char) (h * 16 + l);
		++dst;
		src += 3;
	}

normal_advance:

	*dst = *src;
	++dst;
	++src;

	if (src <= max_src)
		goto loop;

	while (src != uri.end) {
		*dst = *src;
		++dst;
		++src;
	}

	result.end = dst;
	*result.end = 0;
	return result;
}


static MemoryBlock
cstr_convert_uri_to_ascii_old(MemoryBlock uri)
{
	char *dst = uri.begin;
	char *src = uri.begin;

	MemoryBlock result;
	result.begin = dst;
	while (src != uri.end) {
		if (*src == '%' && (uri.end - src > 2)
		    && (*(src+1) == '2' && *(src+2) == '2')) {
			*dst = '"';
			src += 3;
		} else {
			*dst = *src;
			++src;
		}
		++dst;
	}
	result.end = dst;
	return result;
}

static void
cstr_buffer_replace(char* text, s32 len, char old, char new)
{
	for (s32 i=0;i<len;++i) {
		if (text[i] == old) {
			text[i] = new;
		}
	}
}

static s32
cstr_buffer_squeeze(char* text, s32 len, char ch)
{
	if (len==0) return len;
	s32 i=0, it=0;
	while (it < len) {
		text[i] = text[it];
		++it;
		if (text[i] == ch) {
			while (it < len && text[it] == ch) {
				++it;
			}
		}
		++i;
	}
	return i;
}

// util_buffer_replace(text, len, ',', ' ');
// util_buffer_squeeze(text, len, ' ');

static char*
cstr_buffer_find_char(char symbol, char *begin, char *end, s32 discard_count, s32 reverse)
{
	if (begin == end)
		return 0;
	s32 inc=0;
	char *it=0, *it_end=0;
	if (reverse) {
		inc = -1;
		it = end - 1;
		it_end = begin-1;
	} else {
		inc = 1;
		it = begin;
		it_end = end;
	}
	while (it != it_end) {
		if (*it == symbol) {
			if (discard_count > 0) {
				--discard_count;
			} else {
				return it;
			}
		}
		it += inc;
	}
	return 0;
}

static s32
cstr_find_cstr_match(char *target, char* *sources, s32 sources_length)
{
	for (s32 i=0;i<sources_length;++i) {
		if (cstr_match(target, sources[i])) {
			return i;
		}
	}
	return -1;
}

static s32
cstr_char_in(char ch, char *list_begin, char *list_end)
{
	while (list_begin < list_end) {
		if (ch == list_begin[0]) {
			return 1;
		} else {
			++list_begin;
		}
	}
	return 0;
}

static MemoryBlock
cstr_ltrim(char *begin, char *end, char *blanks_begin, char *blanks_end)
{
	char *l = begin;
	char *r = end;
	while (l < r) {
		if (cstr_char_in(l[0], blanks_begin, blanks_end)) ++l;
		else break;
	}
	return (MemoryBlock) { .begin = l, .end = r };
}

static MemoryBlock
cstr_rtrim(char *begin, char *end, char *blanks_begin, char *blanks_end)
{
	char *l = begin;
	char *r = end;
	while (l < r) {
		if (cstr_char_in(r[-1], blanks_begin, blanks_end)) --r;
		else break;
	}
	return (MemoryBlock) { .begin = l, .end = r };
}

static MemoryBlock
cstr_trim(char *begin, char *end, char *blanks_begin, char *blanks_end)
{
	MemoryBlock ltrim = cstr_ltrim(begin ,end, blanks_begin, blanks_end);
	return cstr_rtrim(ltrim.begin, ltrim.end, blanks_begin, blanks_end);

}

