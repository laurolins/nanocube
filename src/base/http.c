//
// Minimal http functionality for parsing simple request/response
// messages from low level raw data
//

//
// @todo add minimal support to parse an HTTP response
// # response example
// HTTP/1.1 200 OK
// Access-Control-Allow-Origin: *
// Access-Control-Allow-Methods: GET
// Content-Type: application/json
// Content-Length: 162                           <----- this is the critical line we want to support
//
// two line feeds and then read all the content-length bytes
//


//
// GET /taxi HTTP/1.1
// User-Agent: wget
// timestamp: ......
// mime-type: ??????
//

#define http_RESPONSE_BUFFER_LENGTH 4096
#define http_RESPONSE_BUFFER_MASK   0xfff

typedef enum {
	http_RESPONSE_STATUS_OFF,
	http_RESPONSE_STATUS_REQUEST_IN_PROGRESS,
	http_RESPONSE_STATUS_REQUEST_ERROR,
	http_RESPONSE_STATUS_REQUEST_DONE
} http_ResponseStatus;

//
// HTTP-message  =  start-line
//                  *(header-field CRLF)
//                  CRLF
//                  [message-body]
//
// start-line    = request-line / status-line
//
// request-line  = method SP request-target SP HTTP-Version CRLF
//

//
// Assumption here is that each channel will have a buffer
// large enough fit a whole line (either start-line or header-field)
//
typedef enum {
	http_PARSE_STATE_IDLE,
	http_PARSE_STATE_FIELD,
	http_PARSE_STATE_DATA
} http_ParseState;

typedef struct {
	http_ResponseStatus status;
	void *user_data;
} http_Response;

typedef struct http_Channel http_Channel;

// the concept of an http_MessagePiece here
#define http_PIECE_REQUEST            1
#define http_PIECE_RESPONSE           2
#define http_PIECE_FIELD              3
#define http_PIECE_DATA               4

typedef struct {
	char *begin;
	char *end;
} http_String;

typedef struct {
	s32 type;
	union {
		struct {
			http_String method;
			http_String target;
			http_String http_version;
		} request;
		struct {
			http_String http_version;
			http_String status;
			http_String comment;
		} response;
		struct {
			http_String key;
			http_String value;
		} field;
		struct {
			http_String raw;
			s32         done; // is this the last data piece of the message? (based on Content-Length)
		} data;
	};
} http_Piece;

// http_Response *response
#define http_CALLBACK(name) void name(http_Channel *channel, http_Piece piece)
typedef http_CALLBACK(http_Callback);

//
// Channel to receive http requests.
//
struct http_Channel {
	struct {
		http_Response buffer[http_RESPONSE_BUFFER_LENGTH];
		// 'sentinel' chases 'end' in a cyclic fashion
		// when 'end' (mod size) == 'sentinel' (mod size)
		// buffer is full. Assuming no buffer overflow
		// will happen.
		u32 sentinel;
		u32 end;
	} response;
	struct {
		char *begin;
		char *end;
		char *capacity;
	} buffer;
	http_Callback *callback;
	struct {
		http_ParseState state;
		b8   partial_line;
		b8   buffer_overflow;
		b8   invalid_message;
		s32  buffer_overflow_count;
		s64  content_length;
		s64  content_to_push;
	} parsing;
	/* data associated with http channel */
	union {
		void *user_data;
		u64   user_index;
	};
};


internal void
http_Channel_init(http_Channel *self, void *buffer, u64 buffer_size, http_Callback *callback, void *user_data)
{
	Assert(buffer_size > 0);
	/* buffer */
	self->buffer.begin    = buffer;
	self->buffer.end      = buffer;
	self->buffer.capacity = buffer + buffer_size - 1;
	/* response */
	self->response.sentinel = 0;
	self->response.end      = 1;
	/* callback */
	self->callback = callback;
	/* parsing */
	self->parsing.state = http_PARSE_STATE_IDLE;
	self->parsing.partial_line = 0;
	self->parsing.buffer_overflow = 0;
	self->parsing.invalid_message = 0;
	self->parsing.buffer_overflow_count = 0;
	self->parsing.content_length = 0;
	self->parsing.content_to_push = 0;
	/* user data associated to channel */
	self->user_data = user_data;
}

internal http_Response*
http_Channel_current_response(http_Channel *self)
{
	u32 index     = (self->response.sentinel + 1) & http_RESPONSE_BUFFER_MASK;
	u32 end_index = (self->response.end + 1) & http_RESPONSE_BUFFER_MASK;
	if (index == end_index) {
		return 0;
	}
	return self->response.buffer + index;
}

