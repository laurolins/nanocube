//------------------------------------------------------------------------------
//
// tok_Tokens
//
//------------------------------------------------------------------------------

// @Todo include some notion of buffer size so that
// copy operations can be safe. Right now we assume
// the destination buffer can fit the active content
// in the source buffer
typedef struct {
	// the i-th token goes from position
	// tok_boundaries[i]+1 to tok_boundaries[i+1]
	char *buffer;
	s32  *tok_boundaries;
	s32  tok_count;
	s32  tok_capacity;
	union {
		char sep;
		u32 pad_;
	};
} tok_Tokens;

StaticAssertAlignment(tok_Tokens,8);

static void
tok_init(tok_Tokens *self, void *buffer, u64 size)
{
	Assert((u64) buffer % 8 == 0);
	self->tok_boundaries = buffer;
	self->tok_capacity = size / sizeof(s32);
	self->tok_count = 0;
	self->buffer = 0;
	self->sep='|';
}

static void
tok_prepare(tok_Tokens *self, char *c_str, char sep)
{
	Assert(sep != 0);
	self->buffer = c_str;
	self->sep = sep;
	self->tok_boundaries[0] = -1;
	s32  tok_boundaries_next_slot = 1;
	char *it = c_str;
	while (*it != 0) {
		if (*it == sep) {
			self->tok_boundaries[tok_boundaries_next_slot] = it - c_str;
			++tok_boundaries_next_slot;
			// we mess with the input c_str so that all token values
			// become null terminated
			*it = 0;
		}
		++it;
	}
	self->tok_boundaries[tok_boundaries_next_slot] = it - c_str;
	self->tok_count = tok_boundaries_next_slot;
}

static void
tok_pop(tok_Tokens *self)
{
	Assert(self->tok_count > 0);
	--self->tok_count;
}

static void
tok_restore_separators(tok_Tokens *self)
{
	s32 i=1;
	for (i=1;i<self->tok_count;++i) {
		self->buffer[self->tok_boundaries[i]] = self->sep;
	}
}

static void
tok_unrestore_separators(tok_Tokens *self)
{
	s32 i=1;
	for (i=1;i<self->tok_count;++i) {
		self->buffer[self->tok_boundaries[i]] = 0;
	}
}

static char*
tok_get(tok_Tokens *self, s32 index)
{
	Assert(index < self->tok_count);
	return self->buffer + (self->tok_boundaries[index] + 1);
}

static char*
tok_get_end(tok_Tokens *self, s32 index)
{
	Assert(index < self->tok_count);
	return self->buffer + self->tok_boundaries[index+1];
}

static MemoryBlock
tok_get_block(tok_Tokens *self, s32 index)
{
	Assert(index < self->tok_count);
	return (MemoryBlock) {
		.begin = self->buffer + (self->tok_boundaries[index] + 1),
		.end = self->buffer + self->tok_boundaries[index+1]
	};
}

// static char*
// tok_(tok_Tokens *self, s32 index)
// {
// 	Assert(index < self->tok_count);
// 	return self->buffer + (self->tok_boundaries[index] + 1);
// }


static void
tok_remove_last_token_if_blank(tok_Tokens *self)
{
	if (self->tok_count > 0) {
		if (*(tok_get(self, self->tok_count-1)) == 0) {
			tok_pop(self);
		}
	}
}

static s32
tok_is_sorted(tok_Tokens *self)
{
	for (s32 i=1;i<self->tok_count;++i) {
		if (cstr_compare(tok_get(self,i-1),tok_get(self,i)) >= 0) {
			return 0;
		}
	}
	return 1;
}


static s32
tok_active_buffer_size(tok_Tokens *self)
{
	if (self->tok_count > 0) {
		return (s32)((s64) (tok_get_end(self, self->tok_count-1) - tok_get(self,0) + 1));
	} else {
		return 1;
	}
}

static void
tok_copy_tokens(tok_Tokens *self, tok_Tokens *other)
{
	Assert(other->tok_count <= self->tok_capacity);
	Assert(self->buffer);
	self->tok_count = other->tok_count;
	// one extra boundary for normalization purposed (the -1)
	for (s32 i=0;i<other->tok_count+1;++i) {
		self->tok_boundaries[i] = other->tok_boundaries[i];
	}
	// copy the buffer with inclusive
	// notion of buffer end
	s32 buffer_size_to_copy = tok_active_buffer_size(other);
	for (s32 i=0;i<buffer_size_to_copy;++i) {
		self->buffer[i] = other->buffer[i];
	}
}

static s32
tok_binary_search(tok_Tokens *self, char *value)
{
	s32 l = 0;
	s32 r = self->tok_count;

	if (r == 0) {
		return -1;
	}

	while (r - l > 2) {
		s32 m = (l + r) / 2; // m is always different from l and r
		s32 sign = cstr_compare(value, tok_get(self, m));
		if (sign < 0) {
			r = m;
		} else if (sign > 0) {
			l = m+1;
		} else {
			return m;
		}
	}

	// do a run of comparisons
	s32 sign_l = cstr_compare(value, tok_get(self, l));
	if (sign_l < 0) {
		return -(l+1);
	} else if (sign_l == 0) {
		return l; // found it at position l
	} else if (l + 1 == r) {
		return -(l+2);
	} else {
		s32 sign_lp1 = cstr_compare(value, tok_get(self, l+1));
		if (sign_lp1 < 0) {
			return -(l+2);
		} else if (sign_lp1 == 0) {
			return l+1; // found it at position l
		} else {
			return -(l+3);
		}
	}
}

static tok_Tokens*
tok_new(Arena *arena, u64 max_items)
{
	u64 offset = sizeof(tok_Tokens);
	u64 size = 0;
	if (max_items == 0) {
		size = Kilobytes(4);
	} else {
		size = offset + (max_items + 1) * sizeof(s32);
	}
	void *buffer = Arena_push(arena, size, 8, 0);
	tok_Tokens *tokens = buffer;
	*tokens = (tok_Tokens) { 0 }; //clear
	tok_init(tokens, OffsetedPointer(buffer,offset), size - offset);
	return tokens;
}






