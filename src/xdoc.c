//
// Search for all occurrenced of the pattern
//
// /*
// BEGIN_DOC_STRING name
// ... any content ...
// END_DOC_STRING
//
// BEGIN_AUX_FILE name
// ... any content ...
// END_AUX_FILE
// */
//
// and dump a c array for it
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define Assert(EX) (void) ((EX) || (abort(), 0))
#define InvalidCodePath Assert(!"InvalidCodePath")

static char*
read_file_into_heap_as_c_str(char *filename, int *output_file_size)
{
	// read list of laccids we are interested, should be in memory
	FILE *fp = fopen(filename,"r");
	if (!fp) { return 0; }
	// read whole file into memory
	fseek(fp, 0L, SEEK_END);
	int file_size = (int) ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	char *c_str = (char*) malloc(file_size + 1); // +1 for null terminated string
	int bytes_read = (int) fread(c_str, file_size, 1, fp);
	fclose(fp);
	c_str[file_size] = 0;
	if (output_file_size) {
		*output_file_size = file_size;
	}
	return c_str;
}

static char *text = 0;
static int text_length = 0;

typedef struct {
	int begin;
	int end;
} Segment;


#define BLOCK_TYPE_DOC_FILE 1
#define BLOCK_TYPE_AUX_FILE 2

typedef struct {
	Segment name;
	Segment content;
	int     type;
} Block;

static int
match_any(char c, char *str)
{
	while (*str) {
		if (c == *str) return 1;
		++str;
	}
	return 0;
}

static int
match_str(Segment s, char *str)
{
	int i = s.begin;
	while (*str) {
		if (i == s.end || text[i] != *str) return 0;
		++str;
		++i;
	}
	return (i == s.end);
}

//
// discard empty lines
//
static int
next_nonempty_token(Segment *context, char *delim, Segment *result)
{
	int end = context->end;
	for (;;) {

		int c = context->begin;

		if (c == end) break;

		// search for new line
		int i=c;
		while (i<end) {
			if (match_any(text[i],delim)) {
				// sequence of delimiters is treated as one
				context->begin = i + 1;
				break;
			}
			++i;
		}

		if (i > c) {
			result[0] = (Segment) { .begin = c, .end = i };
			return 1;
		}
	}
	return 0;
}

static int
seg_len(Segment s)
{
	return s.end - s.begin;
}


int
main(int argc, char *argv[])
{
	// read file into string
	if (argc != 2) {
		fprintf(stderr, "Usage: xdoc C_FILENAME\n");
		return 0;
	}

	text = read_file_into_heap_as_c_str(argv[1], &text_length);
	if (!text) {
		fprintf(stderr, "Could not read file\n");
	}

	Segment context = { .begin = 0, .end = text_length };

	int max_blocks = 10000;
	Block *blocks = malloc(sizeof(Block) * 10000);
	int block_index = 0;


	// state is 1 ----> identified the start of doc file
	// state is 2 ----> identified the start of aux file

	int state = 0;
	Segment prev_line = { 0 };
	Segment line = { 0 };
	Segment tok = { 0 };
	while (next_nonempty_token(&context, "\n", &line)) {
		if (state == 0) {
			if (next_nonempty_token(&line, "\t ", &tok)) {
				// check if it matches BEGIN_DOC_STRING
				int doc_file = 0;
				int aux_file = 0;
				if ( (doc_file = match_str(tok, "BEGIN_DOC_STRING")) || (aux_file = match_str(tok, "BEGIN_AUX_FILE")) ) {
					if (next_nonempty_token(&line, "\t ", &tok)) {
						blocks[block_index] = (Block) {
							.name = tok,
							.content = { .begin = context.begin, .end = context.begin },
							.type = (doc_file ? BLOCK_TYPE_DOC_FILE : BLOCK_TYPE_AUX_FILE)
						};
						if (next_nonempty_token(&line, "\t ", &tok)) { }
						state = 1;
					} else {
						// warning BEGIN_DOC_STRING without identifier
					}
				}
			}
		} else if (state == 1) {
			if (next_nonempty_token(&line, "\t ", &tok)) {
				// check if it matches BEGIN_DOC_STRING
				char *end_marker = ((blocks[block_index].type == BLOCK_TYPE_DOC_FILE) ? "END_DOC_STRING" : "END_AUX_FILE");
				if (match_str(tok, end_marker)) {
					blocks[block_index].content.end = line.begin;
					{
						int b = blocks[block_index].content.begin;
						int e = blocks[block_index].content.end;
						if (b < e && text[e-1] == '\n' || text[e-1] == '\r') --e;
						blocks[block_index].content.end = e;
					}
					++block_index;
					if (next_nonempty_token(&line, "\t ", &tok)) {
						// warning: extra symbols on the END_DOC_STRING line being disconsidered
					}
					state = 0;
				}
			}
		}
	}


	//
	// write the doc strings to stdout and the aux files to
	// the named files
	//

	for (int i=0;i<block_index;++i) {
		Block *b = blocks + i;
		if (b->type == BLOCK_TYPE_DOC_FILE) {
			fprintf(stdout, "static const char %.*s[] = {", seg_len(b->name), text + b->name.begin);
			int off = b->content.begin;
			int len = seg_len(b->content);
			// print null terminator for string
			for (int j=0;j<len+1;++j) {
				if ((j % 10) == 0) {
					fprintf(stdout, "\n    ");
				}
				fprintf(stdout, "0x%02x%s",
					(j < len) ? (int) text[off + j] : 0,
					(j < len) ? "," : "");
			}
			fprintf(stdout, "\n};\n");
		} else if (b->type == BLOCK_TYPE_AUX_FILE) {
			char filename[1024];
			int n = seg_len(b->name);
			for (int i=0;i<n;++i) { filename[i] = text[b->name.begin + i]; }
			filename[n] = 0;
			FILE *f = fopen(filename, "w");
			fwrite(text + b->content.begin, 1, seg_len(b->content), f);
			fclose(f);
		} else {
			InvalidCodePath;
		}
	}

	return 0;
}

