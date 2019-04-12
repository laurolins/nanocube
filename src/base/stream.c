//
// stream
//
// given some arbitrary callback that fills in a buffer, abastract it to
// a stream of "records" given record separator sequences
//

#define strm_PULL_CALLBACK(name) u32 name(void *user_data, void *buffer, u32 length)
typedef strm_PULL_CALLBACK(strm_PullCallback);

typedef struct {
	char              *buffer;
	u32               capacity; // bytes available on buffer
	u32               begin;    // current record begin offset
	u32               end;      // current last buffered location
	union {
		struct {
			char separator;
			b8 eof;
			b8 overflow;
			b8 discard_empty;
		};
		u32 padding;
	};
	strm_PullCallback *pull_callback;
	void              *pull_callback_data;
} strm_Stream;

#define strm_DEFAULT_SIZE Kilobytes(64) - sizeof(strm_Stream)

//
// easy way to initialize with a single char separator
//
static strm_Stream*
strm_new_stream(a_Arena *arena, u64 size, char sep, b8 discard_empty, strm_PullCallback* pull_callback, void *pull_callback_data)
{
	Assert(sizeof(strm_Stream) % 8 == 0);
	if (size == 0) {
		size = strm_DEFAULT_SIZE;
	}
	size += sizeof(strm_Stream);
	u32 offset = sizeof(strm_Stream);
	char *buffer = a_push(arena, size, 8, 0);
	strm_Stream *stream = (strm_Stream*) buffer;
	*stream = (strm_Stream) {
		.buffer = buffer + offset,
		.capacity = size - offset - 1, // leave space to write a null terminator
		.begin = 0,
		.end = 0,
		.pull_callback = pull_callback,
		.pull_callback_data = pull_callback_data,
		.padding = 0
	};
	stream->separator = sep;
	stream->discard_empty = discard_empty;
	return stream;
}

static void
strm_Stream_reset(strm_Stream *self, char sep, b8 discard_empty, strm_PullCallback* pull_callback, void *pull_callback_data)
{
	*self = (strm_Stream) {
		.buffer = self->buffer,
		.capacity = self->capacity, // leave space to write a null terminator REMOVE THIS CONSTRAINT
		.begin = 0,
		.end = 0,
		.pull_callback = pull_callback,
		.pull_callback_data = pull_callback_data,
		.padding = 0
	};
	self->separator = sep;
	self->discard_empty = discard_empty;
}

static void
strm_Stream_simple(strm_Stream *stream, char sep, b8 discard_empty, void *buffer, u64 length)
{
	*stream = (strm_Stream) {
		.buffer = buffer,
		.capacity = length, // leave space to write a null terminator
		.begin = 0,
		.end = length,
		.pull_callback = 0,
		.pull_callback_data = 0,
		.padding = 0
	};
	stream->separator = sep;
	stream->discard_empty = discard_empty;
	if (length == 0) {
		stream->eof = 1;
	}
}

static strm_Stream*
strm_new_simple_stream(a_Arena *arena, char sep, b8 discard_empty, void *buffer, u64 length)
{
	strm_Stream *stream = a_push(arena, sizeof(strm_Stream), 8, 1);
	strm_Stream_simple(stream, sep, discard_empty, buffer, length);
	return stream;
}

#define strm_OK              0
#define strm_OVERFLOW        1
#define strm_EOF             2

typedef struct {
	char *begin;
	u32  length;
	u32  status;
} strm_Record;

#define strm_OVERFLOW_RECORD (strm_Record) { .begin = 0, .length = 0, .status = strm_OVERFLOW }
#define strm_EOF_RECORD (strm_Record) { .begin = 0, .length = 0, .status = strm_EOF }

static strm_Record
strm_Stream_next(strm_Stream *self)
{
	if (self->eof) {
		return strm_EOF_RECORD;
	} else if (self->overflow) {
		return strm_OVERFLOW_RECORD;
	}
	u32 it = self->begin;
	for (;;) {
		if (it != self->end) {
			if (self->buffer[it] == self->separator) {
				// found new record from [begin,it)
				// make *it == 0 for string like stuff
				strm_Record result = (strm_Record) {
					.begin = self->buffer + self->begin,
					.length = it - self->begin,
					.status = strm_OK
				};
				// self->buffer[it] = 0;
				self->begin = it + 1;
				if (result.length > 0 || !self->discard_empty) {
					return result;
				} else {
					it = self->begin;
				}
			} else {
				++it;
			}
		} else {
			//
			// if buffer is full and there is no callback available
			//
			if (self->begin == 0 && self->end == self->capacity) {

				// check if next pull returns something, otherwise we found the
				// last token
				if (self->pull_callback) {
					char dummy=0;
					u32 bytes_retrieved = self->pull_callback(self->pull_callback_data, &dummy, 1);
					if (bytes_retrieved > 0) {
						self->overflow = 1; // nowhere to go. record overflow.
						// assumption that buffer would fit largest record was broken
						return strm_OVERFLOW_RECORD;
					}
				}

				// if we get here we found the last record
				self->eof = 1;
				strm_Record result = (strm_Record) {
					.begin = self->buffer + self->begin,
					.length = it - self->begin,
					.status = strm_OK
				};
				return result;

			}  else {
				//
				// shift only if there is the potential to bring in more data
				//
				u32 bytes_retrieved = 0;
				if (self->pull_callback) {
					if (self->begin > 0) {
						u32 offset = it - self->begin;
						u32 length = self->end - self->begin;
						platform.memory_move(self->buffer, self->buffer + self->begin, self->end - self->begin);
						self->begin = 0;
						self->end   = length;
						it = offset;
					}
					// try pulling more data from the back-end mechanism
					bytes_retrieved = self->pull_callback(self->pull_callback_data, self->buffer + self->end, self->capacity - self->end);
				}
				if (bytes_retrieved > 0) {
					self->end += bytes_retrieved;
					// self->buffer[self->end] = 0; // we can always do this
				} else {
					// EOF
					// self->buffer[it] = 0;
					self->eof = 1;
					strm_Record result = (strm_Record) {
						.begin = self->buffer + self->begin,
						.length = it - self->begin,
						.status = strm_OK
					};
					self->begin = it;
					if (result.length > 0) {
						return result;
					} else {
						return strm_EOF_RECORD;
					}
				}
			}
		}
	}
	InvalidCodePath;
}


static strm_Record
strm_Stream_unprocessed(strm_Stream *self)
{
	if (self->eof) {
		return strm_EOF_RECORD;
	} else if (self->overflow) {
		return strm_OVERFLOW_RECORD;
	} else {
		if (self->discard_empty) {
			// get rid empty spaces if discard empty is enabled
			while (self->begin != self->end && self->buffer[self->begin] == self->separator) {
				++self->begin;
			}
		}
		return (strm_Record) {
			.begin = self->buffer + self->begin,
			.length = self->end - self->begin,
			.status = strm_OK
		};
	}
}


