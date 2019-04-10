//------------------------------------------------------------------------------
// Print
//------------------------------------------------------------------------------

typedef struct {
	char *begin;
	char *end;
	char *capacity;
	union {
		struct {
			u64 written: 56;
			u64 overflow: 1;
			u64 user_flags: 7;
		};
		u64 flags;
	};
} Print;

_Static_assert((sizeof(Print) % 8) == 0, "Print_invalid_size");

static void
print_init(Print *ps, void *buffer, u64 buffer_size)
{
	ps->begin = ps->end = buffer;
	ps->capacity = (char*) buffer + buffer_size - 1; // leave space for null terminated string
	ps->flags = 0;
}

static u64
print_length(Print *self)
{
	Assert(self->begin <= self->end);
	return self->end - self->begin;
}

static u64
print_length0(Print *self)
{
	Assert(self->begin <= self->end);
	Assert(*self->end == 0);
	return self->end - self->begin + 1;
}

static void
print_clear(Print *ps)
{
	ps->end = ps->begin;
	if (ps->end != ps->capacity) {
		*ps->end = 0; // make sure we have a cstr
	}
	ps->flags = 0;
}

static char*
print_checkpoint(Print *self)
{
	return self->end;
}

static void
print_rewind(Print *self, char *checkpoint)
{
	Assert(self->begin <= checkpoint && checkpoint <= self->capacity);
	self->end = checkpoint;
}

static void
print_u64(Print *ps, u64 x)
{
	if (x == 0) {
		if (ps->end < ps->capacity) {
			*ps->end = '0';
			++ps->end;
			ps->written = 1;
			ps->overflow = 0;
		}
		else {
			ps->overflow = 1;
			ps->written = 0;
		}
	}
	else {
		char *it = ps->end;
		while (x > 0) {
			if (it == ps->capacity) {
				// overflow
				ps->overflow = 1;
				ps->written = 0;
				return;
			}
			u64 d = x % 10;
			x = x/10;
			*it = (char)('0' + d);
			++it;
		}
		// reverse
		int i = (int) (it - ps->end)/2 - 1;
		while (i >= 0) {
			char c = *(ps->end + i);
			*(ps->end + i) = *(it - 1 - i);
			*(it - 1 - i) = c;
			--i;
		}
		ps->written = it - ps->end;
		ps->overflow = 0;
		ps->end += ps->written;
	}
}


static void
print_s64(Print *ps, s64 x)
{
	if (x == 0) {
		if (ps->end < ps->capacity) {
			*ps->end = '0';
			++ps->end;
			ps->written = 1;
			ps->overflow = 0;
		} else {
			ps->overflow = 1;
			ps->written = 0;
		}
		return;
	} else {

		char *begin = ps->end;

		if (x < 0) {
			if (begin < ps->capacity) {
				*begin = '-';
				++begin;
				ps->written = 1;
				ps->overflow = 0;
				x = -x;
			} else {
				ps->overflow = 1;
				ps->written = 0;
				return;
			}
		}

		char *it = begin;
		while (x > 0) {
			if (it == ps->capacity) {
				// overflow
				ps->overflow = 1;
				ps->written = 0;
				return;
			}
			u64 d = x % 10;
			x = x/10;
			*it = (char)('0' + d);
			++it;
		}
		// reverse
		int i = (int) (it - begin)/2 - 1;
		while (i >= 0) {
			char c = *(begin + i);
			*(begin + i) = *(it - 1 - i);
			*(it - 1 - i) = c;
			--i;
		}
		ps->written = it - ps->end;
		ps->overflow = 0;
		ps->end += ps->written;
	}
}


static void
print_cstr_safe(Print *self, char *begin, char *end)
{
	char* it = self->end;
	char* src_it = begin;
	while (*src_it != 0 && src_it != end) {
		if (it == self->capacity) {
			// overflow
			self->overflow = 1;
			self->written = 0;
			return;
		}
		*it = *src_it;
		++it;
		++src_it;
	}
	self->written = it - self->end;
	self->overflow = 0;
	self->end += self->written;
}

static void
print_cstr(Print *ps, const char* s)
{
	char* it = ps->end;
	while (*s != 0) {
		if (it == ps->capacity) {
			// overflow
			ps->overflow = 1;
			ps->written = 0;
			return;
		}
		*it = *s;
		++it;
		++s;
	}
	ps->written = it - ps->end;
	ps->overflow = 0;
	ps->end += ps->written;
	*ps->end = 0;
}

static void
print_char(Print *self, char c)
{
	if (self->end != self->capacity) {
		*self->end = c;
		++self->end;
		*self->end = 0; // the slot of self->capacity
		self->written = 1;
		self->overflow = 0;
	} else {
		self->overflow = 1;
		self->written = 0;
	}
}

static void
print_nchar(Print *self, char c, u64 n)
{
	if (n <= (u64) (self->capacity - self->end)) {
		for (u64 i=0;i<n;++i) {
			*self->end = c;
			++self->end;
		}
		*self->end = 0; // the slot of self->capacity
		self->written = n;
		self->overflow = 0;
	} else {
		self->overflow = 1;
		self->written = 0;
	}
}