internal inline void
http_Channel_append(http_Channel *self, char *begin, char *end)
{
	s64 length = end - begin;
	s64 available = self->buffer.capacity - self->buffer.end;
	if (length > available) {
		self->parsing.buffer_overflow = 1;
		length = available;
	}
	pt_copy_bytesn(begin, self->buffer.end, length);
	self->buffer.end += length;
	*self->buffer.end = 0; // make sure it is a valid c_string
}

#if 0

internal void
http_Channel_process_request_line(http_Channel *self, char *begin, char *end)
{
	if (self->parsing.buffer_overflow) {
		//
		//    whole message will be invalid
		//    keep receiving header-fields without triggering callbacks
		//    try to detect if the upcoming header-fields indicate a message body (or chunks)
		//    get message body, but don't care
		//    trigger end of HTTP-message callback with a fatal invalid request line
		//    go back to http_PARSING_REQUEST_LINE
		//
		/* this http message will be signaled as invalid */
		self->parsing.invalid_message = 1;
		++self->parsing.buffer_overflow_count;
	} else if (self->callback.request_line) {
		// a message deserving of a response

		/* atomic increment response end */
		u32 index = self->response.end & http_RESPONSE_BUFFER_MASK;
		++self->response.end;
		// unsafe sanity check
		// assumes no overflow will happen
		Assert(index != (self->response.sentinel & http_RESPONSE_BUFFER_MASK));
		http_Response *response = self->response.buffer + index;
		response->status = http_RESPONSE_STATUS_REQUEST_IN_PROGRESS;
		response->user_data = 0;

		// find method, request-target, http-version fields
		char *it = begin;
		while (it != end && *it != ' ')
			++it;
		char *request_method_end= it;
		if (it != end)
			++it;
		while (it != end && *it != ' ')
			++it;
		char *request_target_end = it;

		char *request_method_begin = begin;
		char *request_target_begin = (request_method_end < end) ? (request_method_end + 1) : end;
		char *http_version_begin = (request_target_end < end) ? (request_target_end + 1) : end;
		self->callback.request_line(self,
					    response,
					    request_method_begin, request_method_end,
					    request_target_begin, request_target_end,
					    http_version_begin, end);
	}
	/* go to next state */
	self->parsing.state = http_PARSING_HEADER_FIELD;
}

#endif

internal inline b8
http_is_field_name_separator(char ch)
{
	return (ch <= 31 || ch == 127 || ch == ':' || ch == '\t' || ch == ' ');
}

#if 0

internal void
http_Channel_process_header_field_line(http_Channel *self, char *begin, char *end)
{
	if (self->parsing.buffer_overflow) {
		++self->parsing.buffer_overflow_count;
	} else {

		//
		// will field-name will be everything that comes
		// before a ctl or a SP HT or ':'
		//

		// find method, request-target, http-version fields
		char *it = begin;
		while (it != end && !http_is_field_name_separator(*it))
			++it;
		char *field_name_end = it;

		char *field_name_begin = begin;
		char *field_value_begin = (field_name_end < end) ? (field_name_end + 1) : end;

		//
		// TODO(llins)
		// try to detect if it is a Content-Length kind of message
		//


		if (self->callback.header_field) {
			http_Response *response = http_Channel_current_response(self);
			self->callback.header_field(self,
						    response,
						    field_name_begin, field_name_end,
						    field_value_begin, end);
		}
	}
	self->parsing.state = http_PARSING_HEADER_FIELD;
}
#endif

internal void
http_Channel_goto(http_Channel *self, s32 state)
{
	switch(state) {
	case http_PARSE_STATE_IDLE: {
		self->parsing.state = http_PARSE_STATE_IDLE;
		self->parsing.partial_line = 0;
		self->parsing.buffer_overflow = 0;
		self->parsing.invalid_message = 0;
		self->parsing.buffer_overflow_count = 0;
		// clear content length
		self->parsing.content_length = 0;
		self->parsing.content_to_push= 0;
		self->buffer.end = self->buffer.begin;
	} break;
	case http_PARSE_STATE_FIELD: {
		self->parsing.state = http_PARSE_STATE_FIELD;
		self->parsing.partial_line = 0;
		self->parsing.buffer_overflow = 0;
		self->parsing.invalid_message = 0;
		self->parsing.buffer_overflow_count = 0;
		self->buffer.end = self->buffer.begin;
	} break;
	case http_PARSE_STATE_DATA: {
		self->parsing.state = http_PARSE_STATE_DATA;
		self->parsing.partial_line = 0;
		self->parsing.buffer_overflow = 0;
		self->parsing.invalid_message = 0;
		self->parsing.buffer_overflow_count = 0;
		self->buffer.end = self->buffer.begin;
	} break;
	}
}

