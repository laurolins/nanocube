

//
// Well-Known Text Representation of Geometry
//
// https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry
// # CCW exterior ring, CW holes
// POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))
// MULTIPOLYGON (((30 20, 45 40, 10 40, 30 20)), ((15 5, 40 10, 10 20, 5 10, 15 5)))
// MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))
//

//
// wkt_start
//

typedef struct {
	s32 offset;
	s32 length;
	s32 type;
} wkt_Token;

#define wkt_TOKEN_ERROR_INVALID_NUMBER -5

#define wkt_TOKEN_NEED_BUFFER_UPDATE 0
#define wkt_TOKEN_POLYGON 1
#define wkt_TOKEN_MULTIPOLYGON 2
#define wkt_TOKEN_OPEN 3
#define wkt_TOKEN_CLOSE 4
#define wkt_TOKEN_NUMBER 5
#define wkt_TOKEN_TUPLE_SEP 6
#define wkt_TOKEN_EOF 7

#define wkt_POLYGON "POLYGON"
#define wkt_POLYGON_LEN (sizeof(wkt_POLYGON)-1)
#define wkt_MULTIPOLYGON "MULTIPOLYGON"
#define wkt_MULTIPOLYGON_LEN (sizeof(wkt_MULTIPOLYGON)-1)

static char *wkt_STR_TOKEN_TYPE[] = {
"wkt_TOKEN_NEED_BUFFER_UPDATE",
"wkt_TOKEN_POLYGON",
"wkt_TOKEN_MULTIPOLYGON",
"wkt_TOKEN_OPEN",
"wkt_TOKEN_CLOSE",
"wkt_TOKEN_NUMBER",
"wkt_TOKEN_TUPLE_SEP",
"wkt_TOKEN_EOF"
};

static char *wkt_STR_TOKEN_TYPE_ERROR[] = {
"wkt_TOKEN_NEED_BUFFER_UPDATE",
"wkt_TOKEN_ERROR_POLYGON",
"wkt_TOKEN_ERROR_MULTIPOLYGON",
"wkt_TOKEN_ERROR_OPEN",
"wkt_TOKEN_ERROR_CLOSE",
"wkt_TOKEN_ERROR_NUMBER",
"wkt_TOKEN_ERROR_TUPLE_SEP",
"wkt_TOKEN_ERROR_EOF"
};

//
// what if we hava a huge file?
//

//
// - assume streaming case
// - a small window into the data stored at the buffer
// - someone else fills in the buffer (exit parsing requiring more info)
//
// [ parser ] [ label ][ label ][ left ] ----> <----- [ right ] [ buffer ]
//
typedef struct {
	s32 it;            // where the next unprocessed token starts
	s32 end;           // where no more data is available

	s32 buffer_length; // buffer to keep unprocessed data
	s32 left;          // make left be the buffer size

	s32 right;         // space to keep current object (needs to be less than 2GB)
	s32 size;          // size in bytes of this Parser (including text buffer and data buffer)

	s32 num_numbers;   // num numbers
	s32 num_rings;     // num rings

	s32 num_labels;    // labels come before polygon
	s32 labels_length;

	s32 ring_index;
	s32 polygon_index;

	s32 level;
	s16 state;
	s16 eof;
} wkt_Parser;

typedef struct {
	char *buffer;
	s32 length;
} wkt_Buffer;

#define wkt_ITEM_RING 1
#define wkt_ITEM_POLYGON 2

typedef struct {
	s32 polygon;
	s32 ring;
	s32 offset;
	s32 length; // number of numbers
} wkt_Ring;

typedef struct {
	s32  length;
	char text[];
} wkt_Label;


// StaticAssertAlignment(wkt_Parser, 8);

//
// multiples of 4KB and at least 4KB bigger for size than buffer_length
//
static wkt_Parser*
wkt_parser_new(s32 size, s32 buffer_length)
{
	size = RAlign(size, 4096);
	buffer_length = RAlign(buffer_length, 4096);
	if (size == buffer_length) {
		size += 4096;
	}
	wkt_Parser *parser = valloc(size);
	parser[0] = (wkt_Parser) {
		.it = 0,
		.end = 0,
		.buffer_length = buffer_length,
		.left = sizeof(wkt_Parser),
		.right = size - buffer_length,
		.size = size,
		.eof = 0
	};
	return parser;
}

