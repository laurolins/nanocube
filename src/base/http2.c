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

#ifdef http2_UNIT_TEST
#include "platform.c"
#include "cstr.c"
#include "bst.c"
#include "arena.c"
#endif

//
// negative versions of the state indicates
// about to read the corresponding information
//
#define http2_STATE_REQUEST_METHOD      0
#define http2_STATE_REQUEST_TARGET      1
#define http2_STATE_REQUEST_VERSION     2
#define http2_STATE_REQUEST_FIELD_KEY   3
#define http2_STATE_REQUEST_FIELD_VALUE 4

#define http2_STATE_RESPONSE_VERSION       100
#define http2_STATE_RESPONSE_STATUS_CODE   101
#define http2_STATE_RESPONSE_REASON_PHRASE 102
#define http2_STATE_RESPONSE_FIELD_KEY     103
#define http2_STATE_RESPONSE_FIELD_VALUE   104
#define http2_STATE_RESPONSE_DATA          105

#ifndef http2_DEBUG_LEVEL
#define http2_DEBUG_LEVEL 0
#endif

#if http2_DEBUG_LEVEL > 0
#define http2_log(format, ...) fprintf(stderr, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define http2_log(format, ...)
#endif

#if http2_DEBUG_LEVEL > 1
#define http2_log2(format, ...) fprintf(stderr, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define http2_log2(format, ...)
#endif

#if http2_DEBUG_LEVEL > 2
#define http2_log3(format, ...) fprintf(stderr, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define http2_log3(format, ...)
#endif

typedef struct http2_Channel http2_Channel;

#define http2_CALLBACK(name) void name(http2_Channel *channel)
typedef http2_CALLBACK(http2_Callback);


//
// [ http2_Channel ] [ (k,v) (k,v) ... ] [partial_text_not processed_from_previous] --->  <--- [ strings ]
//

struct http2_Channel {

	s32 id;
	s32 left;
	s32 right;
	s32 size;

	s32 state; // in which state are we in
	s32 partial_text_length;

	struct {
		s32 method;
		s32 target;
		s32 version;
		s32 num_fields;
	} request;

	struct {
		s32  version;
	        s32  status_code;
		s32  reason_phrase;
		s32  num_fields;
		u64  block_offset; // current block offset
		u64  block_length;
		u64  length; // total length of the response announced in the header
		void *data;
	} response;

	void *source; // use to couple the socket that is feeding the data to this channel
	void *user_data;

	http2_Callback *callback;
	volatile u64 flags;
};

typedef struct {
	s32 key;
	s32 value;
} http2_Field;

typedef struct {
	s32 type;
	s32 offset;
	s32 length;
	s32 pad_;
} http2_Token;

typedef struct {
	s32  length;
	char text[];
} http2_String;

StaticAssertAlignment(http2_Token,8);
StaticAssertAlignment(http2_Field,8);
StaticAssertAlignment(http2_Channel,8);

static http2_Channel*
http2_new(void *buffer, s32 length, s32 id, http2_Callback *callback, void *user_data)
{
	Assert((u64) buffer % 8 == 0);
	Assert((u64) length % 8 == 0);
	Assert(length >= Kilobytes(4));

	http2_Channel *channel = buffer;
	channel[0] = (http2_Channel) { 0 };
	channel[0] = (http2_Channel) {
		.id = id,
		.left = sizeof(http2_Channel),
		.right = length,
		.size = length,
		.user_data = user_data,
		.callback = callback,
		.flags = 0
	};
	return channel;
}

//
// think of the prefix in left to left + partial_text_length
// concatenated with the buffer
//

typedef struct {
	char *prefix;
	s32   prefix_length;
	char *suffix;
	s32   suffix_length;
} http2_Text;

static s32
http2_text_at_(http2_Text *self, s32 index)
{
	if (index < self->prefix_length) return self->prefix[index];
	index -= self->prefix_length;
	if (index < self->suffix_length) return self->suffix[index];
	InvalidCodePath;
	return 0;
}

static s32
http2_text_length_(http2_Text *self)
{
	return self->prefix_length + self->suffix_length;
}