internal s32
http_cstr_is_prefix(char *cstr, char *begin, char *end)
{
	char *it = begin;
	while (it != end && *cstr != 0) {
		if (*it != *cstr)
			return 0;
		++it;
		++cstr;
	}
	if (*cstr != 0) {
		return 0;
	}
	return 1;
}

internal char*
http_find_next(char *begin, char *end, char ch)
{
	char *it = begin;
	while (it != end) {
		if (*it == ch) {
			return it;
		} else {
			++it;
		}
	}
	return end;
}

internal s32
http_Channel_process_request_or_response_line(http_Channel *self, char *begin, char *end)
{
	char *tok1 = http_find_next(begin, end, ' ');
	if (tok1 == end) { return 0; } // malformed request/response line

	char *tok2 = http_find_next(tok1+1, end, ' ');
	if (tok2 == end) { return 0; } // malformed request/response line

	// fprintf(stderr, "[request_line]:\n%.*s\n", (s32) (end - begin), begin);
	// Assert(tok1 <= tok2);

	char *tok3 = tok2+1;

	//
	// expecte at least 3 tokens on a req/res line
	//
	// HTTP/1.1 200 OK
	// GET /taxi HTTP/1.1
	//

	http_Piece piece = { 0 };
	if (http_cstr_is_prefix("HTTP/", begin, tok1)) {
		// response message
		piece.type = http_PIECE_RESPONSE;
		piece.response.http_version = (http_String) { .begin = begin,  .end = tok1 };
		piece.response.status       = (http_String) { .begin = tok1+1, .end = tok2 };
		piece.response.comment      = (http_String) { .begin = tok2+1, .end = end  };
	} else {
		// request message
		piece.type = http_PIECE_REQUEST;
		piece.request.method        = (http_String) { .begin = begin,  .end = tok1 };
		piece.request.target        = (http_String) { .begin = tok1+1, .end = tok2 };
		piece.request.http_version  = (http_String) { .begin = tok2+1, .end = end  };
	}
	if (self->callback) {
		self->callback(self, piece);
	}
	return 1;
}

internal s32
http_Channel_process_field(http_Channel *self, char *begin, char *end)
{
	// find method, request-target, http-version fields
	char *it = begin;
	while (it != end && !http_is_field_name_separator(*it)) {
		++it;
	}
	char *field_name_end = it;
	while (it != end && http_is_field_name_separator(*it)) {
		++it;
	}
	char *field_value_begin = it;

	http_Piece piece = { 0 };
	piece.type = http_PIECE_FIELD;
	piece.field.key   = (http_String) { .begin = begin, .end = field_name_end };
	piece.field.value = (http_String) { .begin = field_value_begin, .end = end};
	if (self->callback) {
		self->callback(self, piece);
	}

	if (http_cstr_is_prefix("Content-Length", piece.field.key.begin, piece.field.key.end)) {
		// try to parse u64 number
		u64 content_length = 0;
		if (pt_parse_u64(piece.field.value.begin, piece.field.value.end, &content_length)) {
			self->parsing.content_length = content_length;
			self->parsing.content_to_push = content_length;
		} else {
			// log a warning that couldn't parse the content length
		}
	}

	return 1;
}