static void
print_str(Print *self, char* begin, char *end)
{
	u64 n = end - begin;
	if (n <= (u64)(self->capacity - self->end)) {

		// assumption that there is a platform constant defined
		platform.memory_copy(self->end, begin, n);

		self->end += n;
// 		for (u64 i=0;i<n;++i) {
// 			*self->end = *(begin + i);
// 			++self->end;
// 		}
		*self->end = 0; // the slot of self->capacity
		                // is reserved, so this is always
				// valid
		self->written = n;
		self->overflow = 0;
	}
	else {
		self->overflow = 1;
		self->written = 0;
	}
}

static void
print_buffer(Print *self, void *buffer, u64 length)
{
	if (length <= (u64)(self->capacity - self->end)) {
		// assumption that there is a platform constant defined
		platform.memory_copy(self->end, buffer, length);
		self->end += length;
		*self->end = 0; // the slot of self->capacity
		                // is reserved, so this is always
				// valid
		self->written = length;
		self->overflow = 0;
	} else {
		self->overflow = 1;
		self->written = 0;
	}
}

typedef union f64_bits
{
	u64 data;
	f64 value;
	struct {
		u64 mantissa: 52;
		u64 exponent: 11;
		u64 sign: 1;
	};
}
f64_bits;

static void
print_f64(Print *ps, f64 value)
{
	char buffer[32];
	sprintf(buffer, "%e", value);
	print_cstr(ps, buffer);
}

/*
 * Assume last print started at position "pos".
 * can be used to align a series of smaller
 * prints.
 */
static void
print_fake_last_print(Print *self, char *pos)
{
	Assert(self->begin <= pos && pos <= self->end && !self->overflow);
	self->written = pt_safe_s64_u64(self->end - pos);
}


#define Print_cstr_aligned_LEFT   -1
#define Print_cstr_aligned_CENTER  0
#define Print_cstr_aligned_RIGHT   1

// @todo check if it is compatible with the platform notion of print
static void
print_cstr_aligned(Print *self, char *st, s32 len, s32 alignment, char space_filler)
{
	char *begin = self->end;

	s64 count = 0;
	while (*st && self->end != self->capacity) {
		*self->end = *st;
		++st;
		++self->end;
		++count;
	}


	if (self->end == self->capacity) {
		return;
	}

	char *new_begin_if_right_aligned = self->end;

	for (s32 i=count;i<len && self->end != self->capacity;++i) {
		*self->end = space_filler;
		++self->end;
	}

	if (alignment == Print_cstr_aligned_RIGHT) {
		pt_rotate(begin, new_begin_if_right_aligned, self->end);
	} else if (alignment == Print_cstr_aligned_CENTER) {
		pt_rotate(begin, new_begin_if_right_aligned + (self->end - new_begin_if_right_aligned)/2, self->end);
	}

	// make sure we null terminate
	*self->end = 0;
}



static void
print_align(Print *ps, u64 len, s8 align, char space_filler)
{
	if (len <= ps->written)
		return;

	u64 extra = len - ps->written;
	if (extra <= (u64)(ps->capacity - ps->end)) {

		char* begin = ps->end - ps->written;

		u64 left = align < 0
			? 0
			: (align > 0
			   ? (len-extra)
			   : (len - (extra+1)/2));

		for (u64 i=0;i<extra;++i) {
			*ps->end = space_filler;
			++ps->end;
		}

		pt_rotate(begin, begin + left, ps->end);

		ps->written  = len;
		ps->overflow = 0;
	}
	else {
		ps->overflow = 1;
		ps->written = 0;
	}
}

static void
print_bin_u64(Print *self, u64 x)
{
	char *mark = self->end;

	// NOTE(llins): super inefficient
	b8 game_on = 0;
	for (s32 i=63;i>=0;--i) {
		b8 on = ((x & (1ULL << i)) != 0);
		if (on) {
			print_char(self, '1');
			game_on = 1;
		} else if (game_on) {
			print_char(self, '0');
		}
	}
	if (!game_on) {
		print_char(self, '0');
	}

	print_fake_last_print(self, mark);
}


// use the elegant stb library from Jeff Roberts and Sean Barret
// https://raw.githubusercontent.com/nothings/stb/master/stb_sprintf.h
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

static void
print_format(Print *self, char const * format, ...)
{
	va_list argp;
	va_start(argp, format);
	s32 result = stbsp_vsnprintf(self->end, self->capacity - self->end, format, argp);
	va_end(argp);
	self->end += result;
	self->written = result;
	self->overflow = 0;
	*self->end=0;
// 	char *it = self->end;
// 	/* expensive loop, if we could access the number of characters written */
// 	while (it != self->capacity && *it != 0)
// 		++it;
// 	self->end = it;
}

static s32
print_s32(Print *self, s32 n)
{
	s32 printed_characters = snprintf(self->end, self->capacity-self->end, "%d", n);
	self->end += printed_characters;
	return printed_characters;
}

static s32
print_f32(Print *self, f32 n)
{
	s32 printed_characters = snprintf(self->end, self->capacity-self->end, "%f", n);
	self->end += printed_characters;
	return printed_characters;
}