static s32
http2_text_suffix_offset_(http2_Text *self, s32 index)
{
	return index - self->prefix_length;
}

#define http2_TOKEN_INVALID -1
#define http2_TOKEN_UNDEFINED 0
#define http2_TOKEN_OK 1
#define http2_TOKEN_EOB 2

//
// parse text until new line or
//
// assumes: end_marker is disjoint of the forbidden_markers
//
static http2_Token
http2_next_token_(http2_Channel *self, char *end_marker, s32 end_marker_len, char *forbidden_char, s32 forbidden_marker_len, void *buffer, s32 length, s32 *advance)
{

	http2_Token tok = { .offset = 0, .length = 0 };

	Assert(end_marker_len > 0);
	http2_Text text = {
		.prefix = OffsetedPointer(self, self->left),
		.prefix_length = self->partial_text_length,
		.suffix = buffer,
		.suffix_length = length
	};

	s32 end = http2_text_length_(&text);
	advance[0] = 0;

	s32 end_marker_offset = 0;

	s32 i = 0;
	while (i < end) {
		char c  = http2_text_at_(&text,i);
		if (i > 0) {
			if (c == end_marker[end_marker_offset]) {
				++end_marker_offset;
				if (end_marker_offset == end_marker_len) {
					tok.length = i + 1 - end_marker_offset;
					tok.type = http2_TOKEN_OK;
					advance[0] = http2_text_suffix_offset_(&text,i+1);
					return tok;
				}
			} else if (end_marker_offset > 0) {
				tok.length = i + 1 - end_marker_offset;
				tok.type  = http2_TOKEN_INVALID;
				advance[0] = http2_text_suffix_offset_(&text,i+1);
				return tok;
			} else {
				// is c forbidden
				for (s32 j=0;j<forbidden_marker_len;++j) {
					if (c == forbidden_char[j]) {
						tok.length = i;
						tok.type = http2_TOKEN_INVALID;
						advance[0] = http2_text_suffix_offset_(&text,i+1);
						return tok;
					}
				}
			}
			++i;
		} else {
			if (c == end_marker[end_marker_offset]) {
				++end_marker_offset;
				if (end_marker_offset == end_marker_len) {
					tok.length = i + 1 - end_marker_offset;
					tok.type = http2_TOKEN_OK;
					advance[0] = http2_text_suffix_offset_(&text,i+1);
					return tok;
				}
			} else {
				// check if c is forbidden
				for (s32 j=0;j<forbidden_marker_len;++j) {
					if (c == forbidden_char[j]) {
						tok.length = i;
						tok.type = http2_TOKEN_INVALID;
						advance[0] = http2_text_suffix_offset_(&text,i+1);
						return tok;
					}
				}
				// assume it is a valid character let it fly
			}
			++i;
		}
	}

	tok.length = i - end_marker_offset;
	tok.type = http2_TOKEN_EOB;
	advance[0] = http2_text_suffix_offset_(&text,i);


	return tok;

}

//
// state machine of the request
//
// 2 REQUEST_METHOD
// 3 REQUEST_TARGET
// 4 REQUEST_VERSION
// 6 REQUEST_FIELD_KEY
// 7 REQUEST_FIELD_VALUE
//



//
// send more data to the request
//
static s32
http2_store_string_including_partial_string_(http2_Channel *self, void *buffer, s32 length)
{
	// make sure to store the length also: 4 bytes behind the
	// beginning of the string
	// right offset of the next partial string
	if (!self->partial_text_length) {
		// simple case
		s32 right_len = RAlign(length + sizeof(http2_String) + 1, 8);
		if (self->left + right_len <= self->right) {
			self->right -= right_len;
			http2_String *str = OffsetedPointer(self, self->right);
			str->length = length;
			char *dst = str->text;
			platform.memory_copy(dst, buffer, length);
			dst[length] = 0;
			return self->size - self->right;
		} else {
			return 0; // overflow
		}
	} else {
		// simple case
		s32 right_len = RAlign(length + sizeof(s32) + 1, 8);
		if (self->left + right_len <= self->right) {
			self->right -= right_len;
			char *prefix = OffsetedPointer(self, self->left);
			http2_String *str = OffsetedPointer(self, self->right);
			str->length = length;
			char *dst = str->text;
			s32 n = Min(length,self->partial_text_length);
			platform.memory_move(dst, prefix, n);
			if (length > n) {
				platform.memory_copy(dst + self->partial_text_length, buffer, length - n);
			}
			dst[length] = 0;
			return self->size - self->right;
		} else {
			return 0; // overflow
		}
	}
}