static void
wkt_parser_delete(wkt_Parser *self)
{
	free(self);
}

static char*
wkt_parser_buffer_(wkt_Parser *self)
{
	return OffsetedPointer(self, self->size - self->buffer_length);
}

// it comes in the reverse order
typedef struct {
	wkt_Ring *begin;
	wkt_Ring *end;
	wkt_Ring *it;
} wkt_RingIter;

static wkt_Ring*
wkt_ring_iterator_next(wkt_RingIter *self)
{
	if (self->it == self->begin) return 0;
	--self->it;
	return self->it;
}

static void
wkt_ring_iterator_reset(wkt_RingIter *self)
{
	self->it = self->end;
}

static wkt_RingIter
wkt_rings(wkt_Parser *self)
{
	wkt_Ring *end   = OffsetedPointer(self, self->size - self->buffer_length);
	wkt_Ring *begin = end - self->num_rings;
	return (wkt_RingIter) {
		.begin = begin,
		.end = end,
		.it = end, // it moves backwards
	};
}

//
// get access to buffer space
//
static wkt_Buffer
wkt_parser_get_update_buffer(wkt_Parser *self)
{
	char *buffer = wkt_parser_buffer_(self);
	return  (wkt_Buffer) { .buffer = buffer + self->end, .length = self->buffer_length - self->end };
}

//
// more "bytes" bytes were written into the buffer
// and "eof" was reached
//
static void
wkt_parser_signal_buffer_update(wkt_Parser *self, s32 bytes, s32 eof)
{
	Assert(!self->eof);
	Assert(bytes + self->end <= self->buffer_length);
	self->end += bytes;
	self->eof = eof;
}

