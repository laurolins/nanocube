/* csv stream */

//
// http://www.ietf.org/rfc/rfc4180.txt/ state machine that parses
//
// file = [header CRLF] record *(CRLF record) [CRLF]
//
// header = name *(COMMA name)
//
// record = field *(COMMA field)
//
// name = field
//
// field = (escaped / non-escaped)
//
// escaped = DQUOTE *(TEXTDATA / COMMA / CR / LF / 2DQUOTE) DQUOTE
//
// non-escaped = *TEXTDATA
//
// COMMA = %x2C
//
// CR = %x0D ;as per section 6.1 of RFC 2234 [2]
//
// DQUOTE =  %x22 ;as per section 6.1 of RFC 2234 [2]
//
// LF = %x0A ;as per section 6.1 of RFC 2234 [2]
//
// CRLF = CR LF ;as per section 6.1 of RFC 2234 [2]
//
// TEXTDATA =  %x20-21 / %x23-2B / %x2D-7E
//

//
// return null if no more data is or will be available
// otherwise return a pointer in [buffer, buffer + length]
// indicating how much data was pulled
//
#define csv_PULL_CALLBACK(name) char* name(void *user_data, char *buffer, u64 length)
typedef csv_PULL_CALLBACK(csv_PullCallback);

#define csv_OK 0
#define csv_ERROR_BUFFER_OVERFLOW 1
#define csv_ERROR_COLUMNS_OVERFLOW 2
#define csv_ERROR_ESCAPED_EOF 3

typedef struct {
} csv_Record;

typedef struct {
	struct {
		char *begin;
		char *record;
		char *cursor;
		char *end;
		char *capacity;
	} buffer;
	struct {
		u32  *begin;
		u32  *end;
		u32  *capacity;
	} separators;
	csv_PullCallback *pull_callback;
	void             *user_data;
	s32              error;
	char             sep;
	// if no more data is availabe through the pull callback
	b8               eof;
} csv_Stream;

internal void
csv_Stream_init(csv_Stream *self, char sep, char *buffer, u64 length, u32 max_separators, csv_PullCallback *pull_callback, void *user_data)
{
	u64 separators_buffer_length = sizeof(u32) * max_separators;
	Assert(separators_buffer_length < length);
	self->sep = sep;
	Assert(sep != '\n' && sep != '"');
	self->separators.begin    = (u32*) buffer;
	self->separators.end      = self->separators.begin;
	self->separators.capacity = self->separators.begin + max_separators;
	self->buffer.begin    = (char*) self->separators.capacity;
	self->buffer.record   = self->buffer.begin;
	self->buffer.cursor   = self->buffer.begin;
	self->buffer.end      = self->buffer.begin;
	self->buffer.capacity = buffer + length;
	self->eof = 0;
	self->error = csv_OK;
	self->pull_callback = pull_callback;
	self->user_data = user_data;
}

internal b8
csv_Stream_pull_more_data(csv_Stream *self)
{
	if (self->eof || self->error != csv_OK)
		return 0;
	// try pulling more data
	self->buffer.cursor = self->buffer.end;
	for (;;) {
		char *end = self->buffer.end;
		if (end < self->buffer.capacity) {
			char *new_end = self->pull_callback(self->user_data, self->buffer.end, self->buffer.capacity - self->buffer.end);
			if (new_end) {
				self->buffer.end = new_end;
				return 1;
			} else {
				self->eof = 1;
				return 0;
			}
		} else if (self->buffer.record > self->buffer.begin) {
			// move [buffer.current,buffer.end) to [buffer.begin,...)
			s64 offset = self->buffer.record - self->buffer.begin;
			char *it = self->buffer.record;
			while (it != self->buffer.end) {
				*(it - offset) = *it;
				++it;
			}
			self->buffer.end    = it - offset;
			self->buffer.record = self->buffer.begin;
			self->buffer.cursor -= offset;
		} else {
			self->error = csv_ERROR_BUFFER_OVERFLOW;
			return 0;
		}
	}
}