static char*
http2_get_string_(http2_Channel *self, s32 handle, s32 *length)
{
	http2_String *str = RightOffsetedPointer(self, self->size, handle);
	if (length) {
		length[0] = str->length;
	}
	return str->text;
}

static http2_Field*
http2_get_field_(http2_Channel *self, s32 index)
{
	Assert(index < self->request.num_fields);
	return OffsetedPointer(self, sizeof(http2_Channel) + index * sizeof(http2_Field));
}

static http2_Field*
http2_push_field_(http2_Channel *self)
{
	if (self->left + sizeof(http2_Field) <= self->right) {
		http2_Field *field = OffsetedPointer(self, self->left);
		self->left += sizeof(http2_Field);
		++self->request.num_fields;
		++self->response.num_fields;
		field[0] = (http2_Field) { 0 };
		return field;
	} else {
		return 0;
	}
}

static s32
http2_parse_append_partial_text_(http2_Channel *self, void *buffer, s32 length)
{
	http2_log3("in:  '%.*s'\n", self->partial_text_length, (char*) OffsetedPointer(self,self->left));
	if (self->left + self->partial_text_length + length <= self->right) {
		platform.copy_memory(OffsetedPointer(self, self->left + self->partial_text_length), buffer, length);
		self->partial_text_length += length;
		http2_log3("out: '%.*s'\n", self->partial_text_length, (char*) OffsetedPointer(self,self->left));
		return 1;
	} else {
		// @todo return error and try to recover
		http2_log3("\nBuffer Overflow\n");
		return 0;
	}
}

//
// if successful, this clears any partial text.
// we can also call with length == 0, it will do the right thing
// on failure, nothing changes in the self object
//
static s32
http2_store_string_and_goto_(http2_Channel *self, void *buffer, s32 length, s32 *dst, s32 next_state)
{
	s32 ok = 1;
	if (length > 0) {
		*dst = http2_store_string_including_partial_string_(self, buffer, length);
		ok = dst[0] != 0;
	} else {
		*dst = 0;
	}
	if (ok) {
		self->state = next_state;
		self->partial_text_length = 0;
		return 1;
	} else {
		return 0;
	}
}

static char*
http2_str_value_(char *st)
{
	static char* null_st = "NULL";
	if (!st) return null_st;
	else return st;
}

//
// send more data to the request
//
static void
http2_request_reset_(http2_Channel *self)
{
	self->request.method = 0;
	self->request.target = 0;
	self->request.version = 0;
	self->request.num_fields = 0;
	self->state = http2_STATE_REQUEST_METHOD;
	s32 new_left = sizeof(http2_Channel);
	platform.memory_move(OffsetedPointer(self,new_left), OffsetedPointer(self,self->left), self->partial_text_length);
	self->left = new_left;
	self->right = self->size;
}

static void
http2_response_reset_(http2_Channel *self)
{
	self->response.version = 0;
	self->response.status_code = 0;
	self->response.reason_phrase = 0;
	self->response.num_fields = 0;
	self->state = http2_STATE_RESPONSE_VERSION;
	s32 new_left = sizeof(http2_Channel);
	platform.memory_move(OffsetedPointer(self,new_left), OffsetedPointer(self,self->left), self->partial_text_length);
	self->left = new_left;
	self->right = self->size;
}

static void
http2_reset(http2_Channel *self)
{
	self->request.method = 0;
	self->request.target = 0;
	self->request.version = 0;
	self->request.num_fields = 0;
	self->state = http2_STATE_REQUEST_METHOD;
	self->partial_text_length = 0;
	self->left = sizeof(http2_Channel);
	self->right = self->size;
}