static wkt_Token
wkt_next_token_(wkt_Parser *self)
{
	char *buffer = wkt_parser_buffer_(self);
	wkt_Token result = { 0 }; // == { .offset=0, .length=0, .type = wkt_TOKEN_NEED_BUFFER_UPDATE };
	s32 it = self->it;
	if (it < self->end) {
		while (it != self->end) {
			if (buffer[it] == '(') {
				result = (wkt_Token) { .offset = it, .length = 1, .type = wkt_TOKEN_OPEN };
				++it;
				break;
			} else if (buffer[it] == ')') {
				result = (wkt_Token) { .offset = it, .length = 1, .type = wkt_TOKEN_CLOSE};
				++it;
				break;
			} else if (buffer[it] == ',') {
				result = (wkt_Token) { .offset = it, .length = 1, .type = wkt_TOKEN_TUPLE_SEP };
				++it;
				break;
			} else if (buffer[it] == ' ' || buffer[it] == '\n' || buffer[it] == '\t' || buffer[it] == '\r') {
				++it;
			} else if (buffer[it] == 'P') {
				// see if we can match buffer as POLYGON
				if (wkt_POLYGON_LEN <= self->end - it) {
					if (strncmp(buffer + it, wkt_POLYGON, wkt_POLYGON_LEN) == 0) {
						result = (wkt_Token) { .offset = it, .length = wkt_POLYGON_LEN, .type = wkt_TOKEN_POLYGON };
						it += wkt_POLYGON_LEN;
					} else {
						fprintf(stderr, "[next_token_error] found P and was expecting POLYGON\n");
						result = (wkt_Token) { .offset = it, .length = wkt_POLYGON_LEN, .type = -wkt_TOKEN_POLYGON };
					}
					break;
				} else {
					// need more to try to match POLYGON
					break;
				}
			} else if (buffer[it] == 'M') {
				// see if we can match buffer as POLYGON
				if (wkt_MULTIPOLYGON_LEN <= self->end - it) {
					if (strncmp(buffer + it, wkt_MULTIPOLYGON, wkt_MULTIPOLYGON_LEN) == 0) {
						result = (wkt_Token) { .offset = it, .length = wkt_MULTIPOLYGON_LEN, .type = wkt_TOKEN_MULTIPOLYGON };
						it += wkt_MULTIPOLYGON_LEN;
					} else {
						fprintf(stderr, "[next_token_error] found P and was expecting POLYGON\n");
						result = (wkt_Token) { .offset = it, .length = wkt_MULTIPOLYGON_LEN, .type = -wkt_TOKEN_MULTIPOLYGON };
					}
					break;
				} else {
					// need more to try to match POLYGON
					break;
				}
			} else if ((buffer[it] >= '0' && buffer[it] <= '9') || buffer[it] >= '-' || buffer[it] >= '+') {
				s32 it_number_start = it;
				s32 integral_digits = (buffer[it] >= '0' && buffer[it] <= '9') ? 1 : 0;
				s32 exponent_digits = -1;
				s32 fraction_digits = -1;
				++it;
				// parse integral part
				while (it != self->end && buffer[it] >= '0' && buffer[it] <= '9') {
					++integral_digits;
					++it;
				}

				if (it != self->end && buffer[it] == '.') {
					++it;
					++fraction_digits;
					while (it != self->end && buffer[it] >= '0' && buffer[it] <= '9') {
						++fraction_digits;
						++it;
					}
				}

				if (it != self->end && (buffer[it] == 'e' || buffer[it] == 'E')) {
					++it;
					++exponent_digits;
					if (it != self->end && (buffer[it] == '+' || buffer[it] == '-')) {
						++it;
						while (it != self->end && buffer[it] >= '0' && buffer[it] <= '9') {
							++exponent_digits;
							++it;
						}
					} else {
						while (it != self->end && buffer[it] >= '0' && buffer[it] <= '9') {
							++exponent_digits;
							++it;
						}
					}
				}

				// number should be triggered
				s32 number_should_be_complete = (it < self->end || self->eof);

				if (number_should_be_complete) {
					// found separator, a number should be in
					if (integral_digits == 0 && fraction_digits == 0) {
						fprintf(stderr, "[next_token_error] Number triggered without integral or fractional digits\n");
						result = (wkt_Token) { .offset = it_number_start, .length = it-it_number_start, .type = -wkt_TOKEN_NUMBER };
					} else if (exponent_digits == 0) {
						fprintf(stderr, "[next_token_error] Exponent triggered without digits\n");
						result = (wkt_Token) { .offset = it_number_start, .length = it-it_number_start, .type = -wkt_TOKEN_NUMBER };
					} else {
						result = (wkt_Token) { .offset = it_number_start, .length = it-it_number_start, .type = wkt_TOKEN_NUMBER };
					}
					// else we should be good to go with a valid number token
				} else if (!self->eof) {
					// maybe more symbols will come for the current number
				} else {
					fprintf(stderr, "[next_token_error] Invalid symbol at %d\n", it);
					result.type = -wkt_TOKEN_NUMBER;
				}
				break;
			}
		}

		if (result.type > 0) {
			self->it = it;
		}
		return result;
	} else if (it == self->end) {
		if (self->eof) {
			return (wkt_Token) { .offset=0, .length=0, .type = wkt_TOKEN_EOF };
		} else {
			return (wkt_Token) { 0 }; // wkt_TOKEN_NEED_BUFFER_UPDATE
		}
	}
	return result;
}


//
// 0            2 3 4  5  6 4  5  6 4  5  6 4  5  6 7 8 2
// ^            ^ ^ ^  ^  ^ ^  ^  ^ ^  ^  ^ ^  ^  ^ ^ ^ ^
//  POLYGON      ( ( 35 10 , 45 45 , 15 40 , 10 20 , 35 10), (20 30, 35 35, 30 20, 20 30))
//
// 0            1 2 3 4  5  6 4  5  6 4  5  6 4  5  6 7 8 2                                   0
// ^            ^ ^ ^ ^  ^  ^ ^  ^  ^ ^  ^  ^ ^  ^  ^ ^ ^ ^ ...                            ...^
//  MULTIPOLYGON ( ( ( 30 20 , 45 40 , 10 40 , 30 20 ) ) , ((15 5, 40 10, 10 20, 5 10, 15 5)))
//  MULTIPOLYGON ( ( ( 40 40 , 20 45 , 45 30 , 40 40 ) ) , ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))
//

#define wkt_RESULT_MORE_DATA_NEEDED            0
#define wkt_RESULT_POLYGON_READY               1
#define wkt_RESULT_MULTIPOLYGON_READY          2
#define wkt_RESULT_MORE_MEMORY_NEEDED          3
#define wkt_RESULT_STR_TO_F64_ERROR            4
#define wkt_RESULT_PARSING_ERROR               5
#define wkt_RESULT_LABEL_READY                 6

