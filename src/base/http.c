/* http functionality */

//
// state machine that parses incoming HTTP messages
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


// Assumption here is that each channel will have a buffer
// large enough fit a whole line (either start-line or header-field)
typedef enum {
	http_PARSING_REQUEST_LINE,
	http_PARSING_HEADER_FIELD,
	http_PARSING_MESSAGE_BODY
} http_ParseState;

typedef struct {
	http_ResponseStatus status;
	void *user_data;
} http_Response;

typedef struct http_Channel http_Channel;

//
// this callback marks the beginning of a new request
//
#define http_REQUEST_LINE_CALLBACK(name) \
	void name(http_Channel *channel, \
		  http_Response *response, \
		  char *method_begin, char *method_end, \
		  char *request_target_begin, char *request_target_end, \
		  char *http_version_begin, char *http_version_end)
typedef http_REQUEST_LINE_CALLBACK(http_RequestLineCallback);

#define http_HEADER_FIELD_CALLBACK(name) \
	void name(http_Channel *channel, \
		  http_Response *response, \
		  char *field_name_begin, char *field_name_end, \
		  char *field_value_begin, char *field_value_end)
typedef http_HEADER_FIELD_CALLBACK(http_HeaderFieldCallback);

// #define http_SEND_CALLBACK(name) void name(http_Channel *http_channel, char *buffer, u64 length)
// typedef http_SEND_CALLBACK(http_SendCallback);

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
	struct {
		http_RequestLineCallback *request_line;
		http_HeaderFieldCallback *header_field;
		// http_SendCallback *send;
	} callback;
	struct {
		http_ParseState state;
		b8   partial_line;
		b8   buffer_overflow;
		b8   invalid_message;
		s32  buffer_overflow_count;
		u64  message_body_length;
	} parsing;
	/* data associated with http channel */
	void *user_data;
};


internal void
http_Channel_init(http_Channel *self, char *buffer_begin, char *buffer_end,
		  http_RequestLineCallback *request_line_callback,
		  http_HeaderFieldCallback *header_field_callback,
		  void *user_data)
{
	Assert(buffer_begin <= buffer_end);
	/* buffer */
	self->buffer.begin    = buffer_begin;
	self->buffer.end      = buffer_begin;
	self->buffer.capacity = buffer_end;
	/* response */
	self->response.sentinel = 0;
	self->response.end      = 1;
	/* callback */
	self->callback.request_line = request_line_callback;
	self->callback.header_field = header_field_callback;
	/* parsing */
	self->parsing.state = http_PARSING_REQUEST_LINE;
	self->parsing.partial_line = 0;
	self->parsing.buffer_overflow = 0;
	self->parsing.invalid_message = 0;
	self->parsing.buffer_overflow_count = 0;
	self->parsing.message_body_length = 0;
	/* user data associated to channel */
	self->user_data = user_data;
}

// internal http_Response*
// http_Channel_query(http_Channel *self, char *buffer, u64 length)
// {
// 	// prepend header write buffer in wide format (with %081)
// 	// send message
//
// 	/* atomic increment response end */
// 	u32 index    = pt_atomic_add_u32(&self->response.end,1) & http_RESPONSE_BUFFER_MASK;
//
// 	// unsafe sanity check
// 	// assumes no overflow will happen
// 	Assert(index != (self->response.end & http_RESPONSE_BUFFER_MASK));
//
// 	http_Response *response = self->response.buffer + i;
//
// 	response->status = http_RESPONSE_STATUS_REQUEST_IN_PROGRESS;
// 	response->user_data = 0;
//
// 	//
// 	// will translate characters on the buffer to
// 	// %000 notation (precise bytes)
// 	//
// 	// GET translated_buffer_message HTTP/1.1\r\n\r\n
// 	self->callback.send(self, buffer, length);
// }

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
}

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

internal inline b8
http_is_field_name_separator(char ch)
{
	return (ch <= 31 || ch == 127 || ch == ':' || ch == '\t' || ch == ' ');
}

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


internal void
http_Channel_process_end_of_header(http_Channel *self)
{
	// call back indicating end of message
	/* reset */
	self->parsing.state = http_PARSING_REQUEST_LINE;
	self->parsing.partial_line = 0;
	self->parsing.buffer_overflow = 0;
	self->parsing.invalid_message = 0;
	self->parsing.buffer_overflow_count = 0;
	self->parsing.message_body_length = 0;
}

/* Assumes a single thread at a time is calling this function on the same channel*/
/* This is true within the tcp socket management we have on the linux platform layer */
internal void
http_Channel_receive_data(http_Channel *self, char *buffer, u64 length)
{
	if (self->parsing.state == http_PARSING_MESSAGE_BODY) {

	} else {
		u64 i=0;
		u64 n = length;
		char *begin = buffer;
		while (1) {
			b8 eol = 0;
			// find end of line or end of buffer
			while (i<n) {
				if (buffer[i] != '\n') {
					++i;
				} else {
					eol=1;
					break;
				}
			}
			if (!eol) {
				http_Channel_append(self, begin, buffer + i);
				self->parsing.partial_line = 1;
				goto done;
			} else {
				char *line_begin = begin;
				char *line_end   = buffer + i;
				if (self->parsing.partial_line) {
					/* copy input buffer to channel buffer */
					http_Channel_append(self, begin, buffer + i);
					line_begin = self->buffer.begin;
					line_end   = self->buffer.end;
				}

				if (line_begin < line_end) {
					if (*(line_end-1) == '\r') {
						--line_end;
					}
				}

				// there is a whole line ready
				// from line_begin to line_end
				// (might have a \r at the end)
				if (line_begin == line_end) {
					http_Channel_process_end_of_header(self);
				} else if (self->parsing.state == http_PARSING_REQUEST_LINE) {
					http_Channel_process_request_line(self, line_begin, line_end);
				} else if (self->parsing.state == http_PARSING_HEADER_FIELD) {
					http_Channel_process_header_field_line(self, line_begin, line_end);
				} else {
					Assert(0 && "ooops");
				}
				++i;
				begin = buffer + i;

				if (self->parsing.partial_line) {
					self->buffer.end = self->buffer.begin;
					self->parsing.partial_line = 0;
					self->parsing.buffer_overflow = 0;
				}
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