//
// send more data to the request
//
static void
http2_push_request_data(http2_Channel *self, void *buffer, s32 length)
{
	s32 offset = 0;
	s32 advance = 0;

	s32 type = 0;

	http2_Token tok = { 0 };

#define http2_M_str_end(s) s , (s + sizeof(s)-1)
#define http2_M_str_len(s) s , sizeof(s)-1

	// on any error, reset to the request method state

	for (;;) {
		offset += advance;
		advance = 0;

		if (self->state == http2_STATE_REQUEST_METHOD) {

			tok = http2_next_token_(self, http2_M_str_len(" "), http2_M_str_len("\0\t\n\r"), OffsetedPointer(buffer,offset), length-offset, &advance);

			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &self->request.method, http2_STATE_REQUEST_TARGET);
				http2_log("Request, method:'%.*s'\n", 50, http2_str_value_(http2_get_string_(self,self->request.method, 0)));
			} else {
				http2_log("PARSER_ERROR: Expected 'method' text\n");
				http2_request_reset_(self);

				//
				// should make all text until the failed token disappear
				//
				http2_log2("method failed - tok.length:%d - advance:%d - partial(%d):%.*s\n",
					   tok.length,
					   advance,
					   self->partial_text_length,
					   50,
					   (char*) OffsetedPointer(self, self->left));
				self->partial_text_length = 0;
				// if (self->partial_text_length > 0) {
				// 	self->partial_text_length = 0;
				// } else if (advance == 0) {
				// 	advance=1; // make sure we advane of at leas one unit
				// 	char *partial_text = OffsetedPointer(self, self->left);
				// 	partial_text[0] = ((char*)buffer)[offset];
				// 	self->partial_text_length = 1;
				// }
				// put the next char on the partial
			}

		} else if (self->state == http2_STATE_REQUEST_TARGET) {

			tok = http2_next_token_(self, http2_M_str_len(" "), http2_M_str_len("\0\t\n\r"), OffsetedPointer(buffer,offset), length-offset, &advance);

			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &self->request.target, http2_STATE_REQUEST_VERSION);

				s32 target_len = 0;
				char *target = http2_get_string_(self,self->request.target,&target_len);
				if (target) {
					if (target_len > 80) {
						http2_log("Request, target:'%.40s ... %.40s'\n", target, target + target_len - 40);
					} else {
						http2_log("Request, target:'%s\n", target);
					}
				} else {
						http2_log("Request, target:null");
				}
			} else {
				http2_log("PARSER_ERROR: Expected 'target' text... resetting\n");
				http2_request_reset_(self);
			}

		} else if (self->state == http2_STATE_REQUEST_VERSION) {

			tok = http2_next_token_(self, http2_M_str_len("\r\n"), http2_M_str_len(" \0\t\n"), OffsetedPointer(buffer,offset), length-offset, &advance);

			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &self->request.version, http2_STATE_REQUEST_FIELD_KEY);
				http2_log("Request, version:'%.*s'\n", 50, http2_str_value_(http2_get_string_(self,self->request.version,0)));
			} else {
				http2_log("PARSER_ERROR: Expected 'version' text... resetting\n");
				http2_request_reset_(self);
			}

		} else if (self->state == http2_STATE_REQUEST_FIELD_KEY) {
			//
			// this is a fragile case:
			// - if we find a ':' withouth any invalid char then
			//      - done we found a key
			// - otherwise
			//      - if we find '\r\n' found the trigger for a request
			//      - otherwise
			//          - if end of batch (which might have a partial \r or none of ':\n\r')
			//              - done for now, wait for more data with the partial text saved
			//          - otherwise error and reset
			//

			tok = http2_next_token_(self, http2_M_str_len(":"), http2_M_str_len(" \0\t\n\r"), OffsetedPointer(buffer,offset), length-offset, &advance);
			if (tok.type == http2_TOKEN_OK) {
				s32 st = 0;
				s32 ok = http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &st, http2_STATE_REQUEST_FIELD_VALUE);
				if (ok) {
					http2_Field *field = http2_push_field_(self);
					if (!field) {
						http2_log("Request, overflow while inserting new field\n");
						http2_request_reset_(self);
					} else {
						field->key = st;
						http2_log("Request, field key:'%s'\n", http2_str_value_(http2_get_string_(self,field->key,0)));
					}
				} else {
					http2_log("Request, overflow while inserting key\n");
					http2_request_reset_(self);
				}
			} else {
				// here we need to check for \r\n
				tok = http2_next_token_(self, http2_M_str_len("\r\n"), http2_M_str_len(" \0\t\n"), OffsetedPointer(buffer,offset), length-offset, &advance);
				if (tok.type == http2_TOKEN_EOB) {
					http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
					return;
				} else if (tok.type == http2_TOKEN_OK) {
					if (tok.length == 0) {
						// @todo trigger event on the callback
						http2_log("Request, identified a complete request: trigger it!\n");
						if (self->callback) {
							self->callback(self);
						}
						self->partial_text_length = 0;
					} else {
						http2_log("PARSER_ERROR: Expected either a key of an empty line\n");
						advance = 0;
					}
					http2_request_reset_(self);
				} else {
					http2_log("PARSER_ERROR: Expected either a key of an empty line\n");
					http2_request_reset_(self);
				}
			}
		} else if (self->state == http2_STATE_REQUEST_FIELD_VALUE) {

			tok = http2_next_token_(self, http2_M_str_len("\r\n"), http2_M_str_len("\0\n"), OffsetedPointer(buffer,offset), length-offset, &advance);
			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_Field * field = http2_get_field_(self, self->request.num_fields-1);
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &field->value, http2_STATE_REQUEST_FIELD_KEY);
				http2_log("Request, field_value:'%s'\n", http2_str_value_(http2_get_string_(self,field->value,0)));
			} else {
				http2_log("PARSER_ERROR: Expected field value\n");
				http2_request_reset_(self);
			}
		} else {
			http2_log("Unknown State\n");
			InvalidCodePath;
		}
	}