static const char *wkt_STR_RESULT[] = {
	"wkt_RESULT_MORE_DATA_NEEDED",
	"wkt_RESULT_POLYGON_READY",
	"wkt_RESULT_MULTIPOLYGON_READY",
	"wkt_RESULT_MORE_MEMORY_NEEDED",
	"wkt_RESULT_STR_TO_F64_ERROR",
	"wkt_RESULT_PARSING_ERROR",
	"wkt_RESULT_LABEL_READY"
};

#define wkt_STATE_IDLE                        0
#define wkt_STATE_EXPECT_MULTIPOLYGON         1
#define wkt_STATE_EXPECT_POLYGON              2
#define wkt_STATE_EXPECT_RING                 3
#define wkt_STATE_EXPECT_POINT_X	      4
#define wkt_STATE_EXPECT_POINT_Y              5
#define wkt_STATE_EXPECT_NEXT_POINT_OR_END    6
#define wkt_STATE_EXPECT_NEXT_RING_OR_END     7
#define wkt_STATE_EXPECT_NEXT_POLYGON_OR_END  8

// a label is done when 'TAB' is found
// discard left spaces, new lines, etc
#define wkt_STATE_READING_LABEL               9

static const char *wkt_STR_STATE[] = {
"wkt_STATE_IDLE",
"wkt_STATE_EXPECT_MULTIPOLYGON",
"wkt_STATE_EXPECT_POLYGON",
"wkt_STATE_EXPECT_RING",
"wkt_STATE_EXPECT_POINT_X",
"wkt_STATE_EXPECT_POINT_Y",
"wkt_STATE_EXPECT_NEXT_POINT_OR_END",
"wkt_STATE_EXPECT_NEXT_RING_OR_END",
"wkt_STATE_EXPECT_NEXT_POLYGON_OR_END",
"wkt_STATE_READING_LABEL"
};

// either separator or a new ring becomes the active one


// macro to make the default error cases in wkt_next(...) below  more compact
#define wkt_err_(msg) default: { fprintf(stderr, msg); return wkt_RESULT_PARSING_ERROR; } break;

// static wkt_Ring*
// wkt_parser_push_ring_(wkt_Parser *self)
// {
// }

static f64*
wkt_push_uninitilized_number_(wkt_Parser *self)
{
	s32 new_left = self->left + sizeof(f64);
	if (new_left <= self->right) {
		f64 *result = OffsetedPointer(self, self->left);
		self->left = new_left;
		++self->num_numbers;
		return result;
	} else {
		return 0;
	}
}

static s32
wkt_label_storage_size_(s32 length)
{
	return sizeof(wkt_Label) + RAlign(length+1,8);
}

static wkt_Label*
wkt_push_label_(wkt_Parser *self, s32 begin, s32 end)
{
	s32 length = end - begin;
	s32 storage_length = wkt_label_storage_size_(length);
	s32 new_left = self->left + storage_length;
	if (new_left <= self->right) {
		char *buffer = wkt_parser_buffer_(self);
		wkt_Label *result = OffsetedPointer(self, self->left);
		result[0] = (wkt_Label) {
			.length = length
		};
		for (s32 i=0;i<length;i++) {
			result->text[i] = buffer[begin+i];
		}
		result->text[length] = 0;
		self->left = new_left;
		++self->num_labels;
		self->labels_length += storage_length;
		return result;
	} else {
		return 0;
	}
}

static void
wkt_start_polygon_(wkt_Parser *self)
{
	self->ring_index = 0;
}

static wkt_Ring*
wkt_start_ring_(wkt_Parser *self)
{
	if (self->left + sizeof(wkt_Ring) <= self->right) {
		self->right -= sizeof(wkt_Ring);
		wkt_Ring *result = OffsetedPointer(self, self->right);
		++self->num_rings;
		result[0] = (wkt_Ring) {
			.polygon = self->polygon_index,
			.ring = self->ring_index,
			.offset = self->num_numbers,
			.length = 0
		};
		return result;
	} else {
		// can't fit a new ring
		return 0;
	}
}

static void
wkt_end_ring_(wkt_Parser *self)
{
	Assert(self->num_rings > 0);
	wkt_Ring *ring = OffsetedPointer(self, self->right);
	ring->length = self->num_numbers - ring->offset;
	Assert(ring->length % 2 == 0);
}

static void
wkt_end_polygon_(wkt_Parser *self)
{
	++self->polygon_index;
	self->ring_index = 0;
}