//
// Assumes a single thread at a time is calling this function on the same channel
// This is true within the tcp socket management we have on the linux platform layer
//
internal void
http_Channel_receive_data(http_Channel *self, char *buffer, u64 length)
{
	// while there is still unprocessed data in the incoming buffer
	// keep advancing the http state machine

	fprintf(stderr, "http receive data: %d starting with %.16s (partial line: %d)\n", (s32) length, buffer,self->parsing.partial_line);

	// cursor
	u64 i=0;
	char *begin = buffer;
	for (;;) {

		if (self->parsing.state == http_PARSE_STATE_IDLE || self->parsing.state == http_PARSE_STATE_FIELD) {

			b8 eol = 0;

			// find end of line or end of buffer
			while (i<length) {
				if (buffer[i] != '\n') {
					++i;
				} else {
					eol=1;
					break;
				}
			}

			if (!eol) {

				// if we haven't reached the end of line, just copy
				// bytes to local buffer and tag the parsing state as
				// partial
				http_Channel_append(self, begin, buffer + i);
				self->parsing.partial_line = 1;
				goto done;

			} else {

				// prepare line_begin and line_end considering
				// the partial case as well
				char *line_begin = begin;
				char *line_end   = buffer + i;
				if (self->parsing.partial_line) {
					/* copy input buffer to channel buffer */
					http_Channel_append(self, begin, buffer + i);
					line_begin = self->buffer.begin;
					line_end   = self->buffer.end;
				}

				// avoid the carriage return as the last character
				if (line_begin < line_end) {
					if (*(line_end-1) == '\r') {
						--line_end;
					}
				}

				if (line_begin == line_end) {
					if (self->parsing.content_length > 0) {
						http_Channel_goto(self, http_PARSE_STATE_DATA);
					} else {
						http_Channel_goto(self, http_PARSE_STATE_IDLE);
					}
				} else if (self->parsing.state == http_PARSE_STATE_IDLE) {

					fprintf(stderr, "identified next request/response line: %d ...starting with...\n%.16s\n", (s32) (line_end - line_begin), line_begin);

					// the callback mechanism should copy the data
					// if it will need later, otherwise it will be lost
					s32 ok = http_Channel_process_request_or_response_line(self, line_begin, line_end);
					if (ok) {
						http_Channel_goto(self, http_PARSE_STATE_FIELD);
					} else {
						http_Channel_goto(self, http_PARSE_STATE_IDLE);
					}
				} else if (self->parsing.state == http_PARSE_STATE_FIELD) {
					s32 ok = http_Channel_process_field(self, line_begin, line_end);
					// regardless of ok or no ok go to field again
					http_Channel_goto(self, http_PARSE_STATE_FIELD);
				}

				++i; // if we get here, position i has a new line
				begin = buffer + i;
			}
		} else if (self->parsing.state == http_PARSE_STATE_DATA) {
			// Check if new batch of data completes the current msg.
			// If so send the appropriate callback with done=1 flag,
			// enter the IDLE stata and process the remaining bytes.
			s64 bytes_available = length - i;
			if (bytes_available < self->parsing.content_to_push) {
				// push all the available bytes
				http_Piece piece = { 0 };
				piece.type = http_PIECE_DATA;
				piece.data.raw = (http_String) { .begin = begin, .end = begin + bytes_available };
				piece.data.done = 0;
				if (self->callback) {
					self->callback(self, piece);
				}
				self->parsing.content_to_push -= bytes_available;
				goto done;
			} else {
				// push all the available bytes
				http_Piece piece = { 0 };
				piece.type = http_PIECE_DATA;
				piece.data.raw = (http_String) { .begin = begin, .end = begin + self->parsing.content_to_push };
				piece.data.done = 1;
				if (self->callback) {
					self->callback(self, piece);
				}
				i += self->parsing.content_to_push;
				self->parsing.content_to_push = 0;
				http_Channel_goto(self, http_PARSE_STATE_IDLE);
				begin = buffer + i;
			}
		}
	}
done:
	return;
}



inline internal s32
http_hex_digit(char ch)
{
	// 0x30 - 0x39 ('0'-'9')
	// 0x41 - 0x46 ('A'-'F')
	// 0x61 - 0x66 ('a'-'f')
	if (ch < 0x30) {
		return -1;
	} else if (ch <= 0x39 ) {
		return (s32) ch - '0';
	} else if (ch < 0x41 ) {
		return -1;
	} else if (ch <= 0x46 ) {
		return 10 + (s32) ch - 'A';
	} else if (ch < 0x61 ) {
		return -1;
	} else if (ch <= 0x66 ) {
		return 10 + (s32) ch - 'a';
	} else {
		return -1;
	}
}

internal char
http_decode_pct_encoded(char *it, char *end)
{
	Assert(*it == '%');
	if ((it + 3) <= end) {
		s32 hex1 = http_hex_digit(it[1]);
		s32 hex0 = http_hex_digit(it[2]);
		if (hex1 >= 0 && hex0 >= 0) {
			return (char) ((hex1 << 4) + hex0);
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

//
// decoding inplace the characters between begin and end
//
internal char*
http_URI_decode_percent_encoded_symbols(char *begin, char *end, s32 *status)
{
	/* it and dst are in sync */
	char *dst = 0;
	char *it  = begin;
	char ch = 0;
	while (it != end) {
		if (*it != '%') {
			++it;
		} else {
			ch = http_decode_pct_encoded(it, end);
			if (ch >= 0) {
				*it = ch;
				dst = it + 1;
				it  += 3;
				goto outofsync;
			} else {
				goto error;
			}
		}
	}
	dst = it;
	goto ok;
outofsync:
	while (it != end) {
		if (*it != '%') {
			*dst++ = *it++;
		} else {
			ch = http_decode_pct_encoded(it, end);
			if (ch >= 0) {
				*dst++ = ch;
				it += 3;
			} else {
				goto error;
			}
		}
	}
ok:
	if (status) {
		*status = 0;
	}
	return dst;
error:
	if (status) {
		*status = (s32) (-1 - (it - begin));
	}
	return 0;
}