#undef http2_M_str_len

}



//
// send more data to the request
//
static void
http2_push_response_data(http2_Channel *self, void *buffer, s32 length)
{
	s32 offset = 0;
	s32 advance = 0;

	s32 type = 0;

	http2_Token tok = { 0 };

#define http2_M_str_len(s) s , sizeof(s)-1

	// on any error, reset to the response method state

	for (;;) {
		offset += advance;
		advance = 0;

		if (offset == length) {
			return;
		} else if (self->state == http2_STATE_RESPONSE_VERSION) {

			tok = http2_next_token_(self, http2_M_str_len(" "), http2_M_str_len("\0\t\n\r"), OffsetedPointer(buffer,offset), length-offset, &advance);

			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &self->response.version, http2_STATE_RESPONSE_STATUS_CODE);
				http2_log("Response, version:'%.*s'\n", 50, http2_str_value_(http2_get_string_(self,self->response.version, 0)));
			} else {
				http2_log("PARSER_ERROR: Expected 'version' text\n");
				http2_response_reset_(self);

				//
				// should make all text until the failed token disappear
				//
				http2_log2("method failed - tok.length:%d - advance:%d - partial(%d):%.*s\n",
					   tok.length,
					   advance,
					   self->partial_text_length,
					   50,
					   (char*) OffsetedPointer(self, self->left));
				self->partial_text_length = 0;
				// if (self->partial_text_length > 0) {
				// 	self->partial_text_length = 0;
				// } else if (advance == 0) {
				// 	advance=1; // make sure we advane of at leas one unit
				// 	char *partial_text = OffsetedPointer(self, self->left);
				// 	partial_text[0] = ((char*)buffer)[offset];
				// 	self->partial_text_length = 1;
				// }
				// put the next char on the partial
			}

		} else if (self->state == http2_STATE_RESPONSE_STATUS_CODE) {

			tok = http2_next_token_(self, http2_M_str_len(" "), http2_M_str_len("\0\t\n\r"), OffsetedPointer(buffer,offset), length-offset, &advance);

			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &self->response.status_code, http2_STATE_RESPONSE_REASON_PHRASE);
				http2_log("Response, version:'%.*s'\n", 50, http2_str_value_(http2_get_string_(self,self->response.status_code, 0)));
			} else {
				http2_log("PARSER_ERROR: Expected 'status_code' text... resetting\n");
				http2_response_reset_(self);
			}

		} else if (self->state == http2_STATE_RESPONSE_REASON_PHRASE) {

			tok = http2_next_token_(self, http2_M_str_len("\r\n"), http2_M_str_len(" \0\t\n"), OffsetedPointer(buffer,offset), length-offset, &advance);

			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &self->response.reason_phrase, http2_STATE_RESPONSE_FIELD_KEY);
				http2_log("Response, reason_phrase:'%.*s'\n", 50, http2_str_value_(http2_get_string_(self,self->response.reason_phrase,0)));
			} else {
				http2_log("PARSER_ERROR: Expected 'reason_phrase' text... resetting\n");
				http2_response_reset_(self);
			}

		} else if (self->state == http2_STATE_RESPONSE_FIELD_KEY) {
			//
			// this is a fragile case:
			// - if we find a ':' withouth any invalid char then
			//      - done we found a key
			// - otherwise
			//      - if we find '\r\n' found the trigger for a response
			//      - otherwise
			//          - if end of batch (which might have a partial \r or none of ':\n\r')
			//              - done for now, wait for more data with the partial text saved
			//          - otherwise error and reset
			//

			tok = http2_next_token_(self, http2_M_str_len(":"), http2_M_str_len(" \0\t\n\r"), OffsetedPointer(buffer,offset), length-offset, &advance);
			if (tok.type == http2_TOKEN_OK) {
				s32 st = 0;
				s32 ok = http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &st, http2_STATE_RESPONSE_FIELD_VALUE);
				if (ok) {
					http2_Field *field = http2_push_field_(self);
					if (!field) {
						http2_log("Response, overflow while inserting new field\n");
						http2_response_reset_(self);
					} else {
						field->key = st;
						http2_log("Response, field key:'%s'\n", http2_str_value_(http2_get_string_(self,field->key,0)));
					}
				} else {
					http2_log("Response, overflow while inserting key\n");
					http2_response_reset_(self);
				}
			} else {
				// here we need to check for \r\n
				tok = http2_next_token_(self, http2_M_str_len("\r\n"), http2_M_str_len(" \0\t\n"), OffsetedPointer(buffer,offset), length-offset, &advance);
				if (tok.type == http2_TOKEN_EOB) {
					http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
					return;
				} else if (tok.type == http2_TOKEN_OK) {
					if (tok.length == 0) {
						//
						// search for the Content-Length field
						//
						s32 valid_data_length = 1;
						u64 data_length = 0;
						for (s32 i=0;i<self->response.num_fields;++i) {
							http2_Field *field = http2_get_field_(self, i);
							if (field->key) {
								char *key = http2_get_string_(self, field->key, 0);
								if (cstr_match(key, "Content-Length")) {
									if (field->value) {
										s32 len = 0;
										char *val = http2_get_string_(self, field->value, &len);
										MemoryBlock b = cstr_trim(val, val+len, http2_M_str_end(" \t"));
										valid_data_length = pt_str_to_u64(b.begin, MemoryBlock_length(&b), &data_length);
									}
									break;
								}
							}
						}


						if (!valid_data_length) {
							http2_log("Couldn't parse Content-Length, discarding this response\n");
							http2_response_reset_(self);
						} else {
							self->response.length = data_length;

							s64 block_length = Min(length - (offset + advance), data_length);

							self->response.block_offset = 0;
							self->response.block_length = block_length;
							self->response.data = (block_length ? OffsetedPointer(buffer, offset + advance) : (void*) 0);

							s32 complete = block_length == data_length;
							http2_log("Response, %s identified!\n", complete ? "totally" : "partially");

							// only signal here if no extra data is available for response (data_length == 0)
							// or if some (or all) data is already available
							if (self->callback && (data_length == 0 || block_length > 0)) {
								self->callback(self);
							}

							advance += block_length;

							if (complete) {
								http2_response_reset_(self);
							} else {
								// set next state to be to read and forward more data
								self->state = http2_STATE_RESPONSE_DATA;
							}
						}
						self->partial_text_length = 0;
					} else {
						http2_log("PARSER_ERROR: Expected either a key or an empty line\n");
						advance = 0;
					}
				} else {
					http2_log("PARSER_ERROR: Expected either a key of an empty line\n");
					http2_response_reset_(self);
				}
			}
		} else if (self->state == http2_STATE_RESPONSE_FIELD_VALUE) {

			tok = http2_next_token_(self, http2_M_str_len("\r\n"), http2_M_str_len("\0\n"), OffsetedPointer(buffer,offset), length-offset, &advance);
			if (tok.type == http2_TOKEN_EOB) {
				http2_parse_append_partial_text_(self, OffsetedPointer(buffer, offset), advance);
				return;
			} else if (tok.type == http2_TOKEN_OK) {
				http2_Field * field = http2_get_field_(self, self->response.num_fields-1);
				http2_store_string_and_goto_(self, OffsetedPointer(buffer, offset), tok.length, &field->value, http2_STATE_RESPONSE_FIELD_KEY);
				http2_log("Response, field_value:'%s'\n", http2_str_value_(http2_get_string_(self,field->value,0)));
			} else {
				http2_log("PARSER_ERROR: Expected field value\n");
				http2_response_reset_(self);
			}
		} else if (self->state == http2_STATE_RESPONSE_DATA) {
			Assert(advance == 0);
			Assert(self->partial_text_length == 0);
			Assert(self->response.length > 0);

			s64 data_length = self->response.length;

			self->response.block_offset += self->response.block_length;

			s64 block_length = Min(length - offset, data_length - self->response.block_offset);

			self->response.block_length = block_length;
			self->response.data = (block_length ? OffsetedPointer(buffer, offset) : (void*) 0);

			s32 complete = self->response.block_offset + self->response.block_offset == data_length;
			http2_log2("Response, %s data from response being pushed!\n", complete ? "last" : "more");

			if (self->callback) {
				self->callback(self);
			}

			advance += block_length;

			if (complete) {
				http2_response_reset_(self);
			} else {
				// set next state to be to read and forward more data
				return;
			}
		} else {
			http2_log("Unknown State\n");
			InvalidCodePath;
		}
	}