static wkt_Label*
wkt_first_label(wkt_Parser *self)
{
	if (self->num_labels == 0) return 0;
	return (wkt_Label*) OffsetedPointer(self, sizeof(wkt_Parser));
}

static void
wkt_clear_data(wkt_Parser *self)
{
	self->left = sizeof(wkt_Parser);
	self->right = self->size - self->buffer_length;
	self->num_labels = 0;
	self->num_rings = 0;
	self->num_numbers = 0;
	self->labels_length = 0;
	self->ring_index = 0;
	self->polygon_index = 0;
	self->level = 0;
	self->state = 0; // IDLE
}

static void
wkt_move_unprocessed_buffer_to_the_left_(wkt_Parser *self)
{
	if (self->it < self->end) {
		char *buffer = wkt_parser_buffer_(self);
		char *dst = buffer;
		char *src = buffer + self->it;
		s32 n = self->end - self->it;
		for (s32 i=0;i<n;++i) {
			*dst = *src;
			++dst;
			++src;
		}
		self->it = 0;
		self->end = n;
	} else {
		self->it = 0;
		self->end = 0;
	}
}


static s32
wkt_next_wkt(wkt_Parser *self)
{
//	if (self->state
	for (;;) {
		wkt_Token token = wkt_next_token_(self);

		if (token.type == wkt_TOKEN_NEED_BUFFER_UPDATE) {
			// rotate buffer and return wkt_RESULT_MORE_DATE_NEEDED
			wkt_move_unprocessed_buffer_to_the_left_(self);
			return wkt_RESULT_MORE_DATA_NEEDED;
		}

		// fprintf(stderr,"state: %s token: %s\n", wkt_STR_STATE[self->state], wkt_STR_TOKEN_TYPE[token.type]);

		switch(self->state) {
		case wkt_STATE_IDLE: {
			switch(token.type) {
			case wkt_TOKEN_MULTIPOLYGON: {
				self->state = wkt_STATE_EXPECT_MULTIPOLYGON;
			} break;
			case wkt_TOKEN_POLYGON: {
				self->state = wkt_STATE_EXPECT_POLYGON;
			} break;
			wkt_err_("Expecting either POLYGON or MULTIPOLYGON\n");
			}
		} break;
		case wkt_STATE_EXPECT_MULTIPOLYGON: {
			switch(token.type) {
			case wkt_TOKEN_OPEN: {
				self->state = wkt_STATE_EXPECT_POLYGON;
				++self->level;
			} break;
			wkt_err_("Expecting '(' to open up rings of a MULTIPOLYGON\n");
			}
		} break;
		case wkt_STATE_EXPECT_POLYGON: {
			switch(token.type) {
			case wkt_TOKEN_OPEN: {
				wkt_start_polygon_(self);
				self->state = wkt_STATE_EXPECT_RING;
				++self->level;
			} break;
			wkt_err_("Expecting '(' to open up rings of a POLYGON\n");
			}
		} break;
		case wkt_STATE_EXPECT_RING: {
			switch(token.type) {
			case wkt_TOKEN_OPEN: {
				wkt_Ring *ring = wkt_start_ring_(self);
				if (!ring) {
					self->it = token.offset;
					return wkt_RESULT_MORE_MEMORY_NEEDED;
				}
				self->state = wkt_STATE_EXPECT_POINT_X;
				++self->level;
			} break;
			wkt_err_("Expecting '(' to open up a POLYGON ring\n");
			}
		} break;
		case wkt_STATE_EXPECT_POINT_X: {
			switch(token.type) {
			case wkt_TOKEN_NUMBER: {
				f64 *number_slot = wkt_push_uninitilized_number_(self);
				if (number_slot == 0) {
					self->it = token.offset;
					return wkt_RESULT_MORE_MEMORY_NEEDED;
				}
				char *buffer = wkt_parser_buffer_(self);
				s32 ok = pt_str_to_f64(buffer + token.offset, token.length, number_slot);
				if (ok) {
					self->state = wkt_STATE_EXPECT_POINT_Y;
				} else {
					return wkt_RESULT_STR_TO_F64_ERROR;
				}
			} break;
			wkt_err_("Expecting NUMBER as a X coordinate\n");
			}
		} break;
		case wkt_STATE_EXPECT_POINT_Y: {
			switch(token.type) {
			case wkt_TOKEN_NUMBER: {
				f64 *number_slot = wkt_push_uninitilized_number_(self);
				if (number_slot == 0) {
					self->it = token.offset;
					return wkt_RESULT_MORE_MEMORY_NEEDED;
				}
				char *buffer = wkt_parser_buffer_(self);
				s32 ok = pt_str_to_f64(buffer + token.offset, token.length, number_slot);
				if (ok) {
					self->state = wkt_STATE_EXPECT_NEXT_POINT_OR_END;
				} else {
					return wkt_RESULT_STR_TO_F64_ERROR;
				}
			} break;
			wkt_err_("Expecting NUMBER as a Y coordinate\n");
			}
		} break;
		case wkt_STATE_EXPECT_NEXT_POINT_OR_END: {
			switch(token.type) {
			case wkt_TOKEN_TUPLE_SEP: {
				self->state = wkt_STATE_EXPECT_POINT_X;
			} break;
			case wkt_TOKEN_CLOSE: {
				// start ring...
				wkt_end_ring_(self);
				self->state = wkt_STATE_EXPECT_NEXT_RING_OR_END;
				--self->level;
			} break;
			wkt_err_("wkt_STATE_EXPECT_NEXT_POINT_OR_END: Expecting either ',' or ')' to close a ring\n");
			}
		} break;
		case wkt_STATE_EXPECT_NEXT_RING_OR_END: {
			switch(token.type) {
			case wkt_TOKEN_TUPLE_SEP: {
				self->state = wkt_STATE_EXPECT_RING;
			} break;
			case wkt_TOKEN_CLOSE: {
				--self->level;
				wkt_end_polygon_(self);
				if (self->level == 0) {
					self->state = wkt_STATE_IDLE;
					return wkt_RESULT_POLYGON_READY;
				} else if (self->level == 1) {
					self->state = wkt_STATE_EXPECT_NEXT_POLYGON_OR_END;
				}
			} break;
			wkt_err_("wkt_STATE_EXPECT_NEXT_RING_OR_END: Expecting either ',' or ')' to close a ring\n");
			}
		}; break;
		case wkt_STATE_EXPECT_NEXT_POLYGON_OR_END: {
			switch(token.type) {
			case wkt_TOKEN_TUPLE_SEP: {
				self->state = wkt_STATE_EXPECT_POLYGON;
			} break;
			case wkt_TOKEN_CLOSE: {
				--self->level;
				self->state = wkt_STATE_IDLE;
				return wkt_RESULT_MULTIPOLYGON_READY;
			} break;
			wkt_err_("wkt_STATE_EXPECT_NEXT_POLYGON_OR_END: Expecting either ',' or ')' to close a ring\n");
			}
		}; break;
		};
	}
}