internal b8
csv_Stream_push_separator(csv_Stream *self, u32 offset)
{
	if (self->separators.end < self->separators.capacity) {
		*self->separators.end = offset;
		++self->separators.end;
		return 1;
	} else {
		return 0;
	}
}

internal b8
csv_Stream_get_fields(csv_Stream *self, MemoryBlock *buffer, u32 offset, u32 num_fields)
{
	u32 n = 1 + self->separators.end - self->separators.begin;
	if (offset + num_fields > n) {
		return 0;
	}
	for (u32 i = offset; i<offset+num_fields; ++i) {
		if (i > 0 && i < n - 1) {
			/* intermediate column */
			buffer[i-offset].begin = self->buffer.record + self->separators.begin[i-1] + 1;
			buffer[i-offset].end   = self->buffer.record + self->separators.begin[i];
		} else {
			/* begin */
			if (i > 0) {
				buffer[i-offset].begin = self->buffer.record + self->separators.begin[i-1]+1;
			} else {
				buffer[i-offset].begin = self->buffer.record;
			}
			/* end */
			if (i == n-1) {
				char *it = self->buffer.cursor;
				if (it > self->buffer.record) {
					if (*(it-1) == '\n') {
						--it;
						if (it > self->buffer.record && *(it-1) == '\r') {
							--it;
						}
					}
				}
				buffer[i-offset].end = it;
			} else {
				buffer[i-offset].end = self->buffer.record + self->separators.begin[i];
			}
		}
	}
	return 1;
}

internal u32
csv_Stream_num_fields(csv_Stream *self)
{
	return self->separators.end - self->separators.begin + 1;
}

internal b8
csv_Stream_next(csv_Stream *self)
{
	if (self->eof || self->error != csv_OK) {
		return 0;
	}

	/* advance it */
	b8 escaped = 0;
	b8 more_data_is_available      = 0;
	self->buffer.record = self->buffer.cursor;
	/* reset separators */
	self->separators.end = self->separators.begin;
	char *it = self->buffer.record;

	// search for end of record
	//
	// 2017-06-27T00:15
	// @Todo more robust new line processing: \r\n
	//
search_for_eor:
	while (it != self->buffer.end) {
		if (!escaped) {
			if (*it == self->sep) {
				b8 ok = csv_Stream_push_separator(self, it - self->buffer.record);
				if (!ok) {
					self->error  = csv_ERROR_COLUMNS_OVERFLOW;
					self->buffer.cursor = it;
					goto data_is_not_available;
				}
				++it;
			} else if (*it == '"') {
				escaped = 1;
				++it;
			} else if (*it != '\n') {
				++it;
			} else {
				// found new line (set cursor for the next record)
				self->buffer.cursor = it + 1;
				goto data_is_available;
			}
		} else {
			if (*it == '"') {
				// we need a look ahead here
				if (it + 1 == self->buffer.end) {
					more_data_is_available = csv_Stream_pull_more_data(self);
					if (!more_data_is_available) {
						Assert(self->eof && "Assumes csv_Stream_pull_more_data triggers eof when unsuccessful");
						/* reached end of file, columns are the ones we have */
						escaped = 0;
						goto data_is_available;
					} else {
						/* it is the previous */
						it = self->buffer.cursor - 1;
					}
				}

				if (*(it + 1) == '"') {
					// 2dquote
					it = it + 2;
				} else {
					++it;
					escaped = 0;
				}
			} else {
				++it;
			}
		}
	}
	/* needs more data */
	if (it == self->buffer.end) {
		more_data_is_available = csv_Stream_pull_more_data(self);
		if (more_data_is_available) {
			/* point it to new data */
			it = self->buffer.cursor;
			goto search_for_eor;
		} else {
			Assert(self->eof);
			if (escaped) {
				self->error = csv_ERROR_ESCAPED_EOF;
				goto data_is_not_available;
			} else {
				if (self->buffer.record < it) {
					goto data_is_available;
				} else {
					goto data_is_not_available;
				}
			}
		}
	} else {
		Assert(0 && "invalid execution path");
	}
data_is_available:
	return 1;
data_is_not_available:
	return 0;
}