#undef http2_M_str_len

}




//------------------------------------------------------------------------------
//
//
// http2_List
//
//
//------------------------------------------------------------------------------

typedef struct {
	http2_Channel* *channels;
	u64 num_channels;
	volatile u64 next_channel;
} http2_List;

static http2_List*
http2_list_new(a_Arena *arena, s32 num_channels, s32 size_per_channel, http2_Callback *callback, void *user_data)
{
	http2_List *channels =  a_push(arena, sizeof(http2_List), 8, 0);
	http2_Channel* *array = a_push(arena, sizeof(http2_Channel*) * num_channels, 8, 0);
	for (s32 i=0;i<num_channels;++i) {
		void *buffer = a_push(arena, size_per_channel, 8, 0);
		array[i] = http2_new(buffer, size_per_channel, i+1, callback, user_data);
	}
	channels[0] = (http2_List) {
		.channels = array,
		.num_channels = num_channels,
		.next_channel = 0
	};
	return channels;
}

static http2_Channel*
http2_list_get(http2_List  *self, s32 index)
{
	Assert(index < self->num_channels);
	return self->channels[index];
}

static http2_Channel*
http2_list_reserve(http2_List *self)
{
	//
	// @todo add some time-out mechanism?
	//
	http2_Channel *channel = 0;
	for (;;) {
		u64 index = self->next_channel;
		if (pt_atomic_cmp_and_swap_u64(&self->next_channel, index, index+1) == index) {
			// ok we got a shot at channel index % num_channels
			// if it is in use we won't be able to swap its flag
			channel = self->channels[index % self->num_channels];
			if (pt_atomic_cmp_and_swap_u64(&channel->flags, 0, 1) == 0) {
				// all right we are ready
				break;
			} else {
				platform.thread_sleep(1); // wait a bit
			}
		}
	}
	http2_reset(channel);
	return channel;
}