static f64*
wkt_numbers_(wkt_Parser *self)
{
	if (self->num_numbers == 0) return 0;
	return OffsetedPointer(self, sizeof(wkt_Parser) + self->labels_length);
}

static s32
wkt_next_label(wkt_Parser *self)
{
	Assert(self->state == wkt_STATE_IDLE);
	Assert(self->num_rings == 0);
	Assert(self->num_numbers == 0);

	char *text = wkt_parser_buffer_(self);
	s32 it = self->it;
	s32 begin = -1;
	while (it != self->end) {
		if (text[it] == '\t') {
			if (begin < 0) {
				begin = it;
			}
			wkt_Label *label = wkt_push_label_(self, begin, it);
			if (!label) {
				// rotate buffer and return wkt_RESULT_MORE_DATE_NEEDED
				return wkt_RESULT_MORE_MEMORY_NEEDED;
			} else {
				self->it = it+1;
				return wkt_RESULT_LABEL_READY;
			}
		} else if (begin < 0 && (text[it] != ' ' && text[it] != '\n' && text[it] != '\r')) {
			begin = it;
		}
		++it;
	}

	// if we end up with the buffer without finding the end trigger of a label
	// report that more data is needed
	if (self->eof) {
		if (begin < 0) {
			begin = it;
		}
		wkt_Label *label = wkt_push_label_(self, begin, it);
		if (!label) {
			// rotate buffer and return wkt_RESULT_MORE_DATE_NEEDED
			return wkt_RESULT_MORE_MEMORY_NEEDED;
		} else {
			self->it = it;
			return wkt_RESULT_LABEL_READY;
		}
	} else {
		wkt_move_unprocessed_buffer_to_the_left_(self);
		return wkt_RESULT_MORE_DATA_NEEDED;
	}
}