static s32
print_is_empty(Print *self)
{
	return self->begin == self->end;
}

static s32
print_fits(Print *self, u64 len)
{
	return len <= (self->capacity - self->end);
}

static s64
print_capacity(Print *self)
{
	return self->capacity - self->begin;
}

static s64
print_free_capacity(Print *self)
{
	return self->capacity - self->end;
}

static s32
print_lrtrim(Print *self, char *discard_chars, char *begin, char *end)
{
	while (begin != end) {
		char *discard_char = discard_chars;
		char *it = begin;
		while (*discard_char != 0) {
			if (*it == *discard_char) {
				++begin;
				break;
			} else {
				++discard_char;
			}
		}
		if (it == begin) {
			// nothing was trimmed on the left
			break;
		}
	}
	while (begin != end) {
		char *discard_char = discard_chars;
		char *it = end - 1;
		while (*discard_char != 0) {
			if (*it == *discard_char) {
				--begin;
				break;
			} else {
				++discard_char;
			}
		}
		if (it == end - 1) {
			// nothing was trimmed on the right
			break;
		}
	}
	print_str(self, begin, end);
	return self->written;
}

static void
print_pop(Print *self, s32 n)
{
	Assert(n >= 0);
	Assert(self->begin + n <= self->end);
	self->end -= n;
}

static s32
print_cstr_len(Print *self, char *cstr, s64 len)
{
	char *src = cstr;
	char *dst = self->end;
	while (*src != 0 && dst != self->capacity && len > 0) {
		*dst  = *src;
		++src;
		++dst;
		--len;
	}
	*dst = 0; // we can always write here since we reserved the last byte also
	self->end = dst;
	// return number of bytes written
	return src - cstr;
}

static s32
char_is_letter(char ch)
{
	return (((s32) ch >= (s32) 'A') && ((s32) ch <= (s32) 'Z')) || (((s32) ch >= (s32) 'a') && ((s32) ch <= (s32) 'z'));
}

static s32
char_is_digit(char ch)
{
	return ((s32) ch >= (s32) '0') && ((s32) ch <= (s32) '9');
}

static s32
char_is_uppercase_letter(char ch)
{
	return ((s32) ch >= (s32) 'A') && ((s32) ch <= (s32) 'Z');
}

static s32
char_is_space_or_tab(char ch)
{
	return (ch == ' ' || ch == '\t');
}

//
// given a string with tokens separated by space,
// everytime a new token starts, print its first
// character in case it is a letter and it is
// upper case
//
static s32
print_uppercase_initials(Print *self, char *cstr)
{
	s32 next_char_might_start_new_token = 1;

	char *src = cstr;
	char *dst = self->end;
	while (*src != 0 && dst != self->capacity) {
		if (char_is_space_or_tab(*src)) {
			next_char_might_start_new_token = 1;
		} else {
			if (next_char_might_start_new_token) {
				if (char_is_uppercase_letter(*src)) {
					*dst = *src;
					++dst;
				}
				next_char_might_start_new_token = 0;
			}
		}
		++src;
	}
	s32 result = (s32) (dst - self->end);
	self->end = dst;
	*dst = 0; // we can always write here since we reserved the last byte also

	// return number of bytes written
	return result;
}

#define print_STANDARD_SIZE Kilobytes(4)
#define print_MIN_SIZE sizeof(Print) + 1

static Print*
print_init_(void *raw, u64 size)
{
	Print *print = raw;
	print[0] = (Print) { 0 };
	print[0] = (Print) {
		.begin    = OffsetedPointer(raw, sizeof(Print)),
		.end      = OffsetedPointer(raw, sizeof(Print)),
		.capacity = OffsetedPointer(raw, size-1)
	};
	if (print->begin < print->end)
		print->begin[0] = 0;
	return print;
}

// size is the number of bytes for both the Print structure
// and its buffer and safety slot
static Print*
print_new_raw(u64 size)
{
	if (size == 0) {
		size = print_STANDARD_SIZE;
	}
	if (size < print_MIN_SIZE) {
		return 0;
	}
	void *raw = platform.allocate_memory_raw(size,0);
	if (!raw) {
		return 0;
	}
	return print_init_(raw, size);
}

static void
print_free_raw(Print *print)
{
	platform.free_memory_raw(print);
}

// assumption that input print was created with new_raw_print
static Print*
print_resize_raw(Print *print, u64 new_size)
{
	u64 n = print->end - print->begin;
	u64 min_raw_size = sizeof(Print) + n + 1;
	if (new_size < min_raw_size) {
		return 0;
	}
	Print *new_print = print_new_raw(new_size);
	platform.memory_copy(new_print->begin, print->begin, n);
	new_print->end = new_print->begin + n;
	*new_print->end = 0;
	platform.free_memory_raw(print);
	return new_print;
}

static Print*
print_new(a_Arena *arena, u64 size)
{
	if (size == 0) {
		size = print_STANDARD_SIZE;
	}
	if (size < print_MIN_SIZE) {
		return 0;
	}
	char *raw= a_push(arena, size, 8, 0);
	Assert(raw);
	return print_init_(raw, size);
}