static void
http2_list_free(http2_List *self, http2_Channel *channel)
{
	for (;;) {
		// this one should be always successful once only
		// one socket 'has' this channel
		if (pt_atomic_cmp_and_swap_u64(&channel->flags, 1, 0) == 1) {
			break;
		} else {
			platform.thread_sleep(1); // wait a bit
		}
	}
}

//-----------------------------------------------------------------------------
//
//
//
// main
//
//
//
//-----------------------------------------------------------------------------

#ifdef http2_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

#include "print.c"

#include "../nix_platform.c"

// returns 0 if couldn't open the file
static char*
read_file_into_heap_as_c_str(char *filename, u64 *output_file_size)
{
	// read list of laccids we are interested, should be in memory
	FILE *fp = fopen(filename,"r");
	if (!fp) {
		return 0;
	}
	// read whole file into memory
	fseek(fp, 0L, SEEK_END);
	u64 file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	char *c_str = (char*) malloc(file_size + 1); // +1 for null terminated string
	u64 bytes_read = fread(c_str, file_size, 1, fp);
	fclose(fp);
	c_str[file_size] = 0;
	if (output_file_size) {
		*output_file_size = file_size;
	}
	return c_str;
}


int bug(int argc, char *argv[])
{
	nix_init_platform(&platform);

	Arena arena = { 0 };

	s32  length = Megabytes(4);
	void *buffer = Arena_push(&arena, length, 8, 0);

	http2_Channel *channel = http2_new(buffer, length, 1, 0, 0);

	// read file into a cstr

	u64 size = 0;
	char *message = read_file_into_heap_as_c_str("/tmp/data_4.raw", &size);
	u64 pushed = 0;
	while (pushed < size) {
		s32 block = MIN(65535,size-pushed);
		http2_push_request_data(channel, message+pushed, block);
		pushed += block;
	}
	return 0;
}

int bug2(int argc, char *argv[])
{
	if (argc < 2) return 0;
	nix_init_platform(&platform);

	Arena arena = { 0 };

	s32  length = Megabytes(4);
	void *buffer = Arena_push(&arena, length, 8, 0);

	http2_Channel *channel = http2_new(buffer, length, 1, 0, 0);

	// read file into a cstr

	u64 size = 0;
	char *message = read_file_into_heap_as_c_str(argv[1], &size);
	u64 pushed = 0;
	while (pushed < size) {
		s32 block = MIN(57,size-pushed);
		http2_push_request_data(channel, message+pushed, block);
		pushed += block;
	}
	return 0;
}




int main(int argc, char *argv[])
{
#if 0
	nix_init_platform(&platform);

	Arena arena = { 0 };

	s32  length = Megabytes(4);
	void *buffer = Arena_push(&arena, length, 8, 0);

	http2_Channel *channel = http2_new(buffer, length, 0, 0);

	char *message = "GET /target HTTP/1.1\r\nContent-Length:1024\r\nencoding:text/plain here we\tgo\r\n\r\nGET /target HTTP/1.1\r\nGET /target HTTP/1.1\r\nContent-Length:1024\r\nencoding:text/plain\r\n\r\ndhdfasdkfhj\r\nfdsafsd jlasdkfjasd lkasjdflkjasd \r \n \t lkasjdflsdklklasdf     \rGET /target HTTP/1.1\r\nContent-Length:1024\r\n\r\n";

	for (s32 i=0;i<cstr_len(message);++i) {
		http2_push_request_data(channel, message+i, 1);
	}

	return 0;
#elif 0
	return bug(argc, argv);
#else
	return bug2(argc, argv);
#endif
}

#endif

