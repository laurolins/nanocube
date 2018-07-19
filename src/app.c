/*
BEGIN_TODO

# 2018-04-11T14:38:14
Support multiple paths in a dive query.
Support multiple dive in the same query.

# 2017-11-29T11:07
A mechanism to bin numerical columns while creating a nanocube. Could be
done externally through an Rscript like

     #!/usr/bin/env Rscript
     # R --slave -e 'x <- scan(file="stdin", quiet=TRUE); summary(x)'
     args <- commandArgs(TRUE)
     num.breaks <- as.integer(args[1])
     values <- scan(file="stdin",sep="\n")
     b <- pretty(values[!is.na(values)],n=num.breaks)
     x <- cut(values,breaks=b,include.lowest=T)
     cat(sprintf("%02d_%s",x,x),sep="\n")

The issues is that it would need a two pass through the data so that
it could first bin the desired fields (read all data, sort it, and
bin on the right quantiles etc).

An alternative would be to send in the break points: one pass only
and have a minor sevice tailored to a single column to bin it automatically
based on a parameter indicating number of breaks based on quantiles or on
prettyness (R pretty function).

# 2017-10-06T23:38:27
Create an intermediate representation for an nm_Table with the nanocube
vector payload that will be easy for custom clients to read results.
It should contain no pointers and be easily serializable.

# 2017-07-11T11:35
Path-by-name everywhere we expect a path we could have also a path defined by
its name. Assume no amiguity.

# 2017-07-11T11:35
Evaluate if we need an interval query based on start/end time dimensions.

# 2017-07-11T11:35
Should we have a simple boundary query for a specific dimension (not
constrained min).

# 2017-07-10T10:50
HTTPs how to wrap our server so that it supports encrypted connections.

# 2017-07-10T10:50
Mechanism to initialize aliases for all levels of the hierarchy and
not just the leaves.

# 2017-07-10T11:39:26
When the aliases file is pre-defined, there should be a mode to make
it strict (ie no unknown values should be added to the alias
list) and a permissive mode that let new values go into unused slots.

# 2017-07-07T10:47:32 [DONE]
Use the schema of the last nanocube with the same alias.
New records might introduce new aliases, when we load a multipart
nanocube, we should respond with the most complete aliases set
which is in the last part.

# 2017-07-05T13:02:15 [DONE]
If a client closes a connection while we are still responding, a SIGPIPE error
is generated and the server is crashing on linux_platform.c write. Try running
test() on the console of google chrome with a large response (eg. schema with
lots of aliases) and disable the http default flags.
<html> <script>
function reqListener () { console.log(this.responseText); }
function test() {
	var oReq = new XMLHttpRequest();
	oReq.addEventListener("load", reqListener);
	oReq.open("GET", "http://www.example.org/example.txt");
	oReq.send();
}
</script> </html>
Used send instead of write with the MSG_NOSIGNAL
char buf[888];
//write( sockfd, buf, sizeof(buf) );
send(    sockfd, buf, sizeof(buf), MSG_NOSIGNAL );
https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly

# 2017-06-29T00:48:12
Improve nx_Threads_singleton performance

# 2017-06-29T00:48:12
Multi-threaded creation of multi-parts nanocube.

# 2017-06-26T22:34 [DONE]
Watermark into newly created nanocubes with __VERSION__.

# 2017-03-14T17:23 [DONE]
btree head kv was not being set on nanocube_btree. check other btrees.

# 2017-03-14T17:23 [DONE]
document nanocube query API and make it accessible from the command
line interface and when we hit an http server.

# 2017-03-15T11:54 [DONE]
Include separator option in csv-col

# 2017-03-16T15:36 [DONE]
Query should go under a function call? Maybe q(taxi.b(x)/census.b())
In this way we have context and the possibility of adding other stuff.
A format could be assoicated to a query, for instance. We can have
call for schema within the API. Multiple queries and schema calls
can be sent in a single request.

# 2017-03-20T14:02 [DONE]
Add an extra measure dimension type that is the row number in u32
or u64. This will help drawing small nanocubes of up to 32/64 records
and recognizing the different sets.

END_TODO
*/

/*
BEGIN_DOC_STRING nanocube_api_doc
Nanocube API                                             __VERSION__
============

q(Q)
    Run query Q. Q should be a *measure* object, and the evaluation
    of the *measure* Q yields the query result (a table).

    A query object can be formatted in a few ways:

    format(F)
         where F is either 'text', 'json', or 'bin'

schema([S*(','S)])
    list the schema of all data sources (or the ones with the given aliases S).

version()
    lists the API and executable versions.

memory([S*(','S)])
    show memory usage of all data sources (or the ones with the given aliases S).

b(DIM_NAME, <TARGET>, <format>)

## Source

A *source* captures the notion of a structured data source. A source is
associated with one or more *index dimensions* and one or more *measure
dimensions*. One can assume that a given *nanocube frontend* has a set of
*source* objects available. These *sources* and their descriptions can be
obtained sending the `sources();` message to the querying engine.

## Dimension Binding

```
<source> . b(dimension_name: <string>, <target>)
<source> . b(dimension_name: <string>, <target>, <format>)
```

A *dimension binding* object associates a *name* to a *target* object.  One can
combine a *dimension bindings* with a *source* object using the `.` binary
operator.  It is assumed that the given name of a *dimension binding* is also
the name of a *free* dimension in the *source* object.

A *format* when defined specifies that instead of *loop indices* or *paths* as
values in the *index dimension* columns, a user wants some tranformed version
of those values. For example, in the case of a time interval, the user would
like to get a time window.
```
time
2016-10-14T23:00:00-05:00 3600
```

## Selection

```
<source> . select(<string>[, <string>]*)
```

A list of one or more names to be combined to a *source* object. The assumption
is that those names are valid measure names in the *source* and that the source
has not other selection associated to it.

## Targets

There are a few targets that cover the use cases.

### Path

```
p(N*)
S
```
either a sequence of numbers N or a string S representing a path alias can
be used to specify a path.

### tile2d or img2d

```
tile2d(level, x, y)
img2d(level, x, y)    # this one is the slippy tile coords
```

Returns a path target.

### Region

Region is a target based on lat/lon polygons. We define a region
by feeding RESOLUTION and POLY to the 'region' function.

    region(RESOLUTION, POLY)

A POLY primitive is defined by the poly function with a sequence
of lat/lon pairs that are comma separated in a string:

    poly(LAT_LON_STRING)

We can combine POLY objects using

    poly_complement(POLY)
    poly_diff(POLY, POLY)
    poly_union(POLY [, POLY]*)
    poly_symdiff(POLY [, POLY]*)
    poly_intersection(POLY [, POLY]*)

#
# contour(lat1,lon1,lat2,lon2,...,latN,lonN)
# region(contour_negate(contour_union(contour(lat,lon,lat,lon,...,lat,lon),...,)))
#

### Path List

pathagg(<path>*)

Aggregate the values of all paths in the list, it might count multiple
times the same "bins" if the given paths are not disjoint.

### Dive

```
dive(levels:<number>)
dive(base:<path>, depth:<number>)
dive(PATH_LIST, DEPTH)
dive(PATH_LIST, DEPTH_LIST)
```

A *dive* object indicates a set of nodes within a hierarchy.  Each existing
descendant node of *base* node (root node if no base is defined) that is
*depth* levels deeper than *base* should and exists in the actual query context
should be reported as a separate result. In other words, a *dive* target
branches the result report.

### Mask

```
mask(<string>)
```

A *mask* target represents a hierarchy traversal object in a string object.
For now, this string is limited to labels from 0 to 9. In other words, if we
want traversals of wider hierarchies (some nodes with more than 10 children) a
more general format is needed. The interpretation of the traversal object is
that a user is interested in the information that resides on the leaves of the
traversal. No branching on the report occurs.


```python
events.b('location',mask('020<<11<<<3<<'))
```

### Binding

```python
b(<dimension-name>, <target> (, <string>)*)
```

Associates a target to a dimension which will ultimately specify what to look
for every time we traverse that dimension. One can also associate names to
the binding that might impact, for instance, how to represent the output
of that dimension.

```
b('type',dive(1),'text')
b('location',dive(p(2,1,2),8),'img8')
b('location',dive(p(2,1,2),8),'img11')
```

### Interval

```
interval(a:<number>, b:<number>)
```

Focus only on records in [a,b) interval. No branching on the report occurs.
Valid on binary hierarchies.


### Interval Sequence

```
intseq(base:<number>, width:<number>, count:<number>, stride:<number>)
intseqagg(base:<number>, width:<number>, count:<number>, stride:<number>)
```

Generate a sequence of results. Coarsest hierarchy nodes that fall in the
interval `[base, base + width)` will be reported with label 0. Coarsed
hierarchy nodes that fall in the interval `[base + width, base + 2*width)` will
be reported with label 1, and so on up to `[base + (count-1)*width, base +
count*width)`.  This target branches the result report.

### Time Series and Time Series Aggregate

```
timeseries(base:<string>, width_secs:<number>, count:<number>, stride_secs:<number>)
ctimeseries(base:<string>, width_secs:<number>, count:<number>, stride_secs:<number>)
timeseriesagg(base:<string>, width_secs:<number>, count:<number>, stride_secs:<number>)
```

Generate a sequence of results based on an initial date. Coarsest hierarchy
nodes that fall in the interval `[base, base + width)` will be reported with
label 0. Coarsed hierarchy nodes that fall in the interval `[base + stride +
width, base + stride + 2*width)` will be reported with label 1, and so on up to
`[base + (count-1)*stride + (count-1)*width, base + (count-1)*stride +
count*width)`.  This target branches the result report.

```python
# first 24 hours of June 1st, 2016
timeseries('2016-06-01 00:00:00 -05:00', 3600, 24, 3600)

# keep cumulative measure of the firt 24 hours of June 1st, 2016
ctimeseries('2016-06-01 00:00:00 -05:00', 3600, 24, 3600)

# first hour of 24 days starting in June 1st, 2016
timeseries('2016-06-01 00:00:00 -05:00', 3600, 24, 24*3600)

# same weekday for 4 weeks starting on June 1st, 2016
timeseries('2016-06-01 00:00:00 -05:00', 24*3600, 4, 7*24*3600)
```

END_DOC_STRING
*/

#include "base/platform.c"

//------------------------------------------------------------------------------
// Request
//------------------------------------------------------------------------------

//
// request language
//
// <command> <params>
//
// estimate <input>
// estimate crimes_nc.dmp
//
//   - count input records
//   - insert a sample of records and interpolate
//   - estimate time also :)
//   - simple heuristic
//
// create <input> <output> <data_memory>
// create crimes_nc.dmp crimes.nanocube_count 4G
//

// platform util assumes a platform global variable
// is available
global_variable PlatformAPI platform;

/* time and UTC labels handling */
#include "base/time.c"

#include "base/tokenizer.c"
#include "base/options.c"
#include "base/util.c"

#ifdef POLYCOVER
#include "polycover/polycover.h"
#endif

#include "app.h"

/* reference to application state (comes from platform dependent compilation unit) */
global_variable ApplicationState *global_app_state = 0;

/* Request */

typedef struct Request {
	ApplicationState    *app_state;
	pt_File             *pfh_stdin;
	pt_File             *pfh_stdout;
	op_Options          options;
	Print               *print;
	// nt_Tokenizer        tok; // request tokens
} Request;

static Request *debug_request = 0;

internal void
Request_msg(Request *req, const char *cstr)
{
	Print_cstr(req->print, cstr);
	platform.write_to_file(req->pfh_stdout, req->print->begin, req->print->end);
	Print_clear(req->print);
}

internal void
Request_print(Request *req, Print *print)
{
	platform.write_to_file(req->pfh_stdout, print->begin, print->end);
}

/* parse date from text */
#include "base/alloc.c"
#include "base/random.c"
#include "base/time_parser.c"
#include "base/sort.c"
#include "base/http.c"
#include "base/set.c"
#include "base/csv.c"
#include "base/aatree.c"


/* btree data structure */
#include "nanocube/nanocube_btree.c"

#include "nanocube/nanocube_vector_payload.h"

/* the payload that will be stored on nx_PNode is the above struct */
#include "nanocube/nanocube_index.c"

#include "nanocube/nanocube_parser.c"

/* only depends on nanocube index part */
#include "nanocube/nanocube_measure.c"

// the #define below is to enable the function definitions to
// be considered as well as type definitions
#define NANOCUBE_VECTOR_TABLE_IMPLEMENTATION
#include "nanocube/nanocube_vector_table.c"

#include "nanocube/nanocube_vector.c"
#include "nanocube/nanocube_util.c"
/* used on service_create; depends on nanocube_parser */
#include "nanocube/nanocube_csv.c"
#include "nanocube/nanocube_dmp.c"

/* roadmap features */
#include "roadmap/string_set.c"
#include "roadmap/roadmap_parser.c"
#include "roadmap/roadmap_btree.c"
#include "roadmap/roadmap_graph.c"

/*
 * Include documentation strings.
 * NOTE: this file is automatically generated on every new
 * compilation and is not part of the repository
 */
#include "app.c.doc"

/* routines to present data collected from profiling */
#include "base/profile_collect.c"

//------------------------------------------------------------------------------
// Memory Limits
//------------------------------------------------------------------------------

static u64 app_service_serve_MEM_COMPILER            = Megabytes(8);
static u64 app_service_serve_MEM_PRINT_RESULT        = Megabytes(64);
static u64 app_service_serve_MEM_PRINT_HEADER        = Kilobytes(4);
static u64 app_service_serve_MEM_TABLE_INDEX_COLUMNS = Megabytes(64);
static u64 app_service_serve_MEM_TABLE_VALUE_COLUMNS = Megabytes(64);
/* largest GET query is bounded by http channel memory */
static u64 app_service_serve_MEM_HTTP_CHANNEL        = Megabytes(4);

//------------------------------------------------------------------------------
// snap function
//------------------------------------------------------------------------------

static rg_Graph     *g_snap_graph   = 0;
static f32          g_snap_maxdist = 0;

/* @TODO(llins): not multi-threaded! */
internal b8
g_snap(f32 *lat, f32 *lon)
{
	Assert(g_snap_graph);
	static rg_Heap      heap;
	static rg_HeapItem  heap_singleton;
	rg_Heap_init(&heap, &heap_singleton, &heap_singleton + 1);
	rg_Graph_nearest_neighbors(g_snap_graph, *lat, *lon, g_snap_maxdist, &heap, 0, 0);
	if (heap.begin != heap.end) {
		*lat = ((rg_Locate*) heap_singleton.data)->lat;
		*lon = ((rg_Locate*) heap_singleton.data)->lon;
		return 1;
	} else {
		return 0;
	}
}


//------------------------------------------------------------------------------
// u64 and u128 basic stuff
// @TODO(llins): think if we should make this a basic service on platform
//------------------------------------------------------------------------------

internal void
swap_u64(u64 *a, u64 *b)
{
	u64 aux = *a;
	*a = *b;
	*b = aux;
}

internal void
qsort_u64(u64 *begin, u64 *end)
{
	/* are there 2+ elements? */
	if (!(begin + 1 < end)) {
		return;
	}

	/* TODO: perturbe to avoid pretty bad pivots */
	u64 *left   = begin + 1;
	u64 *right  = end   - 1;
	u64 pivot = *begin;

	while (left < right) {
		while (*left <  pivot  && left < right) {
			++left;
		}
		while (pivot <= *right && left < right) {
			--right;
		}
		if (left < right) {
			swap_u64(left, right);
		}
	}
	/* position pivot at the right spot: either before left or after left */
	if (pivot > *left) {
		swap_u64(begin, left);
	} else  {
		swap_u64(begin, left-1);
		left = left - 1;
		// result = left;
	}

	qsort_u64(begin, left);
	qsort_u64(left+1, end);
}

internal b8
u128_lt(u128 *a, u128 *b)
{
	return (a->high < b->high) || ((a->high == b->high && a->low < b->low));
}

internal b8
u128_lte(u128 *a, u128 *b)
{
	return (a->high < b->high) || ((a->high == b->high && a->low <= b->low));
}

internal b8
u128_eq(u128 *a, u128 *b)
{
	return (a->high == b->high) && (a->low == b->low);
}

internal void
u128_swap(u128 *a, u128 *b)
{
	u128 aux;
	aux = *a;
	*a = *b;
	*b = aux;
}

internal void
qsort_u128(u128 *begin, u128 *end)
{
	/* are there 2+ elements? */
	if (!(begin + 1 < end)) {
		return;
	}

	/* perturbe to avoid pretty bad pivots */
	u128 *left   = begin + 1;
	u128 *right  = end   - 1;
	u128 *pivot  = begin;

	while (left < right) {
		while (u128_lt(left,pivot)  && left < right) {
			++left;
		}
		while (u128_lte(pivot,right) && left < right) {
			--right;
		}
		if (left < right) {
			u128_swap(left, right);
		}
	}
	/* position pivot at the right spot: either before left or after left */
	if (u128_lt(left,pivot)) {
		u128_swap(begin, left);
	} else  {
		u128_swap(begin, left-1);
		left = left - 1;
		// result = left;
	}

	qsort_u128(begin, left);
	qsort_u128(left+1, end);
}

internal b8
check_sorted_u128(u128 *begin, u128 *end)
{
	u128 *it = begin + 1;
	while (it != end) {
		if (u128_lt(it, it-1))
			return 0;
		++it;
	}
	return 1;
}

internal u64*
u64_remove_duplicates(u64 *begin, u64 *end)
{
	u64 *left  = begin;
	u64 *right = begin + 1;
	while (right != end) {
		if (*left == *right) {
			++right;
		} else {
			++left;
			*left = *right;
			++right;
		}
	}
	return left+1;
}

internal b8
check_sorted_u64(u64 *begin, u64 *end)
{
	u64 *it = begin + 1;
	while (it != end) {
		if (*it < *(it-1))
			return 0;
		++it;
	}
	return 1;
}

internal b8
check_sorted_uniqueness_u64(u64 *begin, u64 *end)
{
	u64 *it = begin + 1;
	while (it != end) {
		if (*it == *(it-1))
			return 0;
		++it;
	}
	return 1;
}


//------------------------------------------------------------------------------
// version service
//------------------------------------------------------------------------------

/*
BEGIN_DOC_STRING nanocube_service_version_doc
Ver:   __VERSION__
Usage: nanocube version NANOCUBE
Show the executable version that created the input NANOCUBE index.

END_DOC_STRING
*/

internal void
service_version(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_service_version_doc);
		Request_print(request, print);
		return;
	}

	// get next two tokens
	MemoryBlock input_filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &input_filename)) {
		Request_msg(request, "[memory:fail] no input filename given.\n");
		return;
	}

	//
	// could do it in the memory map way
	// add to the platform a way to map a file
	//
	pt_MappedFile mapped_file = platform.open_mmap_file(input_filename.begin, input_filename.end, 1, 0);
	if (!mapped_file.mapped) {
		Request_msg(request, "[memory:fail] couldn't open of the .nc file.\n");
		return;
	}

	al_Allocator*  allocator = (al_Allocator*) mapped_file.begin;
	MemoryBlock watermark_block = al_Allocator_watermark_area(allocator);

	Print_clear(print);
	Print_cstr_safe(print, watermark_block.begin, watermark_block.end);
	Print_char(print, '\n');
	Request_print(request, print);

	platform.close_mmap_file(&mapped_file);
}

//------------------------------------------------------------------------------
// memory service
//------------------------------------------------------------------------------
/*
BEGIN_DOC_STRING nanocube_memory_doc
Ver:   __VERSION__
Usage: nanocube memory NANOCUBE [OPTIONS]
Show the details of the memory usage of a NANOCUBE.
Options:
    -nodetail
        Do not scan all nodes and generates a report of nodes by degree.
	The memory command runs faster with this option.
    -print-empty-caches
        Also list the names of the empty caches.

END_DOC_STRING
*/

internal void
service_memory(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_memory_doc);
		Request_print(request, print);
		return;
	}

	// get next two tokens
	MemoryBlock input_filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &input_filename)) {
		Request_msg(request, "[memory:fail] no input filename given.\n");
		return;
	}

	b8 detail             = op_Options_find_cstr(options, "-nodetail") == 0;
	b8 print_empty_caches = op_Options_find_cstr(options, "-print-empty-caches") != 0;

	//
	// could do it in the memory map way
	// add to the platform a way to map a file
	//
	pt_MappedFile mapped_file = platform.open_mmap_file(input_filename.begin, input_filename.end, 1, 0);
	if (!mapped_file.mapped) {
		Request_msg(request, "[memory:fail] couldn't open of the .nc file.\n");
		return;
	}

	al_Allocator*  allocator = (al_Allocator*) mapped_file.begin;
	nv_Nanocube* cube = (nv_Nanocube*) al_Allocator_get_root(allocator);

	nu_log_memory(allocator, &cube->index, print, detail, print_empty_caches);

	Request_print(request, print);

	platform.close_mmap_file(&mapped_file);
}

//------------------------------------------------------------------------------
// draw
//------------------------------------------------------------------------------

static nv_Nanocube *nv_nanocube_instance = 0;
nu_PRINT_PAYLOAD(nv_print_payload)
{
	Assert(nv_nanocube_instance);
	for (u32 i=0;i<nv_nanocube_instance->num_measure_dimensions;++i) {
		f64 value = nv_Nanocube_get_value(nv_nanocube_instance, i,
				payload);
		if (i > 0)
			Print_char(print,',');
		Print_f64(print, value);
	}
}

/*
BEGIN_DOC_STRING nanocube_draw_doc
Ver:   __VERSION__
Usage: nanocube draw NANOCUBE DOTFILE
For a small NANOCUBE index, generate a visual representation of its
internal structure and save it on the output DOTFILE. To generate
a pdf from this .dot file one needs to have GraphViz installed
and run 'dot -Tpdf DOTFILE > PDFFILE'.

OPTIONS:
    -show-ids
        print node ids on top of the path labels
END_DOC_STRING
*/

internal void
service_draw(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_draw_doc);
		Request_print(request, print);
		return;
	}

	b8 show_ids = op_Options_find_cstr(options,"-show-ids") != 0;

	MemoryBlock input_filename  = { .begin=0, .end=0 };
	MemoryBlock output_filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &input_filename) ||
	    !op_Options_str(options, 2, &output_filename)) {
		Request_msg(request, "[draw] not enough input parameters.\n");
		Request_msg(request, "[draw] usage: nanocube draw <input> <output>.\n");
		return;
	}

	//
	// could do it in the memory map way
	// add to the platform a way to map a file
	//
	pt_File pfh = platform.open_read_file(input_filename.begin, input_filename.end);
	if (!pfh.open) {
		Request_msg(request, "[draw:fail] couldn't open of the .nc file.\n");
		return;
	}

	pt_Memory memory = platform.allocate_memory(pfh.size, 12, 0);
	char *begin = memory.memblock.begin;
	platform.read_next_file_chunk(&pfh, begin, begin + pfh.size);
	Assert(pfh.last_read == pfh.size);
	platform.close_file(&pfh);

	al_Allocator*  allocator = (al_Allocator*) begin;
	nv_Nanocube* cube = (nv_Nanocube*) al_Allocator_get_root(allocator);

	/* generate .dot file */
	nv_nanocube_instance = cube;
	nu_save_dot_file(&cube->index,output_filename.begin,
			 output_filename.end, nv_print_payload,
			 show_ids);

	Print_clear(print);
	Print_cstr(print, "[draw:msg] To generate .pdf of graph run:\n");
	Print_cstr(print, "dot -Tpdf -o");
	Print_str(print, output_filename.begin, output_filename.end);
	Print_cstr(print, ".pdf ");
	Print_str(print, output_filename.begin, output_filename.end);
	Print_cstr(print, "\n");
	Request_print(request, print);
}

//------------------------------------------------------------------------------
// ast service
//------------------------------------------------------------------------------

internal void
print_ast(Request *request, np_AST_Node* node, s32 level)
{
	Print *print = request->print;
	Print_clear(print);
	Print_char(print,' ');
	Print_align(print, level * 4, 0,' ');
	Print_cstr(print, np_AST_Node_Type_cstr(node->type));
	Print_char(print,' ');
	switch (node->type) {
	case np_AST_Node_Type_Number: {
			np_AST_Number* x = (np_AST_Number*) node->detail;
			if (x->is_integer) {
				Print_f64(print, (f64) x->ip);
			} else {
				Print_f64(print, x->fp);
			}
			Print_char(print,'\n');
			Request_print(request, print);
		} break;
	case np_AST_Node_Type_String: {
			np_AST_String* x = (np_AST_String*) node->detail;
			Print_str(print, x->str.begin, x->str.end);
			Print_char(print,'\n');
			Request_print(request, print);
		} break;
	case np_AST_Node_Type_Variable: {
			np_AST_Variable* x = (np_AST_Variable*) node->detail;
			Print_str(print, x->name.begin, x->name.end);
			Print_char(print,'\n');
			Request_print(request, print);
		} break;
	case np_AST_Node_Type_Group:
		{
			np_AST_Group* x = (np_AST_Group*) node->detail;
			Print_char(print,'\n');
			Request_print(request, print);
			print_ast(request, x->node, level+1);
		}
		break;
	case np_AST_Node_Type_Function:
		{
			np_AST_Function* x = (np_AST_Function*) node->detail;
			Print_str(print, x->name.begin, x->name.end);
			Print_char(print,'\n');
			Request_print(request, print);
			np_AST_Function_Parameter *it = x->first_parameter;
			while(it) {
				print_ast(request, it->node, level+1);
				it = it->next;
			}
		}
		break;
	case np_AST_Node_Type_Binary_Operation:
		{
			np_AST_Binary_Operation* x
				= (np_AST_Binary_Operation*) node->detail;
			Print_str(print, x->name.begin, x->name.end);
			Print_char(print,'\n');
			Request_print(request, print);
			print_ast(request, x->left, level+1);
			print_ast(request, x->right, level+1);
		}
		break;
	case np_AST_Node_Type_Assignment:
		{
			np_AST_Assignment* x
				= (np_AST_Assignment*) node->detail;
			Print_str(print, x->name.begin, x->name.end);
			Print_char(print,'\n');
			Request_print(request, print);
			print_ast(request, x->node, level+1);
		}
		break;
	default:
		{
			Assert(0 && "Unexpected!");
		}
		break;
	}
}

/*
BEGIN_DOC_STRING nanocube_ast_doc
Ver:   __VERSION__
Usage: nanocube ast FILE
Parse TEXT and generate abstract syntax tree or find syntax errors.
This grammar is used in multiple nanocube related 'scripts': querying
a nanocube, querying a snap index, setting .csv to nanocube mapping.

END_DOC_STRING
*/

internal void
service_ast(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_ast_doc);
		Request_print(request, print);
		return;
	}

	MemoryBlock filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &filename)) {
		Request_msg(request, "[ast-fail] missing filename\n");
		return;
	}

	// map file to read
	pt_MappedFile mapped_file = platform.open_mmap_file(filename.begin, filename.end, 1, 0);
	if (!mapped_file.mapped) {
		Request_msg(request, "[memory:fail] couldn't open of input file.\n");
		return;
	}


	// reserve heap memory for ast, types, and symbols
	pt_Memory parse_and_compile_buffer = platform.allocate_memory(Megabytes(128), 0, 0);

	BilinearAllocator memsrc;
	BilinearAllocator_init(&memsrc,
			       parse_and_compile_buffer.memblock.begin,
			       parse_and_compile_buffer.memblock.end);

	// setup a measure on top of the nanocube count just loaded

	// parse expression
	// char text[] = "a=dive(3);b=a(\"kind\",a);b;";

	// path

	nt_Tokenizer tok;
	np_initialize_tokenizer(&tok, mapped_file.begin, mapped_file.begin + mapped_file.size);

	np_Parser parser;
	np_Parser_init(&parser, &tok, &memsrc);

	b8 ok = np_Parser_run(&parser);

	if (ok) {
		np_AST_Node* it = parser.ast_first;
		while (it) {

			np_AST_Node *next = it->next;

			// print AST
			print_ast(request, it, 0);

			//	       // adjust precedence
			//	       it = ast_normalize(it);
			//	       it->next = next;
			//
			//	       Print_clear(print);
			//	       Print_cstr(print,"--------------------- after normalizing ast ---------------------\n");
			//	       platform.write_to_file(request->pfh_stdout, print->begin, print->end);
			//
			//	       // print AST
			//	       print_ast(request, it, 0);

			it = next;
		}
	} else {
		Print_clear(print);
		Print_cstr(print,"[Error running ast]\n");
		Request_print(request, print);
		Request_print(request, &parser.log);
	}

	platform.close_mmap_file(&mapped_file);
}

//------------------------------------------------------------------------------
// service btree
//------------------------------------------------------------------------------

//
// run unit test of a stand-alone btree
//
internal void
service_btree(Request *request)
{
	// run a btree test
	Print *print = request->print;

	pt_Memory btree_memory = platform.allocate_memory(Megabytes(32), 12, 0);  // page aligned

	al_Allocator *allocator	= al_Allocator_new(btree_memory.memblock.begin, btree_memory.memblock.end);
	al_Cache *btree_cache	= al_Allocator_create_cache(allocator, "BTree", sizeof(bt_BTree));
	bt_BTree *btree		= (bt_BTree*) al_Cache_alloc(btree_cache);

	// make allocator root pointer point to nanocube_count
	al_Allocator_set_root(allocator, btree);

	bt_BTree_init(btree, allocator);

	char *data[] = {  "Albert", "Einstein", "Carl", "Sagan", "Richard", "Feynman", "Richard", "Dawkins" };
	u64 size = sizeof(data)/sizeof(char*);
	for (u64 i=0;i<size;i+=2)
	{
		bt_BTree_insert(
				btree,
				data[i],     cstr_end(data[i]),
				data[i+1],   cstr_end(data[i+1])
			       );
	}

	Print_cstr(print, "[btree] size: ");
	Print_u64(print, btree->size);
	Print_cstr(print, "\n");
	Print_cstr(print, "[btree] hashes: ");
	Print_u64(print, btree->num_hashes);
	Print_cstr(print, "\n");
	Request_print(request,print);

	MemoryBlock mem;
	if (bt_BTree_get_value(btree, data[2], cstr_end(data[2]), &mem))
	{
		Print_clear(print);
		Print_cstr(print, "[btree] found value ");
		Print_str(print, mem.begin, mem.end);
		Print_cstr(print, " for key ");
		Print_cstr(print, data[2]);
		Print_cstr(print, ".\n");
		Request_print(request, print);
	}
	else
	{
		Print_clear(print);
		Print_cstr(print, "[btree] value for key ");
		Print_cstr(print, data[2]);
		Print_cstr(print, " NOT found.\n");
		Request_print(request, print);
	}


	bt_Iter it;
	bt_Iter_init(&it, btree);
	bt_Hash hash;
	MemoryBlock key;
	MemoryBlock value;
	while ( bt_Iter_next(&it, &hash, &key, &value) )
	{
		Print_clear(print);
		Print_u64(print, (u64) hash);
		Print_align(print, 24, 1, ' ');
		Print_str(print, key.begin, key.end);
		Print_align(print, 32, 1, ' ');
		Print_str(print, value.begin, value.end);
		Print_align(print, 32, 1, ' ');
		Print_cstr(print, "\n");
		Request_print(request, print);
	}

	platform.free_memory(&btree_memory);
}


//------------------------------------------------------------------------------
// service test
//------------------------------------------------------------------------------

internal void
test_fill_in_event(char *input, nx_Label *labels)
{
	u8 row  = (u8) input[2];
	u8 col  = (u8) input[3];
	u8 time = (u8) input[1];
	labels[0] = (u8) input[0] - (u8) 'A';
	labels[1] = (time & 0x4) ? 1 : 0;
	labels[2] = (time & 0x2) ? 1 : 0;
	labels[3] = (time & 0x1) ? 1 : 0;
	labels[4] = ((row & 0x4) ? 2 : 0) + ((col & 0x4) ? 1 : 0);
	labels[5] = ((row & 0x2) ? 2 : 0) + ((col & 0x2) ? 1 : 0);
	labels[6] = ((row & 0x1) ? 2 : 0) + ((col & 0x1) ? 1 : 0);
}

// run unit test of API calls
internal void
service_test(Request *request)
{
	Print *print = request->print;
	op_Options *options = &request->options;

	// Nanocube Cube will be hand build
	pt_Memory data_memory = platform.allocate_memory(Megabytes(32), 12, 0);  // page aligned
	pt_Memory insert_buffer_memory = platform.allocate_memory(sizeof(nx_Threads) + Megabytes(1), 12, 0);

	al_Allocator *allocator      = al_Allocator_new(data_memory.memblock.begin, data_memory.memblock.end);
	al_Cache     *nanocube_cache = al_Allocator_create_cache(allocator, "nv_Nanocube", sizeof(nv_Nanocube));
	nv_Nanocube  *nanocube       = (nv_Nanocube*) al_Cache_alloc(nanocube_cache);

	// make allocator root pointer point to nanocube_count
	al_Allocator_set_root(allocator, nanocube);

	char *names[] = { "type", "time", "location", "x", "x2" };

	nv_Nanocube_init(nanocube); // initialize nanocube (index is not initialized after this call)

	nv_Nanocube_insert_index_dimension(nanocube, 8, 1, names[0], cstr_end(names[0]));
	nv_Nanocube_insert_index_dimension(nanocube, 1, 3, names[1], cstr_end(names[1]));
	nv_Nanocube_insert_index_dimension(nanocube, 2, 3, names[2], cstr_end(names[2]));

	nv_Nanocube_insert_measure_dimension(nanocube, nv_NUMBER_STORAGE_UNSIGNED_32, names[3], cstr_end(names[3]));
	nv_Nanocube_insert_measure_dimension(nanocube, nv_NUMBER_STORAGE_UNSIGNED_32, names[4], cstr_end(names[4]));

	nv_Nanocube_init_index(nanocube, allocator);
	nv_Nanocube_init_key_value_store(nanocube, allocator);

	{ // Insert test data into cube

		nx_Label labels[1 + 3 + 3];
		nx_LabelArrayList address[3]; // some max dimensions
		address[0].next = address + 1;
		address[1].next = address + 2;
		address[2].next = 0;
		nx_Array_init(&address[0].labels, labels,         1);
		nx_Array_init(&address[1].labels, labels + 1,     3);
		nx_Array_init(&address[2].labels, labels + 3 + 1, 3);

		//
		// char *data =
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "|       | A0    |       | C7    |       | A0 B1 | A0    |       |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "|       |       | A5    |       |       |       |       | A1    |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "| D6    |       |       | C0    |       |       |       |       |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "|       |       |       |       |       | D6 B7 |       |       |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "|       | A0 B3 |       |       |       |       |       |       |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "| D3    |       |       |       |       |       | C3    |       |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "|       |       | C2    |       |       | C5    |       | B3    |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n"
		// "|       |       |       | A4 A5 |       |       |       |       |\n"
		// "|-------+-------+-------+-------+-------+-------+-------+-------|\n";

		// points
		char input[] =
		{
			'A', 0, 7, 1,
			'C', 7, 7, 3,
			'A', 0, 7, 5,
			'B', 1, 7, 5,
			'A', 0, 7, 6,
			'A', 5, 6, 2,
			'A', 1, 6, 7,
			'D', 6, 5, 0,
			'C', 0, 5, 3,
			'D', 6, 4, 5,
			'B', 7, 4, 5,
			'A', 0, 3, 1,
			'B', 3, 3, 1,
			'D', 3, 2, 0,
			'C', 3, 2, 6,
			'C', 2, 1, 2,
			'C', 5, 1, 5,
			'B', 3, 1, 7,
			'A', 4, 0, 3,
			'A', 5, 0, 3,
		};
		u64 num_points = sizeof(input) / sizeof(char) / 4;

		u32 payload[2] = { 1, 2 };

		for (u64 i=0;i<num_points;++i) {
			test_fill_in_event(input+(4*i), labels);
			nx_NanocubeIndex_insert(&nanocube->index, address, &payload, nanocube, insert_buffer_memory.memblock);
		}

	} // insert test data into cube

	if (op_Options_find_cstr(options,"-save")) {
		MemoryBlock output_filename;
		if (!op_Options_named_str_cstr(options,"-save",0,&output_filename)) {
			Request_msg(request,"usage: -save=<fname>\n");
			return;
		}
		pt_File f = platform.open_write_file(output_filename.begin, output_filename.end);
		if (!f.open) {
			Request_msg(request,"could not open file for saving\n");
			return;
		}
		platform.write_to_file(&f, (char*) allocator, (char*) allocator + allocator->used_pages * al_PAGE_SIZE);
		platform.close_file(&f);
	}

// 	/* generate .dot file */
// 	{
// 		nv_nanocube_instance = nanocube;
// 		nu_save_dot_file(&nanocube->index,"nv_nanocube.dot",
// 				nv_print_payload);
// 	}


	// @TODO(llins): extend the measure language to select
	// columns and the value part of the matrix to contain
	// multiple named columns

#if 1
	{ // run test queried and check their results

		// reserve heap memory for ast, types, and symbols
		pt_Memory parse_and_compile_buffer     = platform.allocate_memory(Megabytes(128), 0, 0);
		pt_Memory table_index_columns_buffer   = platform.allocate_memory(Megabytes(128), 0, 0);
		pt_Memory table_measure_columns_buffer = platform.allocate_memory(Megabytes(128), 0, 0);

		LinearAllocator table_index_columns_allocator;
		LinearAllocator_init(&table_index_columns_allocator,
				table_index_columns_buffer.memblock.begin,
				table_index_columns_buffer.memblock.begin,
				table_index_columns_buffer.memblock.end);

		LinearAllocator table_measure_columns_allocator;
		LinearAllocator_init(&table_measure_columns_allocator,
				table_measure_columns_buffer.memblock.begin,
				table_measure_columns_buffer.memblock.begin,
				table_measure_columns_buffer.memblock.end);

		BilinearAllocator memsrc;
		BilinearAllocator_init(&memsrc,
				parse_and_compile_buffer.memblock.begin,
				parse_and_compile_buffer.memblock.end);

		// prepare compiler
		np_Compiler compiler;
		np_Compiler_init(&compiler, &memsrc);

		// install measure related types and functions into compiler
		nv_Compiler_init(&compiler);

		// register nanocube into the default environment
		nv_Compiler_insert_nanocube_cstr(&compiler, nanocube, "events");

		// NanocubeCount specific payload aggregation table services
		nm_Services nv_payload_services;
		nv_payload_services_init(&nv_payload_services);

		char *queries[] = {
// 			"events.select(\"x\");",
// 			"events.select(\"x2\");",
// 			"events;",
// 			"events.b(\"type\",dive(1));",
// 			"x=select(\"x\");events.x.b(\"type\",dive(1))/events.x;",
// 			"x=select(\"x\");100*events.b(\"type\",dive(1)).x/events.x;",
// 			"100*events.b(\"type\",dive(1))/events;",
// 			"events.b(\"type\",p(1))/events;",
// 			"events.b(\"location\",p(2));",
// 			"events.b(\"location\",dive(3));",
// 			"events.b(\"location\",mask(\"<\"));",
// 			"events.b(\"location\",mask(\"020<<11<<<3<<\"));",
// 			"events.b(\"location\",mask(\"020<<11<<<3<<\")).b(\"time\",dive(3));",
// 			"events.b(\"time\",intseq(0,1,8));",
// 			"100*events.b(\"time\",intseq(0,1,8))/events;",
// 			"events.b(\"location\",mask(\"020<<11<<<3<<\")).b(\"time\",intseq(0,1,8));"
//			"events.b(\"type\",dive(1)).b(\"location\",mask(\"020<<11<<<3<<\")).b(\"time\",dive(3));",
			"events.b(\"location\",mask(\"223<<<331<<<011<\"));"
		};
		s32 num_queries = sizeof(queries)/sizeof(char*);

		// | 0,1,1 |   1.000000e+00
		// | 1,0,0 |   1.000000e+00
		// | 1,0,1 |   1.000000e+00
		// | 0,0,0 |   2.000000e+00
		// | 0,0,1 |   2.000000e+00
		// | 1,1,0 |   1.000000e+00
		// | 1,1,1 |   1.000000e+00

		BilinearAllocatorCheckpoint chkpt = BilinearAllocator_left_checkpoint(&memsrc);

		nv_Format format = { .format = nv_FORMAT_TEXT };
		nv_ResultStream result_stream;
		nv_ResultStream_init(&result_stream, print, format);
		nv_ResultStream_begin(&result_stream);

		nt_Tokenizer tok;
		np_Parser    parser;

		for (s32 query_index=0;query_index<num_queries;++query_index) {


			{
				Print_clear(print);
				Print_cstr(print,"--- query ");
				Print_u64(print,(u64) query_index);
				Print_cstr(print," ---\n");
				Request_print(request, print);
			}

			// @todo test table order and permuting columns

			BilinearAllocator_rewind(&memsrc, chkpt);

			char *text_begin = queries[query_index];
			char *text_end   = cstr_end(text_begin);

			/* @TODO(llins): check this */
			np_initialize_tokenizer(&tok, text_begin, text_end);
			np_Parser_init(&parser, &tok, &memsrc);

			b8 ok = np_Parser_run(&parser);

			if (!ok) continue;

			np_TypeValue last_statement = np_Compiler_reduce(&compiler, parser.ast_first, text_begin, text_end);

			if (!compiler.reduce.success) {
				Request_print(request, &compiler.reduce.log);
			}

			Assert(!last_statement.error);

			nm_Measure *measure = (nm_Measure*) last_statement.value;

			/* set the table values allocator */
			LinearAllocator_clear(&table_index_columns_allocator);
			LinearAllocator_clear(&table_measure_columns_allocator);
			s32 error = nm_OK;
			nm_Table *table_begin = nm_Measure_eval(measure, &table_index_columns_allocator, &nv_payload_services, &table_measure_columns_allocator, &error);
			nm_Table *table_end   = table_begin + 1; //  measure->num_sources;

			if (table_begin) {
				/* print tables */
				nm_Table *table = table_begin;
				// TODO(llins): too much text being pushed into Print object
				Print_clear(print);
				while (table != table_end) {

					// print result stream
					nv_ResultStream_table(&result_stream, table);

					// nv_print_table(print, table);
					++table;
				}
				Request_print(request, print);
			} else {
				Request_msg(request, nm_error_messages[error]);
			}
		}
		platform.free_memory(&table_index_columns_buffer);
		platform.free_memory(&table_measure_columns_buffer);
		platform.free_memory(&parse_and_compile_buffer);
	}
#endif

	Print_clear(print);
	Print_cstr(print, "Done testing...\n");
	platform.write_to_file
		(request->pfh_stdout, print->begin, print->end);

	// free memory
	platform.free_memory(&insert_buffer_memory);
	platform.free_memory(&data_memory);

}

//------------------------------------------------------------------------------
// http server
//------------------------------------------------------------------------------

typedef struct ServeData ServeData;

typedef struct {
	np_Compiler            compiler;
	np_CompilerCheckpoint  compiler_chkpt;
	BilinearAllocator      compiler_allocator;
	LinearAllocator        table_index_columns_allocator;
	LinearAllocator        table_value_columns_allocator;
	nt_Tokenizer           scanner;
	np_Parser              parser;
	Print                  print_result;
	Print                  print_header;
	http_Channel           http_channel;
	pt_TCP_Socket          *socket;
	nm_Services            *payload_services;
	ServeData              *context; // it is kind of the buffer parent structure
} serve_QueryBuffers;

#define app_ServeData_PAUSE_MASK 0x100000000ull
#define app_ServeData_ACTIVE_COUNT_MASK 0xFFFFFFFFull

struct ServeData {
	Request                *request;
	serve_QueryBuffers     *buffers;
	nm_Services            payload_services;
	u32                    num_buffers;


	// high 32 bits is either 0 or 1 (paused)
	// low  32 bits is the active count
	// can only increment active count if pause is off.
	volatile u64           pause_and_active_count;

	// user wanting to pause the serving system, should
	// pause the system, then wait for the active count
	// get to zero. In this way it is guaranteed that
	// all workers stopped accessing the index and it
	// can be safely replaced.
};

internal void
app_nanocube_print_http_header_default_flags(Print *print)
{
	Print_cstr(print, "Access-Control-Allow-Origin: *\r\n");
	Print_cstr(print, "Access-Control-Allow-Methods: GET\r\n");
}

internal void
app_nanocube_solve_query(MemoryBlock text, serve_QueryBuffers *buffers)
{
	// pf_BEGIN_BLOCK("app_nanocube_solve_query");

	Print     *print_result = &buffers->print_result;
	Print     *print_header = &buffers->print_header;
	Print_clear(print_result);
	Print_clear(print_header);

	// clear previous compiler memory usage
	np_Compiler_goto_checkpoint(&buffers->compiler, buffers->compiler_chkpt);
	np_Compiler_clear_error_log(&buffers->compiler);

	// pf_BEGIN_BLOCK("init_scanner");
	nt_Tokenizer_reset_text(&buffers->scanner, text.begin, text.end);
	// pf_END_BLOCK();

	// pf_BEGIN_BLOCK("init_parser");
	np_Parser_reset(&buffers->parser);
	// pf_END_BLOCK();

	// pf_BEGIN_BLOCK("np_Parser_run");
	b8 ok = np_Parser_run(&buffers->parser);
	// pf_END_BLOCK();

	if (!ok) {
		Print_cstr(print_result, "Syntax Error on Query\n");
		Print_str(print_result, buffers->parser.log.begin, buffers->parser.log.end);

		Print_cstr(print_header, "HTTP/1.1 400 Syntax Error\r\n");
		app_nanocube_print_http_header_default_flags(print_header);
		Print_cstr(print_header, "Content-Type: text/plain\r\n");
		Print_format(print_header, "Content-Length: %lld\r\n", Print_length(print_result));
		Print_cstr(print_header, "\r\n");
		goto done;
	}

	// pf_BEGIN_BLOCK("np_Compiler_reduce");
	np_Compiler_reduce(&buffers->compiler, buffers->parser.ast_first, text.begin, text.end );
	// pf_END_BLOCK();

	if (!buffers->compiler.reduce.success) {
		Print_cstr(print_result, "Compiler Error on Query\n");
		Print_str(print_result, buffers->compiler.reduce.log.begin, buffers->compiler.reduce.log.end);

		Print_cstr(print_header, "HTTP/1.1 400 Syntax Error\r\n");
		app_nanocube_print_http_header_default_flags(print_header);
		Print_cstr(print_header, "Content-Type: text/plain\r\n");
		Print_format(print_header, "Content-Length: %lld\r\n", Print_length(print_result));
		Print_cstr(print_header, "\r\n");
		goto done;
	}

	// find format used (default will be JSON)
	nv_Format format = { .format = nv_FORMAT_JSON };
	{
		b8 format_used = 0;
		np_TypeValueList *it = buffers->compiler.reduce.statement_results;
		while (it) {
			// once a format has been used, we stick to it
			if (it->data.type_id == nv_compiler_types.format) {
				if (format_used == 0) {
					format.format = ((nv_Format*) it->data.value)->format;
				}
				// TODO(llins): include warning if format changes before or after usage
			} else {
				// assuming anythong not format generated output
				format_used = 1;
			}
			it = it->next;
		}
	}

	nv_ResultStream result_stream;
	nv_ResultStream_init(&result_stream, print_result, format);
	nv_ResultStream_begin(&result_stream);

	// loop through all the compiled stuff and evaluate all the queries
	b8 format_used = 0;
	s32 evaluated_statements = 0;
	np_TypeValueList *it = buffers->compiler.reduce.statement_results;

	// @todo: add a check here that if format is binary allow
	// only for one table value element

	if (!it) {
		// if nothing came up in the query solve using the API documentation text.
		Print_cstr(print_result, nanocube_api_doc);
	}

	while (it) {
		// once a format has been used, we stick to it
		if (it->data.type_id == nv_compiler_types.schema) {

			// loop through all the sources to print their schema
			np_Symbol *symbol = buffers->compiler.symbol_table.begin;
			while (symbol != buffers->compiler.symbol_table.end) {
				if (symbol->is_variable && symbol->variable.type_id == nv_compiler_types.measure) {
					// all pre-stored measures
					nm_Measure *measure = (nm_Measure*) symbol->variable.value;
					if (measure->expression->is_source && measure->num_sources == 1) {
						nm_MeasureSource *source = measure->sources[0];
						Assert(source);
						Assert(source->num_nanocubes > 0);
						nv_Nanocube *nanocube = (nv_Nanocube*) source->nanocubes[source->num_nanocubes-1];
						nv_ResultStream_schema(&result_stream, symbol->name, nanocube);
					}
				}
				++symbol;
			}
		}  else if (it->data.type_id == nv_compiler_types.query) {
			//
			// solve query
			//

			nv_Query   *query   = (nv_Query*)   it->data.value;
			nm_Measure *measure = query->measure;

			/* reset pre-reserved memory for preparing response */
			LinearAllocator_clear(&buffers->table_index_columns_allocator);
			LinearAllocator_clear(&buffers->table_value_columns_allocator);

			// pf_BEGIN_BLOCK("nm_Measure_eval");
			s32 error = nm_OK;
			nm_Table *table = nm_Measure_eval(measure, &buffers->table_index_columns_allocator, buffers->payload_services, &buffers->table_value_columns_allocator, &error);
			// pf_END_BLOCK();

			if (table == 0) {
				Print_cstr(print_result, nm_error_messages[error]);
				Print_cstr(print_header, "HTTP/1.1 400 Syntax Error\r\n");
				app_nanocube_print_http_header_default_flags(print_header);
				Print_cstr(print_header, "Content-Type: text/plain\r\n");
				Print_format(print_header, "Content-Length: %lld\r\n", Print_length(print_result));
				Print_cstr(print_header, "\r\n");
				goto done;
			}

			// pf_BEGIN_BLOCK("write_query_result");

			// write down result in memory
			nv_ResultStream_table(&result_stream, table);

			// pf_END_BLOCK();

		} else if (it->data.type_id == nv_compiler_types.version) {

			nv_ResultStream_version(&result_stream, (char*) nanocube_api_version_doc, (char*) nanocube_executable_version_doc);

		}

		it = it->next;
	}

	nv_ResultStream_end(&result_stream);

// 	if (evaluated_statements == 0) {
// 		Print_cstr(print_result, nanocube_api_doc);
// 		Print_cstr(print_header, "HTTP/1.1 200 OK\r\n");
// 		Print_cstr(print_header, "Content-Type: text/plain\r\n");
// 		Print_format(print_header, "Content-Length: %lld\r\n", Print_length(print_result));
// 		Print_cstr(print_header, "\r\n");
// 	} else {
		Print_cstr(print_header, "HTTP/1.1 200 OK\r\n");
		app_nanocube_print_http_header_default_flags(print_header);
		switch(format.format) {
		case nv_FORMAT_JSON: {
			Print_cstr(print_header, "Content-Type: application/json\r\n");
		} break;
		case nv_FORMAT_TEXT: {
			Print_cstr(print_header, "Content-Type: text/plain\r\n");
		} break;
		case nv_FORMAT_BINARY: {
			Print_cstr(print_header, "Content-Type: application/octet-stream\r\n");
		} break;
		}
		Print_cstr(print_header, "Content-Length: ");
		Print_u64(print_header, Print_length(print_result));
		Print_cstr(print_header, "\r\n\r\n");
// 	}

done:
	return;
	// pf_END_BLOCK();


}

// #define http_REQUEST_LINE_CALLBACK(name)
// 	void name(http_Channel *channel,
//  	          http_Response *response,
// 		  char *method_begin, char *method_end,
// 		  char *request_target_begin, char *request_target_end,
// 		  char *http_version_begin, char *http_version_end)
internal
http_REQUEST_LINE_CALLBACK(service_serve_http_request_line_callback)
{
	serve_QueryBuffers *buffers = (serve_QueryBuffers*) channel->user_data;

	Print *print_result = &buffers->print_result;
	Print *print_header = &buffers->print_header;
	Print_clear(print_result);
	Print_clear(print_header);

	pt_TCP_Socket *socket = buffers->socket;

	/* check if method is GET */
	if (pt_compare_memory_cstr(method_begin, method_end, "GET")) {
		Print_cstr(print_result, "Invalid Method\n");

		Print_cstr(print_header, "HTTP/1.1 400 Syntax Error\r\n");
		app_nanocube_print_http_header_default_flags(print_header);
		Print_cstr(print_header, "Content-Type: text/plain\r\n");
		Print_format(print_header, "Content-Length: %lld\r\n", Print_length(print_result));
		Print_cstr(print_header, "\r\n");
		goto done;
	}

	// http request-target starts with '/'
	// consider the remaining of the request-target the query
	if (request_target_begin == request_target_end || *request_target_begin != '/') {
		Print_cstr(print_result, "Expecting request_target to start with '/'\n");

		Print_cstr(print_header, "HTTP/1.1 400 Syntax Error\r\n");
		app_nanocube_print_http_header_default_flags(print_header);
		Print_cstr(print_header, "Content-Type: text/plain\r\n");
		Print_format(print_header, "Content-Length: %lld\r\n", Print_length(print_result));
		Print_cstr(print_header, "\r\n");
		goto done;
	}

	MemoryBlock text = { .begin = request_target_begin + 1, .end = request_target_end };
	text = ut_convert_uri_to_ascii(text);

	ServeData *serve_data = buffers->context;
	if (serve_data) {
		// sync mechanism:
		// try incrementing serve_data->pause_and_active_count
		for (;;) {
			u64 pause_and_active_count = serve_data->pause_and_active_count;
			if ((pause_and_active_count & app_ServeData_PAUSE_MASK) != 0) {
				// service is paused sleep for a while and then try again
				platform.thread_sleep(1);
			} else {
				// try swaping and incrementing
				u64 what_was_there = pt_atomic_cmp_and_swap_u64(&serve_data->pause_and_active_count, pause_and_active_count, pause_and_active_count + 1);
				if (pause_and_active_count == what_was_there) {
					// we are in: our update went through
					break;
				}
			}
		}
	}

	app_nanocube_solve_query(text, buffers);

	if (serve_data) {
		// regardless if it is paused or not, the active count is going to
		// be decremented by one and a correct count of events is in play
		pt_atomic_sub_u64(&serve_data->pause_and_active_count, 1);
	}

done:
	/* write down result on tcp port */
	platform.tcp_write(socket, print_header->begin, Print_length(print_header));
	platform.tcp_write(socket, print_result->begin, Print_length(print_result));

}

// #define http_HEADER_FIELD_CALLBACK(name)
// 	void name(http_Channel *channel,
// 	          http_Response *response,
// 		  char *field_name_begin, char *field_name_end,
// 		  char *field_value_begin, char *field_value_end)
internal
http_HEADER_FIELD_CALLBACK(service_serve_http_header_field_callback)
{
	/* print request_target */
// 	Print *print = &debug_request->print;
// 	Print_clear(print);
// 	Print_cstr(print, "[service_http_header_field_callback]: header field is key:'");
// 	Print_str(print, field_name_begin, field_name_end);
// 	Print_cstr(print, "', value:'");
// 	Print_str(print, field_value_begin, field_value_end);
// 	Print_cstr(print, "'\n");
// 	Request_print(debug_request, print);
}

// (pt_TCP_Socket *socket, char *begin, char *end)
// typedef void PlatformTCPCallback(pt_TCP_Socket *socket, char *buffer, u64 length);
internal
PLATFORM_TCP_CALLBACK(serve_request_handler)
{

	/* assuming single threaded model for the moment */
	ServeData *serve_data = (ServeData*) socket->user_data;

	// TODO(llins): get thread index
	u64 index = platform.get_thread_index();

	serve_QueryBuffers *buffers = serve_data->buffers + index;
	buffers->socket = socket;


	// push data into http channel
	http_Channel_receive_data(&buffers->http_channel, buffer, length);

}

// #if 1
// /* memory in megabytes for solving query */
// #define app_service_serve_MEM_COMPILER            Megabytes(32)
// #define app_service_serve_MEM_PRINT_RESULT        Megabytes(64)
// #define app_service_serve_MEM_PRINT_HEADER        Kilobytes(4)
// #define app_service_serve_MEM_TABLE_INDEX_COLUMNS Megabytes(64)
// #define app_service_serve_MEM_TABLE_VALUE_COLUMNS Megabytes(64)
// /* largest GET query is bounded by http channel memory */
// #define app_service_serve_MEM_HTTP_CHANNEL        Megabytes(4)
// #else
// /* memory in megabytes for solving query */
// #define app_service_serve_MEM_COMPILER            Megabytes(16)
// #define app_service_serve_MEM_PRINT_RESULT        Megabytes(32)
// #define app_service_serve_MEM_PRINT_HEADER        Kilobytes(4)
// #define app_service_serve_MEM_TABLE_INDEX_COLUMNS Megabytes(32)
// #define app_service_serve_MEM_TABLE_VALUE_COLUMNS Megabytes(32)
// /* largest GET query is bounded by http channel memory */
// #define app_service_serve_MEM_HTTP_CHANNEL        Megabytes(4)
// #endif


#define app_NanocubesAndAliases_OK 1
#define app_NanocubesAndAliases_EMPTY_ALIAS 2
#define app_NanocubesAndAliases_COULDNT_OPEN_NANOCUBE_FILE 3
#define app_NanocubesAndAliases_NOT_ENOUGH_MEMORY 4
#define app_NanocubesAndAliases_COULD_NOT_MMAP_FILE 5
#define app_NanocubesAndAliases_NO_NANOCUBE_FOR_ALIAS 6

typedef struct {
	BasicAllocator blocks;
	pt_MappedFile  mapped_files[1024];
	nv_Nanocube    *nanocubes[64];
	MemoryBlock    aliases[64];
	u32            num_nanocubes;
	u32            num_mapped_files;
	u8             parse_result;
} app_NanocubesAndAliases;

internal void
app_NanocubesAndAliases_init(app_NanocubesAndAliases *self)
{
	BasicAllocator_init(&self->blocks);
	self->num_mapped_files = 0;
	self->num_nanocubes = 0;
	self->parse_result = app_NanocubesAndAliases_OK;
}

// internal void
// app_NanocubesAndAliases_free_nanocubes(app_NanocubesAndAliases *self)
// {
// 	// assuming nanocubes came from blocks and not from mapped files
// 	Assert(self->num_mapped_files == 0);
// 	for (s32 i=0;i<self->num_nanocubes;++i) {
// 		BasicAllocator_free(&self->blocks, self->nanocubes[i]);
// 	}
// 	self->num_nanocubes = 0;
// }

internal void
app_NanocubesAndAliases_free_and_pop_nanocube(app_NanocubesAndAliases *self, s32 index)
{
	Assert(index < self->num_nanocubes);

	// I got to free the allocator
	al_Allocator *allocator = nv_Nanocube_get_allocator(self->nanocubes[index]);
	BasicAllocator_free(&self->blocks, allocator); // self->nanocubes[index]);
	for (s32 i=index+1;i<self->num_nanocubes;++i) {
		self->nanocubes[i-1] = self->nanocubes[i];
		self->aliases[i-1] = self->aliases[i];
	}
	--self->num_nanocubes;
}

internal void
app_NanocubesAndAliases_free_resources(app_NanocubesAndAliases *self)
{
	BasicAllocator_free_all(&self->blocks);
	for (s32 i=0;i<self->num_mapped_files;++i) {
		platform.close_mmap_file(self->mapped_files + i);
	}
}

// copies the whole allocator associated with the nanocube
internal b8
app_NanocubesAndAliases_copy_and_register_nanocube(app_NanocubesAndAliases *self, nv_Nanocube *nanocube, char *alias_begin, char *alias_end)
{
	al_Allocator *allocator = nv_Nanocube_get_allocator(nanocube);
	u64 length = (u64) allocator->used_pages * al_PAGE_SIZE;
	void *buffer = BasicAllocator_alloc(&self->blocks, length, al_BITS_PER_PAGE);
	if (buffer == 0) {
		// not enough memory
		return 0;
	}
	platform.copy_memory(buffer, allocator, length);
	al_Allocator *allocator_copy = (al_Allocator*) buffer;

	nv_Nanocube* cube = (nv_Nanocube*) al_Allocator_get_root(allocator_copy);
	self->nanocubes[self->num_nanocubes] = cube;
	self->aliases[self->num_nanocubes]   = (MemoryBlock) { .begin=alias_begin, .end=alias_end };
	++self->num_nanocubes;
	return 1;
}

internal u8
app_NanocubesAndAliases_parse_and_load_alias(app_NanocubesAndAliases *self, char *text_begin, char *text_end, Print *log)
{
	Assert(log);

	MemoryBlock source_text = {.begin = text_begin, .end = text_end };
	char *eq = pt_find_char(source_text.begin, source_text.end, '=');
	b8 alias = eq != source_text.end;
	char *alias_begin = source_text.begin;
	char *alias_end   = eq;
	s32 nanocubes_associated_with_this_alias = 0;
	if (!alias || alias_begin == alias_end) {
		Print_cstr(log, "[serve] Error: empty alias. Use an alias name for .nanocube index files");
		self->parse_result = app_NanocubesAndAliases_EMPTY_ALIAS;
		return self->parse_result;
	}

	nt_Tokenizer tok;
	char sep[] = ":";
	nt_Tokenizer_init_canonical(&tok, sep, cstr_end(sep));
	nt_Tokenizer_reset_text(&tok, eq+1, source_text.end);
	s32 tok_number = -1;
	while (nt_Tokenizer_next(&tok)) {
		++tok_number;
		if (tok_number > 0) {
			Print_char(log,'\n');
		}

		char *filename_begin = tok.token.begin;
		char *filename_end   = tok.token.end;
		if (filename_begin == filename_end) {
			continue;
		}
		b8 cache = 0;
		if (*filename_begin == '@') {
			cache = 1;
			++filename_begin;
		}
		if (filename_begin == filename_end) {
			continue;
		}
		char *block_begin = 0;
		if (cache) {
			pt_File nanocube_file = platform.open_read_file(filename_begin, filename_end);
			if (!nanocube_file.open) {
				Print_cstr(log, "couldn't open file: ");
				Print_str(log, filename_begin, filename_end);
				Print_char(log,'\n');
				self->parse_result = app_NanocubesAndAliases_COULDNT_OPEN_NANOCUBE_FILE;
				return self->parse_result;
			}
			/* bring all file content to memory */

			block_begin = BasicAllocator_alloc(&self->blocks, nanocube_file.size, 12);
			if (!block_begin) {
				Print_cstr(log, "couldn't read file: ");
				Print_str(log, filename_begin, filename_end);
				Print_char(log,'\n');
				self->parse_result = app_NanocubesAndAliases_NOT_ENOUGH_MEMORY;
				return self->parse_result;
			}
			platform.read_next_file_chunk(&nanocube_file, block_begin, block_begin + nanocube_file.size);
			Assert(nanocube_file.last_read == nanocube_file.size);
			platform.close_file(&nanocube_file);
		} else {
			/* mmap file */
			Assert(self->num_mapped_files < ArrayCount(self->mapped_files));
			self->mapped_files[self->num_mapped_files] = platform.open_mmap_file(filename_begin, filename_end, 1, 0);
			pt_MappedFile *mapped_file = self->mapped_files + self->num_mapped_files;
			if (!mapped_file->mapped) {
				Print_cstr(log, "[serve] couldn't memory map file: ");
				Print_str(log, filename_begin, filename_end);
				Print_char(log,'\n');
				self->parse_result = app_NanocubesAndAliases_COULD_NOT_MMAP_FILE;
				return self->parse_result;
			}
			++self->num_mapped_files;
			block_begin = mapped_file->begin;
		}
		/* messge */
		Print_cstr(log, "[serve] Alias \"");
		Print_str(log, alias_begin, alias_end);
		if (cache) {
			Print_cstr(log, "\" file copied in memory: ");
		} else {
			Print_cstr(log, "\" memory mapped file: ");
		}
		Print_str(log, filename_begin, filename_end);
		// Print_cstr(log, "\n");
		al_Allocator* allocator = (al_Allocator*)  block_begin;
		/* TODO(llins): run some sanity check to see if content of mapped file seems valid */
		nv_Nanocube* cube = (nv_Nanocube*)   al_Allocator_get_root(allocator);

		Assert(self->num_nanocubes < ArrayCount(self->nanocubes) - 1);
		self->aliases[self->num_nanocubes].begin = alias_begin;
		self->aliases[self->num_nanocubes].end   = alias_end;
		self->nanocubes[self->num_nanocubes] = cube;
		++self->num_nanocubes;
		++nanocubes_associated_with_this_alias;
	}

	Assert(nanocubes_associated_with_this_alias > 0);
// 	if (nanocubes_associated_with_this_alias == 0) {
// 				Print_cstr(print, "no nanocube for alias");
// 				self->parse_result.status = app_NanocubesAndAliases_NO_NANOCUBE_FOR_ALIAS;
// 				return self->parse_result.status;
// 	}
	self->parse_result = app_NanocubesAndAliases_OK;
	return self->parse_result;
}



/*
BEGIN_DOC_STRING nanocube_executable_version_doc
__VERSION__
END_DOC_STRING
*/

/*
BEGIN_DOC_STRING nanocube_api_version_doc
0.4
END_DOC_STRING
*/


/*
BEGIN_DOC_STRING nanocube_serve_doc
Ver:   __VERSION__
Usage: nanocube serve PORT *(ALIAS=([@]NANOCUBE_FILE)*(,[@]NANOCUBE_FILE))
Start an http server on port PORT and serve one or more nanocube
indices using the given ALIAS names. The same ALIAS can refer to multiple
nanocube index files. The assumption in this case is that they all have
the same schema. The '@' symbol before a nanocube file names indicates
we want to force the server process to load that indicex in memory.
When the '@' symbol is not used, we simply memory map the file and
hope for the best.

Possible OPTIONS:

    -threads=N
         threads used to process http requests. If single threaded,
	 the same thread that establishes connections is the one
	 processing and responding client requests. If more than
	 one thread is used, one thread is responsible for establishing
	 connections while the other threads process incoming requests.
	 At this moment, multiple requests might be processed in parallel
	 by different threads, but each request is processed sequentially.

    -mem_compiler=SIZE                  (default  8M)
    -mem_print_result=SIZE              (default 64M)
    -mem_print_header=SIZE              (default  4K)
    -mem_table_index_columns=SIZE       (default 64M)
    -mem_table_value_columns=SIZE       (default 64M)
    -mem_http_channel=SIZE              (default  4M)
    -mem_table_value=SIZE               (default  8M)
         Maximum memory sizes used by nanocube. On overflow report error.

END_DOC_STRING
*/

// run unit test of api calls
internal void
service_serve(Request *request)
{
	// usage:
	//	   serve <port> <alias1>=<db1> <alias2>=<db2> ... <aliasn>=<dbn>
	//	   serve 8000 crimes.ncc mts.ncc weather.ncc

	//
	// Use platform to find .ncc files in <db>
	// folder. Memory map all ncc files. Use flag
	// to indicate if we should bring .ncc files
	// to memory to eanble "interactivity right away".
	// maybe not necessary. Give it a shot without
	// it.
	//
	Print        *print   = request->print;
	op_Options   *options = &request->options;

	// get next two tokens

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_serve_doc);
		Request_print(request, print);
		return;
	}

	s32 port = 0;
	if (!op_Options_s32(options, 1, &port)) {
		Request_msg(request, "[serve] could not parse port number.\n");
		return;
	}

	u32 num_parameters = op_Options_num_positioned_parameters(options);
	if (num_parameters < 3) {
		Request_msg(request, "[serve] requires at least one source.\n");
		Request_msg(request, "[serve] usage: nanocube serve <port> (<src>)+.\n");
		return;
	}

	u64 num_threads = 0;
	if (op_Options_find_cstr(options,"-threads")) {
		if (!op_Options_named_u64_cstr(options,"-threads",0,&num_threads)) {
			Request_msg(request, "[serve] incorrect usage of options: -threads=<num-threads>\n");
			return;
		}
	}

	{
		// memory limits
		u64 mem_limit = 0;

#define app_MEM_OPTION(name,variable) \
		if (op_Options_find_cstr(options,name)) { if (!op_Options_named_num_bytes_cstr(options,name,0,&variable)) {  \
			fprintf(stderr,"[serve] invalid memory limit: %s\n", name); return; \
		} }

		app_MEM_OPTION("-mem_compiler",            app_service_serve_MEM_COMPILER);
		app_MEM_OPTION("-mem_print_result",        app_service_serve_MEM_PRINT_RESULT);
		app_MEM_OPTION("-mem_print_header",        app_service_serve_MEM_PRINT_HEADER);
		app_MEM_OPTION("-mem_table_index_columns", app_service_serve_MEM_TABLE_INDEX_COLUMNS);
		app_MEM_OPTION("-mem_table_value_columns", app_service_serve_MEM_TABLE_VALUE_COLUMNS);
		app_MEM_OPTION("-mem_http_channel",        app_service_serve_MEM_HTTP_CHANNEL);
		app_MEM_OPTION("-mem_table_value",         nv_TABLE_VALUE_MAX_SIZE);
#undef app_MEM_OPTION

	}

	Print_clear(print);
	Print_format(print,"Ver: %s\n",nanocube_executable_version_doc);
	Request_print(request, print);
	Print_clear(print);

	app_NanocubesAndAliases info;
	app_NanocubesAndAliases_init(&info);

	b8 ok = 1;

	//
	// Reuse this block in both 'serve' and 'query'. Note that we end
	// up with a set of nanocubes, aliases memory mapped files, in memory
	// files etc
	//
	MemoryBlock source_text = {.begin=0, .end=0};
	for (u32 param=2;param<num_parameters;++param) {
		op_Options_str(options, param, &source_text);
		Print_clear(print);
		u8 status = app_NanocubesAndAliases_parse_and_load_alias(&info, source_text.begin, source_text.end, print);
		Request_print(request, print);
		Request_msg(request, "\n");
		if (status != app_NanocubesAndAliases_OK) {
			ok = 0;
			break;
		}
	}

	if (!ok) {
		goto free_resources;
	}

	if (num_threads < 1) {
		num_threads = 1;
	}

	ServeData serve_data = {
		.buffers = (serve_QueryBuffers*) BasicAllocator_alloc(&info.blocks, num_threads * sizeof(serve_QueryBuffers),3),
		.request = request,
		.num_buffers = num_threads,
		.pause_and_active_count = 0
	};
	Assert(serve_data.buffers);
	nv_payload_services_init(&serve_data.payload_services);

	for (u32 i=0;i<num_threads;++i) {

		serve_QueryBuffers *buffer = serve_data.buffers + i;
		buffer->context = &serve_data;

		// reserve heap memory for ast, types, and symbols
		MemoryBlock parse_and_compile_buffer   = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_COMPILER,            3);
		MemoryBlock table_index_columns_buffer = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_TABLE_INDEX_COLUMNS, 3);
		MemoryBlock table_value_columns_buffer = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_TABLE_VALUE_COLUMNS, 3);
		MemoryBlock print_result_buffer        = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_PRINT_RESULT,        3);
		MemoryBlock print_header_buffer        = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_PRINT_HEADER,        3);
		MemoryBlock http_channel_buffer        = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_HTTP_CHANNEL,        3);

		/* initialize compiler allocator */
		BilinearAllocator_init(&buffer->compiler_allocator,
				       parse_and_compile_buffer.begin,
				       parse_and_compile_buffer.end);
		/* init compiler */
		np_Compiler_init(&buffer->compiler, &buffer->compiler_allocator);
		nv_Compiler_init(&buffer->compiler);

		/* init table_index_columns_allocator */
		LinearAllocator_init(&buffer->table_index_columns_allocator,
				     table_index_columns_buffer.begin,
				     table_index_columns_buffer.begin,
				     table_index_columns_buffer.end);

		/* init table_value_columns_allocator*/
		LinearAllocator_init(&buffer->table_value_columns_allocator,
				     table_value_columns_buffer.begin,
				     table_value_columns_buffer.begin,
				     table_value_columns_buffer.end);

		/* init print_result */
		Print_init(&buffer->print_result,
			   print_result_buffer.begin,
			   print_result_buffer.end);

		/* init print_result */
		Print_init(&buffer->print_header,
			   print_header_buffer.begin,
			   print_header_buffer.end);

		/* insert nanocube reference aliases into compiler global environment */
		for (s32 j=0;j<info.num_nanocubes;++j) {
			nv_Compiler_insert_nanocube(&buffer->compiler, info.nanocubes[j], info.aliases[j].begin, info.aliases[j].end);
		}

		/* initialize parser table */
		np_initialize_tokenizer(&buffer->scanner, 0, 0);
		np_Parser_init(&buffer->parser, &buffer->scanner, buffer->compiler.memory);

		// mark checkpoint on compiler as the fresh
		// starting point for new compilations
		buffer->compiler_chkpt = np_Compiler_checkpoint(&buffer->compiler);

		/* initialize http channel memory */
		http_Channel_init(&buffer->http_channel,
				  http_channel_buffer.begin,
				  http_channel_buffer.end,
				  service_serve_http_request_line_callback,
				  service_serve_http_header_field_callback,
				  buffer); /* user data will be the buffer*/

		/* initialize socket */
		buffer->socket = 0;
		buffer->payload_services = &serve_data.payload_services;
	}

	pt_TCP tcp = platform.tcp_create();
	Assert(tcp.handle);

	// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port, void *user_data, PlatformTCPCallback *callback, u32 *status)
	pt_TCP_Feedback feedback;
	platform.tcp_serve(tcp, port, (void*) &serve_data, serve_request_handler, &feedback);

	// should now be available
	platform.tcp_process_events(tcp, 0);

	if (feedback.status != pt_TCP_SOCKET_OK) {
		return;
	}


	// log message to differentiate from sequential server
	Print_clear(print);
	Print_format(print, "[serve] port: %d\n", port);
	Request_print(request, print);

	pt_WorkQueue *work_queue = 0;
	if (num_threads > 1) {
		work_queue = platform.work_queue_create(num_threads);
		// log message to differentiate from sequential server
		Print_clear(print);
		Print_cstr(print, "[serve] using ");
		Print_u64(print, num_threads);
		Print_cstr(print, " threads\n");
		Request_print(request, print);
	}

#ifdef PROFILE
	pf_begin();
#endif

	while (!global_app_state->interrupted) {

		platform.tcp_process_events(tcp, work_queue);

	}

	/* TODO(llins): somehow there should be a way to signal the server to stop */
	platform.work_queue_destroy(work_queue);

	// cleanup tcp
	platform.tcp_destroy(tcp);

#ifdef PROFILE
	// pf_generate_log_events();
	pf_generate_report();
	Request_print(request, &pf_report.print);
	pf_end();

	{
		u64 count = pf_Table_current_events_count(global_profile_table);
		u64 capacity = global_profile_table->event_capacity;

		Print_clear(print);
		Print_cstr(print, "Profile Events (Count/Capacity/Usage): ");
		Print_u64(print, count);
		Print_char(print, ' ');
		Print_u64(print, capacity);
		Print_char(print, ' ');
		Print_u64(print, (100 * count)/capacity);
		Print_char(print, '%');
		Print_char(print, '\n');
		Request_print(request, print);
	}
#endif

free_resources:

	/* free blocks */
	app_NanocubesAndAliases_free_resources(&info);

}

/*
 * Service Query
 */

/*
BEGIN_DOC_STRING nanocube_query_doc
Ver:   __VERSION__
Usage: nanocube query QUERY *(ALIAS=([@]NANOCUBE_FILE)*(,[@]NANOCUBE_FILE))
Run QUERY on the indices being linked by the ALIASes.

    -mem_compiler=SIZE                  (default  8M)
    -mem_print_result=SIZE              (default 64M)
    -mem_print_header=SIZE              (default  4K)
    -mem_table_index_columns=SIZE       (default 64M)
    -mem_table_value_columns=SIZE       (default 64M)
    -mem_table_value=SIZE               (default  8M)
         Maximum memory sizes used by nanocube. On overflow report error.

END_DOC_STRING
*/

// run unit test of api calls
internal void
service_query(Request *request)
{
	// usage:
	//	   serve <port> <alias1>=<db1> <alias2>=<db2> ... <aliasn>=<dbn>
	//	   serve 8000 crimes.ncc mts.ncc weather.ncc

	//
	// Use platform to find .ncc files in <db>
	// folder. Memory map all ncc files. Use flag
	// to indicate if we should bring .ncc files
	// to memory to eanble "interactivity right away".
	// maybe not necessary. Give it a shot without
	// it.
	//
	Print        *print   = request->print;
	op_Options   *options = &request->options;

	// get next two tokens
	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_query_doc);
		Request_print(request, print);
		return;
	}

	{
		// memory limits
		u64 mem_limit = 0;

#define app_MEM_OPTION(name,variable) \
		if (op_Options_find_cstr(options,name)) { if (!op_Options_named_num_bytes_cstr(options,name,0,&variable)) {  \
			fprintf(stderr,"[serve] invalid memory limit: %s\n", name); return; \
		} }

		app_MEM_OPTION("-mem_compiler",            app_service_serve_MEM_COMPILER);
		app_MEM_OPTION("-mem_print_result",        app_service_serve_MEM_PRINT_RESULT);
		app_MEM_OPTION("-mem_print_header",        app_service_serve_MEM_PRINT_HEADER);
		app_MEM_OPTION("-mem_table_index_columns", app_service_serve_MEM_TABLE_INDEX_COLUMNS);
		app_MEM_OPTION("-mem_table_value_columns", app_service_serve_MEM_TABLE_VALUE_COLUMNS);
		app_MEM_OPTION("-mem_table_value",         nv_TABLE_VALUE_MAX_SIZE);

#undef app_MEM_OPTION

	}



	u32 num_parameters = op_Options_num_positioned_parameters(options);
	if (num_parameters < 3) {
		Request_msg(request, "[query] requires at least one source.\n");
		Request_msg(request, "[query] usage: nanocube query <query-fname> (<src>)+.\n");
		return;
	}


	MemoryBlock query  = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &query)) {
		Request_msg(request, "[create] not enough input parameters.\n");
		Request_msg(request, "[create] usage: nanocube draw <input> <output>.\n");
		return;
	}

	app_NanocubesAndAliases info;
	app_NanocubesAndAliases_init(&info);

	//
	// Reuse this block in both 'serve' and 'query'. Note that we end
	// up with a set of nanocubes, aliases memory mapped files, in memory
	// files etc
	//
	b8 ok = 1;
	MemoryBlock source_text = {.begin=0, .end=0};
	for (u32 param=2;param<num_parameters;++param) {
		op_Options_str(options, param, &source_text);
		Print_clear(print);
		u8 status = app_NanocubesAndAliases_parse_and_load_alias(&info, source_text.begin, source_text.end, print);
		Request_print(request, print);
		Request_msg(request, "\n");
		if (status != app_NanocubesAndAliases_OK) {
			ok = 0;
			break;
		}
	}

	if (!ok)
		goto free_resources;

	nm_Services nv_payload_services;
	nv_payload_services_init(&nv_payload_services);

	serve_QueryBuffers buffer_storage;
	serve_QueryBuffers *buffer = &buffer_storage;
	buffer->context = 0;

	// reserve heap memory for ast, types, and symbols
	MemoryBlock parse_and_compile_buffer   = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_COMPILER,            3);
	MemoryBlock table_index_columns_buffer = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_TABLE_INDEX_COLUMNS, 3);
	MemoryBlock table_value_columns_buffer = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_TABLE_VALUE_COLUMNS, 3);
	MemoryBlock print_result_buffer        = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_PRINT_RESULT,        3);
	MemoryBlock print_header_buffer        = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_PRINT_HEADER,        3);
	MemoryBlock http_channel_buffer        = BasicAllocator_alloc_memblock(&info.blocks, app_service_serve_MEM_HTTP_CHANNEL,        3);

	/* initialize compiler allocator */
	BilinearAllocator_init(&buffer->compiler_allocator,
			       parse_and_compile_buffer.begin,
			       parse_and_compile_buffer.end);
	/* init compiler */
	np_Compiler_init(&buffer->compiler, &buffer->compiler_allocator);
	nv_Compiler_init(&buffer->compiler);

	/* init table_index_columns_allocator */
	LinearAllocator_init(&buffer->table_index_columns_allocator,
			     table_index_columns_buffer.begin,
			     table_index_columns_buffer.begin,
			     table_index_columns_buffer.end);

	/* init table_value_columns_allocator*/
	LinearAllocator_init(&buffer->table_value_columns_allocator,
			     table_value_columns_buffer.begin,
			     table_value_columns_buffer.begin,
			     table_value_columns_buffer.end);

	/* init print_result */
	Print_init(&buffer->print_result,
		   print_result_buffer.begin,
		   print_result_buffer.end);

	/* init print_result */
	Print_init(&buffer->print_header,
		   print_header_buffer.begin,
		   print_header_buffer.end);

	/* insert nanocube reference aliases into compiler global environment */
	for (s32 j=0;j<info.num_nanocubes;++j) {
		nv_Compiler_insert_nanocube(&buffer->compiler, info.nanocubes[j], info.aliases[j].begin, info.aliases[j].end);
	}

	/* initialize parser table */
	np_initialize_tokenizer(&buffer->scanner, 0, 0);
	np_Parser_init(&buffer->parser, &buffer->scanner, buffer->compiler.memory);

	// mark checkpoint on compiler as the fresh
	// starting point for new compilations
	buffer->compiler_chkpt = np_Compiler_checkpoint(&buffer->compiler);

	buffer->payload_services = &nv_payload_services;

#ifdef PROFILE
	pf_begin();
#endif

	app_nanocube_solve_query(query, buffer);

	Request_print(request, &buffer->print_header);
	Request_print(request, &buffer->print_result);


#ifdef PROFILE
	// pf_generate_log_events();
	pf_generate_report();
	Request_print(request, &pf_report.print);
	pf_end();

	{
		u64 count = pf_Table_current_events_count(global_profile_table);
		u64 capacity = global_profile_table->event_capacity;

		Print_clear(print);
		Print_cstr(print, "Profile Events (Count/Capacity/Usage): ");
		Print_u64(print, count);
		Print_char(print, ' ');
		Print_u64(print, capacity);
		Print_char(print, ' ');
		Print_u64(print, (100 * count)/capacity);
		Print_char(print, '%');
		Print_char(print, '\n');
		Request_print(request, print);
	}
#endif

free_resources:

	/* free blocks */
	app_NanocubesAndAliases_free_resources(&info);

}

//------------------------------------------------------------------------------
// create service
//------------------------------------------------------------------------------

internal void
service_demo_create(Request *request)
{

	Print      *print   = request->print;
	op_Options *options = &request->options;

	MemoryBlock input_filename  = { .begin=0, .end=0 };
	MemoryBlock output_filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &input_filename) ||
	    !op_Options_str(options, 2, &output_filename)) {
		Request_msg(request, "[create] not enough input parameters.\n");
		Request_msg(request, "[create] usage: nanocube draw <input> <output>.\n");
		return;
	}

	b8  filter = 0;
	u64 filter_offset = 0;
	u64 filter_count  = 0; // zero means no constraint
	if (op_Options_find_cstr(options,"-filter")) {
		if (!op_Options_named_u64_cstr(options,"-filter",0,&filter_offset)
		    ||!op_Options_named_u64_cstr(options,"-filter",1,&filter_count)) {
			Request_msg(request, "[create] invalid offset or count in option -filter.\n");
			Request_msg(request, "[create] option usage: -filter=<offset>,<count>\n");
			return;
		} else {
			filter = 1;
		}
	}

	// mmap 1GB of virtual memory in this process
	// page aligned memory block
	pt_Memory data_memory = platform.allocate_memory(Gigabytes(10), 12, Terabytes(2));
	pt_Memory insert_buffer_memory = platform.allocate_memory(sizeof(nx_Threads) + Megabytes(1), 0, 0);
	// ~4MB of buffer for insertion:  sizeof(Threads): 4259856

	al_Allocator* allocator      = al_Allocator_new(data_memory.memblock.begin, data_memory.memblock.end);
	al_Cache*     nanocube_cache = al_Allocator_create_cache(allocator, "nv_Nanocube", sizeof(nv_Nanocube));
	nv_Nanocube*  nanocube       = (nv_Nanocube*) al_Cache_alloc(nanocube_cache);

	nv_Nanocube_init(nanocube); // initialize nanocube (index is not initialized after this call)

	al_Allocator_set_root(allocator, nanocube);

	// load load_info info
	ndmp_LoadInfo load_info;
	ndmp_LoadInfo_Error error = ndmp_LoadInfo_init(&load_info, nanocube,
						       &input_filename);

	if (error)
	{
		Request_msg(request, "[create-fail] could not extract info from input file.\n");
		return;
	}

	nv_Nanocube_init_index(nanocube, allocator);
	nv_Nanocube_init_key_value_store(nanocube, allocator);

	// paths storage
	nx_Label paths[Kilobytes(2)];
	Assert(load_info.depth < sizeof(paths) && "Dimensions are too deep");

	// prepare addresses
	nx_LabelArrayList address[64]; // some max dimensions
	Assert(load_info.num_dimensions <= 64);
	for (s32 i=0, offset=0;i<load_info.num_dimensions;++i) {
		address[i].next =
			(i < load_info.num_dimensions - 1)
			? &address[i+1]
			: 0;
		u8 levels = nanocube->index_dimensions.num_levels[i];
		nx_Array_init(&address[i].labels, paths + offset, levels);
		offset += levels;
	}

	// there is enough info to parse the records
	pt_File pfh = platform.open_read_file(input_filename.begin, input_filename.end);

	// position file
	platform.seek_file(&pfh, load_info.length + load_info.record_size * filter_offset);

	if (!pfh.last_seek_success)
	{
		Request_msg(request, "[create-fail] could not seek input file to first requested record.\n");
		return;
	}

	char buffer[Kilobytes(4)];

	Assert(load_info.record_size < sizeof(buffer) && "Record too large");

	s64 count = 0;

	f64 t0 = platform.get_time();

	for (;;) {

		platform.read_next_file_chunk(&pfh, buffer,
					      buffer + load_info.record_size);

		if (pfh.last_read !=  (u32) load_info.record_size)
			break;

		++count;

		if (filter && (u64) count > filter_count)
			break;
		// fprintf(stderr, "record: %d\n", ++count);

		//if (count < 0)
		//	continue;

		u64 offset = 0;

		for (s32 i=0;i<load_info.num_dimensions;++i) {

			s32 format = load_info.formats[i];
			// auto resol  = load_info.resolutions[i];

			nx_LabelArrayList* path   = address + i;
			s32 levels = nanocube->index_dimensions.num_levels[i];
			if (format == ndmp_TOKEN_QUADTREE) {
				// read two integers
				u32 xy[2];
				pt_copy_bytes( buffer + offset, buffer + offset + sizeof(xy), (char*) &xy, (char*) &xy + sizeof(xy));

				// fprintf(stderr,"----->  x:%d  y:%d\n", xy[0], xy[1]);

				// quadtree path
				offset += sizeof(xy);

				for (s32 j=0;j<levels;++j) {
					u32 bit = (u32) (1ull << (levels - 1 - j));
					u8 lbl = (u8) (((xy[0] & bit) ? 1 : 0) + ((xy[1] & bit) ? 2 : 0));
					nx_Array_set(&path->labels, (u8) j, lbl);
				}
			}
			else if (format == ndmp_TOKEN_TIME) {
				u16 time = 0;
				s32 n = load_info.resolutions[i];
				pt_copy_bytes(buffer + offset, buffer + offset + n, (char*) &time, (char*) &time + n);


				// fprintf(stderr,"----->  time: %d\n", time);

				// quadtree path
				offset += n;

				for (s32 j=0;j<levels;++j) {
					u32 bit = (u32) (1ull << (levels - 1 - j));
					nx_Label lbl = (nx_Label) ((time & bit) ? 1 : 0);
					nx_Array_set(&path->labels,(u8) j, lbl);
				}
			}
			else if (format == ndmp_TOKEN_CATEGORICAL) {
				u8 cat = 0;
				pt_copy_bytes(buffer + offset, buffer + offset + 1, (char*) &cat, (char*) &cat + 1);


				// fprintf(stderr,"----->  cat: %d\n", cat);

				// quadtree path
				offset += 1;

				nx_Array_set(&path->labels, (u8) 0, cat);

			}
		}

		//		for (u32 i = 0; i < nanocube_count->dimensions; ++i) {
		//			fprintf(stderr, "[%d] -> ", i);
		//			for (PathLength j = 0; j < nanocube_count->num_levels[i]; ++j) {
		//				fprintf(stderr, "%d ", (s32) Array_get(&(address+i)->labels,j) );
		//			}
		//			fprintf(stderr, "\n");
		//		}

		// assuming only one variable to be stored
		u32 payload_unit = 0;
		pt_copy_bytes(buffer + offset, buffer + offset + load_info.variable_size, (char*) &payload_unit, (char*) &payload_unit + load_info.variable_size);


		// ready to insert record!
		nx_NanocubeIndex_insert(&nanocube->index, address, &payload_unit, nanocube, insert_buffer_memory.memblock);

		if ((count % 100000) == 0) {
// 			Print_cstr(print,"Allocator usage after ");
// 			Print_u64(print, count);
// 			Print_align(print, 8, 1);
// 			Print_cstr(print, " records in #pages is ");
// 			Print_u64(print, allocator->used_pages);
// 			Print_char(print, '\n');
// 			Request_print(request, print);
// 			Print_clear(print);
			Print_clear(print);
			Print_cstr(print, "[dmp] progress ");
			Print_u64(print,(u64) count);
			Print_align(print, 10, 1, ' ');
			Print_cstr(print,"  time ");
			Print_u64(print,(u64)
				  (platform.get_time() - t0));
			Print_align(print, 10, 1, ' ');
			Print_cstr(print,"  memory(MB) ");
			Print_u64(print, (u64)(allocator->used_pages *
					       al_PAGE_SIZE)/Megabytes(1));
			Print_align(print, 10, 1, ' ');
			Print_cstr(print,"\n");
			Request_print(request, print);
			Print_clear(print);
		}
	}

	Print_cstr(print,"Allocator usage after ");
	Print_u64(print, count);
	Print_align(print, 8, 1, ' ');
	Print_cstr(print, " records in #pages is ");
	Print_u64(print, allocator->used_pages);
	Print_char(print, '\n');
	platform.write_to_file(request->pfh_stdout, print->begin, print->end);
	Print_clear(print);

	nu_log_memory(allocator, &nanocube->index, print, 0, 0);
	Request_print(request, print);
	Print_clear(print);


	// write to file
	pt_File pfh_db = platform.open_write_file(output_filename.begin, output_filename.end);
	platform.write_to_file(&pfh_db,
			       (char*) allocator,
			       (char*) allocator + allocator->used_pages
			       * al_PAGE_SIZE);
	platform.close_file(&pfh_db);
	Request_msg(request, "[create-success] file saved!\n");

	//     Print_cstr(&print,"sizeof(Threads): ");
	//     Print_u64(&print, sizeof(Threads));
	//     Print_char(&print,'\n');
	//     platform.write_to_file(stdout, print.begin, print.end);
	//     Print_clear(&print);

}


internal u128
app_quadtree2_path_number(f32 lat1, f32 lon1, f32 lat2, f32 lon2)
{
	// mercator from degrees
	f64 mercator_x1 = lon1 / 180.0;
	f64 mercator_y1 = pt_log_f64( pt_tan_f64( (lat1 * pt_PI/180.0)/2.0 + pt_PI/4.0)) / pt_PI;
	f64 mercator_x2 = lon2 / 180.0;
	f64 mercator_y2 = pt_log_f64( pt_tan_f64( (lat2 * pt_PI/180.0)/2.0 + pt_PI/4.0)) / pt_PI;

	u64 resolution = 25;

	/* fixed max resolution */
	u64 bins = 1ull << resolution;
	u64 cell_x1 = (u64) ((mercator_x1 + 1.0)/2.0 * bins);
	u64 cell_y1 = (u64) ((mercator_y1 + 1.0)/2.0 * bins);
	u64 cell_x2 = (u64) ((mercator_x2 + 1.0)/2.0 * bins);
	u64 cell_y2 = (u64) ((mercator_y2 + 1.0)/2.0 * bins);

	u128 result;
	result.high = 0;
	result.low  = 0;

	/* 4 bits per level */
	/* 25 * 4 ... break into 12 * 4 + 13 * 4 */
	u64 resolution_high = 12;
	u64 resolution_low  = 13;

	for (u8 i=0;i<resolution_low;i++) {
		u64 label = ((cell_x1 & 1) ? 1 : 0) + ((cell_y1 & 1) ? 2 : 0)
			+ ((cell_x2 & 1) ? 4 : 0) + ((cell_y2 & 1) ? 8 : 0);
		cell_x1 >>= 1;
		cell_y1 >>= 1;
		cell_x2 >>= 1;
		cell_y2 >>= 1;
		result.low += (label << (4*i));
	}

	for (u8 i=0;i<resolution_high;i++) {
		u64 label = ((cell_x1 & 1) ? 1 : 0) + ((cell_y1 & 1) ? 2 : 0)
			+ ((cell_x2 & 1) ? 4 : 0) + ((cell_y2 & 1) ? 8 : 0);
		cell_x1 >>= 1;
		cell_y1 >>= 1;
		cell_x2 >>= 1;
		cell_y2 >>= 1;
		result.high += (label << (4*i));
	}

	return result;
}

internal u64
app_tile_path_number(u64 level, u64 cell_x, u64 cell_y)
{
	/* fixed max resolution */
	u64 result = 0;
	// u64 bins = 1ull << level;
	u8 full_resolution = 25;
	for (u8 i=0;i<level;i++) {
		u64 label = ((cell_x & 1) ? 1 : 0) + ((cell_y & 1) ? 2 : 0);
		cell_x >>= 1;
		cell_y >>= 1;
		result += (label << (2*i));
	}
	for (u8 i=level;i<full_resolution;++i) {
		result <<= 2; /* push two bits */
	}
	return result;
}

internal u64
app_quadtree_path_number(f32 lat, f32 lon)
{
	// mercator from degrees
	f64 mercator_x = lon / 180.0;
	f64 mercator_y = pt_log_f64( pt_tan_f64( (lat * pt_PI/180.0)/2.0 + pt_PI/4.0)) / pt_PI;

	u64 resolution = 25;

	/* fixed max resolution */
	u64 bins = 1ull << resolution;
	u64 cell_x = (u64) ((mercator_x + 1.0)/2.0 * bins);
	u64 cell_y = (u64) ((mercator_y + 1.0)/2.0 * bins);

	u64 path_number = 0;
	for (u8 i=0;i<resolution;i++) {
		u64 label = ((cell_x & 1) ? 1 : 0) + ((cell_y & 1) ? 2 : 0);
		cell_x >>= 1;
		cell_y >>= 1;
		path_number += (label << (2*i));
	}

	return path_number;
}


/* TPart2 */

/*
 * Feature to let a user partition incoming pairs of quadtree
 * location per record based on a custom input function.
 *
 * <level1> <x1> <y1> <level2> <x2> <y2>
 * <level1> <x1> <y1> <level2> <x2> <y2>
 * next
 * <level1> <x1> <y1> <level2> <x2> <y2>
 * next
 * <level1> <x1> <y1> <level2> <x2> <y2>
 */

#define app_TPart2_MAX_NUM_POINTS 512
#define app_TPart2_SEP 0xFFFFFFFFull

typedef struct {
	MemoryBlock      filename;

	MemoryBlock      lat1_name;
	MemoryBlock      lon1_name;
	u32              lat1_index;
	u32              lon1_index;

	MemoryBlock      lat2_name;
	MemoryBlock      lon2_name;
	u32              lat2_index;
	u32              lon2_index;

	/* each u128 will encode the      */
	/* let 0 0 be the new part signal */
	u128             *cut_begin;
	u128             *cut_end;
	u128             buffer[app_TPart2_MAX_NUM_POINTS];

	u64              set;

	b8               active;
} app_TPart2;

internal void
app_TPart2_init(app_TPart2 *self)
{
	pt_fill((char*) self, (char*) self + sizeof(*self), 0);
	self->cut_begin = self->buffer;
	self->cut_end   = self->buffer;
}

internal b8
app_TPart2_read_latlon(app_TPart2 *self, MemoryBlock *begin, MemoryBlock *end, f32 *lat1, f32 *lon1, f32 *lat2, f32 *lon2)
{
	Assert(begin + self->lat1_index < end);
	Assert(begin + self->lon1_index < end);
	Assert(begin + self->lat2_index < end);
	Assert(begin + self->lon2_index < end);
	MemoryBlock *lat1_text = begin + self->lat1_index;
	MemoryBlock *lon1_text = begin + self->lon1_index;
	MemoryBlock *lat2_text = begin + self->lat2_index;
	MemoryBlock *lon2_text = begin + self->lon2_index;

	*lat1 = 0.0;
	*lon1 = 0.0;
	*lat2 = 0.0;
	*lon2 = 0.0;
	if (!pt_parse_f32(lat1_text->begin, lat1_text->end, lat1)
	    || !pt_parse_f32(lon1_text->begin, lon1_text->end, lon1)
	    || !pt_parse_f32(lat2_text->begin, lat2_text->end, lat2)
	    || !pt_parse_f32(lon2_text->begin, lon2_text->end, lon2)) {
		return 0;
	} else {
		return 1;
	}
}

internal b8
app_TPart2_read(app_TPart2 *self)
{
	Assert(self->active);

	pt_MappedFile mapped_file = platform.open_mmap_file(self->filename.begin, self->filename.end,1,0);
	if (!mapped_file.mapped) {
		return 0;
	}

	nt_Tokenizer tokenizer;
	char sep[] = "\n\t ";
	nt_Tokenizer_init_canonical(&tokenizer, sep, cstr_end(sep));
	nt_Tokenizer_reset_text(&tokenizer, mapped_file.begin, mapped_file.begin + mapped_file.size);
	u64 numbers[6];
	u64 num_index = 0;
	while (nt_Tokenizer_next(&tokenizer)) {
		if (self->cut_end - self->cut_begin == app_TPart2_MAX_NUM_POINTS) {
			return 0;
		} else if (pt_parse_u64(tokenizer.token.begin, tokenizer.token.end, numbers + num_index)) {
			if (num_index == 5) {
				/* perform econding of tile */
				self->cut_end->low   = app_tile_path_number(numbers[0], numbers[1], numbers[2]);
				self->cut_end->low  += (numbers[0] & 0x3f) << 58;
				self->cut_end->high  = app_tile_path_number(numbers[3], numbers[4], numbers[5]);
				self->cut_end->high += (numbers[3] & 0x3f) << 58;
				++self->cut_end;
			};
			num_index = (num_index + 1) % 6;
		} else if (pt_compare_memory_cstr(tokenizer.token.begin, tokenizer.token.end, "new") == 0) {
			if (num_index != 0) {
				return 0;
			} else {
				self->cut_end->high = app_TPart2_SEP;
				self->cut_end->low  = app_TPart2_SEP;
				++self->cut_end;
			}
		} else {
			/* invalid input */
			return 0;
		}
	}
	platform.close_mmap_file(&mapped_file);
	return 1;
}

internal u32
app_TPart2_part_number(app_TPart2 *self, f32 lat1, f32 lon1, f32 lat2, f32 lon2)
{
	/* compute u64 quadtree number */

	u64 full_resolution_1 = app_quadtree_path_number(lat1, lon1);
	u64 full_resolution_2 = app_quadtree_path_number(lat2, lon2);

	u32 part = 0;
	u128 *it = self->cut_begin;
	while (it != self->cut_end) {
		if (it->low == app_TPart2_SEP) {
			++part;
		} else {
			u64 level_1   = (it->low  >> 58);
			u64 level_2   = (it->high >> 58);
			u64 shift_1   = (2 * (25 - level_1));
			u64 shift_2   = (2 * (25 - level_2));
			u64 prefix_1  = (it->low   << 6) >> (6 + shift_1);
			u64 prefix_2  = (it->high  << 6) >> (6 + shift_2);
			u64 input_prefix_1  = full_resolution_1 >> shift_1;
			u64 input_prefix_2  = full_resolution_2 >> shift_2;
			b8  match_1   = prefix_1 == input_prefix_1;
			b8  match_2   = prefix_2 == input_prefix_2;
			if (match_1 && match_2) {
				return part;
			}
		}
		++it;
	}
	/* otherwise return largest part number for anything that doesn't match */
	return part+1;
}


internal b8
app_TPart2_consider_point(app_TPart2 *self, f32 lat1, f32 lon1, f32 lat2, f32 lon2)
{
	u32 part = app_TPart2_part_number(self, lat1, lon1, lat2, lon2);
	return (1ull << part) & self->set;
}


/* QPart2 */

#define app_QPart2_MAX_NUM_CUTS 64

typedef struct {

	MemoryBlock      filename;

	MemoryBlock      lat1_name;
	MemoryBlock      lon1_name;
	u32              lat1_index;
	u32              lon1_index;

	MemoryBlock      lat2_name;
	MemoryBlock      lon2_name;
	u32              lat2_index;
	u32              lon2_index;

	u64              set;

	u128             *cut_begin;
	u128             *cut_end;
	u128             buffer[app_QPart2_MAX_NUM_CUTS];

	b8               active;

} app_QPart2;

internal void
app_QPart2_init(app_QPart2 *self)
{
	pt_fill((char*) self, (char*) self + sizeof(*self), 0);
	self->cut_begin = self->buffer;
	self->cut_end   = self->buffer;
}

internal b8
app_QPart2_read(app_QPart2 *self)
{
	Assert(self->active);

	pt_MappedFile mapped_file = platform.open_mmap_file(self->filename.begin, self->filename.end,1,0);
	if (!mapped_file.mapped) {
		return 0;
	}

	nt_Tokenizer tokenizer;
	char sep[] = "\n ";
	nt_Tokenizer_init_canonical(&tokenizer, sep, cstr_end(sep));
	nt_Tokenizer_reset_text(&tokenizer, mapped_file.begin, mapped_file.begin + mapped_file.size);
	b8 parity = 0;
	while (nt_Tokenizer_next(&tokenizer)) {
		if (self->cut_end == self->cut_begin + app_QPart2_MAX_NUM_CUTS) {
			return 0;
		}
		if (parity == 0) {
			if (!pt_parse_u64(tokenizer.token.begin, tokenizer.token.end, &self->cut_end->high)) {
				return 0;
			}
		} else {
			if (!pt_parse_u64(tokenizer.token.begin, tokenizer.token.end, &self->cut_end->low)) {
				return 0;
			}
			++self->cut_end;
		}
		parity = 1 - parity;
	}
	platform.close_mmap_file(&mapped_file);
	return 1;
}


internal b8
app_QPart2_read_latlon(app_QPart2 *self, MemoryBlock *begin, MemoryBlock *end, f32 *lat1, f32 *lon1, f32 *lat2, f32 *lon2)
{
	Assert(begin + self->lat1_index < end);
	Assert(begin + self->lon1_index < end);
	Assert(begin + self->lat2_index < end);
	Assert(begin + self->lon2_index < end);
	MemoryBlock *lat1_text = begin + self->lat1_index;
	MemoryBlock *lon1_text = begin + self->lon1_index;
	MemoryBlock *lat2_text = begin + self->lat2_index;
	MemoryBlock *lon2_text = begin + self->lon2_index;

	*lat1 = 0.0;
	*lon1 = 0.0;
	*lat2 = 0.0;
	*lon2 = 0.0;
	if (!pt_parse_f32(lat1_text->begin, lat1_text->end, lat1)
	    || !pt_parse_f32(lon1_text->begin, lon1_text->end, lon1)
	    || !pt_parse_f32(lat2_text->begin, lat2_text->end, lat2)
	    || !pt_parse_f32(lon2_text->begin, lon2_text->end, lon2)) {
		return 0;
	} else {
		return 1;
	}
}

internal u32
app_QPart2_part_number(app_QPart2 *self, f32 lat1, f32 lon1, f32 lat2, f32 lon2)
{
	/* compute u64 quadtree number */
	u128 point = app_quadtree2_path_number(lat1, lon1, lat2, lon2);
	s64 n = self->cut_end - self->cut_begin;
	for (u64 i=0;i<n;++i) {
		if ((point.high < self->cut_begin[i].high)
		    || (point.high == self->cut_begin[i].high && point.low <= self->cut_begin[i].low)) {
			return i + 1;
		}
	}
	return n+1;
}


internal b8
app_QPart2_consider_point(app_QPart2 *self, f32 lat1, f32 lon1, f32 lat2, f32 lon2)
{
	/* compute u64 quadtree number */
	u128 point = app_quadtree2_path_number(lat1, lon1, lat2, lon2);
	s64 n = self->cut_end - self->cut_begin;
	for (u64 i=0;i<n;++i) {
		if (u128_lte(&point,self->cut_begin + i)) {
			if ((1ull << i) & self->set) {
				return 1;
			} else {
				return 0;
			}
		}
	}
	if ((1ull << n) & self->set) {
		return 1;
	}
	return 0;
}

//------------------------------------------------------------------------------
// QPart - quadtree partition
//------------------------------------------------------------------------------

#define app_QPart_MAX_NUM_PARTS 64
typedef struct {
	MemoryBlock filename;
	MemoryBlock lat_name;
	MemoryBlock lon_name;
	u32         lat_index;
	u32         lon_index;
	u64         set;
	u64         *cut_begin;
	u64         *cut_end;
	u64         buffer[app_QPart_MAX_NUM_PARTS];
	b8          active;
} app_QPart;

internal void
app_QPart_init(app_QPart *self)
{
	pt_fill((char*) self, (char*) self + sizeof(*self), 0);
	self->cut_begin = self->buffer;
	self->cut_end   = self->buffer;
}

internal b8
app_QPart_append(app_QPart *self, u64 cut_point)
{
	if (self->cut_end == self->cut_begin + app_QPart_MAX_NUM_PARTS)
		return 0;
	*self->cut_end = cut_point;
	++self->cut_end;
	return 1;
}

internal b8
app_QPart_read(app_QPart *self)
{
	Assert(self->active);

	pt_MappedFile mapped_file = platform.open_mmap_file(self->filename.begin, self->filename.end,1,0);
	if (!mapped_file.mapped) {
		return 0;
	}

	nt_Tokenizer tokenizer;
	char sep[] = "\n";
	nt_Tokenizer_init_canonical(&tokenizer, sep, cstr_end(sep));
	nt_Tokenizer_reset_text(&tokenizer, mapped_file.begin, mapped_file.begin + mapped_file.size);
	while (nt_Tokenizer_next(&tokenizer)) {
		u64 cut_point = 0;
		if (!pt_parse_u64(tokenizer.token.begin, tokenizer.token.end, &cut_point)) {
			return 0;
		}
		if (!app_QPart_append(self, cut_point)) {
			/* overflow */
			return 0;
		}
	}
	platform.close_mmap_file(&mapped_file);
	return 1;
}


internal b8
app_QPart_read_latlon(app_QPart *self, MemoryBlock *begin, MemoryBlock *end, f32 *lat, f32 *lon)
{
	Assert(begin + self->lat_index < end);
	Assert(begin + self->lon_index < end);
	MemoryBlock *lat_text = begin + self->lat_index;
	MemoryBlock *lon_text = begin + self->lon_index;

	*lat = 0.0;
	*lon = 0.0;
	if (!pt_parse_f32(lat_text->begin, lat_text->end, lat)
	    || !pt_parse_f32(lon_text->begin, lon_text->end, lon)) {
		return 0;
	} else {
		return 1;
	}
}

internal u32
app_QPart_part_number(app_QPart *self, f32 lat, f32 lon)
{
	/* compute u64 quadtree number */
	u64 path_number = app_quadtree_path_number(lat, lon);
	s64 n = self->cut_end - self->cut_begin;
	for (u64 i=0;i<n;++i) {
		if (path_number <= self->cut_begin[i]) {
			return i + 1;
		}
	}
	return n+1;
}

internal b8
app_QPart_consider_point(app_QPart *self, f32 lat, f32 lon)
{
	/* compute u64 quadtree number */
	u64 path_number = app_quadtree_path_number(lat, lon);
	s64 n = self->cut_end - self->cut_begin;
	for (u64 i=0;i<n;++i) {
		if (path_number <= self->cut_begin[i]) {
			if ((1ull << i) & self->set) {
				return 1;
			} else {
				return 0;
			}
		}
	}
	if ((1ull << n) & self->set) {
		return 1;
	}
	return 0;
}


#define print_size_of(name) \
	Print_cstr(print, #name ); \
	Print_align(print,32,-1,' '); \
	Print_u64(print,sizeof(name)); \
	Print_char(print,'\n'); \

internal void
service_sizes(Request *request)
{
	Print *print = request->print;
	print_size_of(nv_Nanocube);
	print_size_of(nx_NanocubeIndex);
	print_size_of(nm_Measure);
	Request_print(request, print);
}

//------------------------------------------------------------------------------
// time service: test time routines
//------------------------------------------------------------------------------

internal void
service_time(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	tm_initialize_timezones();

	s32 a=1900, b=1901;

	if (!op_Options_s32(options,1,&a) || !op_Options_s32(options,2,&b)) {
		Request_msg(request, "[time] usage is 'nanocube time <year> <year>\n");
		return;
	}

	Print_cstr(print, "Year interval [");
	Print_u64(print, (u64) a);
	Print_cstr(print, ",");
	Print_u64(print, (u64) b);
	Print_cstr(print, ")\n");
	Print_cstr(print, "Leap years: ");
	Print_u64(print, (u64) tm_leap_years_between(a,b));
	Print_cstr(print, "\n");
	Print_cstr(print, "Days: ");
	Print_u64(print, (u64) tm_days_between_years(a,b));
	Print_cstr(print, "\n");
	Request_print(request, print);

	tm_Label label = {.year=2016, .month=3, .day=1,
		.hour=23, .minute=18, .second=15,
		.offset_minutes=-60*5, .timezone = 0};

	Print_clear(print);
	tm_Label_print(&label, print);
	Print_cstr(print,"\n");
	Request_print(debug_request, print);

	tm_Label_adjust_offset(&label, 5 *60);

	Print_clear(print);
	tm_Label_print(&label, print);
	Print_cstr(print,"\n");
	Request_print(debug_request, print);

	tm_Time time;
	tm_Time_init_from_label(&time, &label);

	Print_clear(print);
	Print_s64(print, time.time);
	Print_cstr(print, "\n");
	Request_print(request, print);

	// time.time = 2698012800;
	tm_Label_init(&label, time);

	Print_clear(print);
	tm_Label_print(&label, print);
	Print_cstr(print,"\n");
	Request_print(request, print);


	/* try to parse previous label */
	Print_clear(print);
	tm_Label_print(&label, print);

	Print_clear(print);
	/* Print_cstr(print, "2016-01-01T00:00:00-05:00"); */
// 	Print_cstr(print, "09-06-2016 09:06:36 PM");
	Print_cstr(print, "2016 00:00 -05:00");

	ntp_Parser   parser;
	ntp_Parser_init(&parser);
	if (!ntp_Parser_run(&parser, print->begin, print->end)) {
		Request_print(request, &parser.log);
	} else {
		Print_clear(print);
		tm_Label_print(&parser.label, print);
		Print_cstr(print,"\n");
		Request_print(request, print);
	}

	Print_clear(print);
	Print_cstr(print, "Weekday of 2016-06-09: ");
	Print_u64(print, tm_weekday(2016,6,9));
	Print_cstr(print,"\n");
	Request_print(request, print);

	/* test parsing chicago crime formatted date */
	Print_clear(print);
	Print_cstr(print, "04/29/2010 10:15:00 PM");
	if (!ntp_Parser_run(&parser, print->begin, print->end)) {
		Request_print(request, &parser.log);
	} else {
		Print_clear(print);
		tm_Label_print(&parser.label, print);
		Print_cstr(print,"\n");
		Request_print(request, print);
	}
}




/*
BEGIN_DOC_STRING nanocube_qpart_doc
Ver:   __VERSION__
Usage: nanocube qpart CSVFILE LAT LON SAMPLESIZE K [OPTIONS]
Consider quadtree numbers coming from mercator projection of
degree lat/lon in a .csv file. Find the quantiles of these
numbers to be used for partitioning data with similar spatial
distribution in a balanced way. Use SAMPLESIZE samples from
the range of records in the csv file, and create K groups.
Columns for latitude and longitude in the .csv file are
given by LAT and LON.
Options
    -sep=C
         Character C is the .csv file separator
    -unique
         If set, make quantile cuts after erasing duplicate quadtree numbers
    -filter=OFFSET,COUNT
         Consider only records in the given range
    -snap=FILE,MAXDIST
         Snap points to a snap database before considering the quantiles

END_DOC_STRING
*/


internal void
service_qpart(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;


	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_qpart_doc);
		Request_print(request, print);
		return;
	}


	MemoryBlock csv_filename = { .begin=0, .end=0 };
	MemoryBlock separator    = { .begin=0, .end=0 };
	MemoryBlock latitude     = { .begin=0, .end=0 };
	MemoryBlock longitude    = { .begin=0, .end=0 };
	u64         sample_size  = 0;
	u64         k            = 0;


	static char *sep = ",";
	separator.begin = sep;
	separator.end   = cstr_end(sep);
	if (op_Options_find_cstr(options,"-sep")) {
		if (!op_Options_named_str_cstr(options,"-sep",0,&separator)) {
			Request_msg(request, "[qpart] invalid separator\n");
			Request_msg(request, "[qpart] option usage: -sep=<sep>\n");
			Request_msg(request, "[qpart] option usage: -filter=<offset>,<count>\n");
			return;
		}
	}

	/* make quantile cuts after erasing duplicates ? */
	b8 uniq = op_Options_find_cstr(options,"-unique") != 0;

	b8  filter = 0;
	u64 filter_offset = 0;
	u64 filter_count  = 0; // zero means no constraint
	if (op_Options_find_cstr(options,"-filter")) {
		if (!op_Options_named_u64_cstr(options,"-filter",0,&filter_offset)
		    ||!op_Options_named_u64_cstr(options,"-filter",1,&filter_count)) {
			Request_msg(request, "[qpart] invalid offset or count in option -filter.\n");
			Request_msg(request, "[qpart] option usage: -filter=<offset>,<count>\n");
			return;
		} else {
			filter = 1;
		}
	}


	pt_MappedFile              snap_mapped_file;
	cm_SnappingLatLonFunction *snap_function = 0;
	if (op_Options_find_cstr(options,"-snap")) {
		if (!op_Options_named_f32_cstr(options,"-snap",1,&g_snap_maxdist)) {
			Request_msg(request, "[qpart] missing maxdist\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
		}
		MemoryBlock roadmap_filename = {.begin=0, .end=0};
		if (!op_Options_named_str_cstr(options,"-snap",0, &roadmap_filename)) {
			Request_msg(request, "[qpart] -snap: missing roadmap filename\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		snap_mapped_file = platform.open_mmap_file(roadmap_filename.begin, roadmap_filename.end, 1, 0);
		if (!snap_mapped_file.mapped) {
			Request_msg(request, "[qpart] couldn't open snap file\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		/* set graph */
		al_Allocator *allocator = (al_Allocator*) snap_mapped_file.begin;
		g_snap_graph = (rg_Graph*) al_Allocator_get_root(allocator);
		if (!g_snap_graph) {
			Request_msg(request, "[qpart] couldn't find snap graph inside roadmap file\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		snap_function = g_snap;
	}

	if (!op_Options_str(options, 1, &csv_filename)
	    || !op_Options_str(options, 2, &latitude)
	    || !op_Options_str(options, 3, &longitude)
	    || !op_Options_u64(options, 4, &sample_size)
	    || !op_Options_u64(options, 5, &k)) {
		Request_msg(request, "[qpart] missing some parameters.\n");
		Request_msg(request, "[qpart] usage:  qpart <csv-fname> <latcol> <loncol> <sample-size> <k>\n");
		Request_msg(request, "[qpart] option: -sep=<sep>\n");
		Request_msg(request, "[qpart] option: -filter=<offset>,<count>\n");
		Request_msg(request, "[qpart] option: -snap=<roadmap-filename>,<maxdist-f32>\n");
		return;
	}

	pt_MappedFile mapped_file = platform.open_mmap_file(csv_filename.begin, csv_filename.end, 1, 0);
	if (!mapped_file.mapped) {
		Request_msg(request, "[qpart] couldn't map .csv file.\n");
		return;
	}

	pt_Memory sample_memory = platform.allocate_memory(sample_size * sizeof(u64), 3, 0);
	Array_u64 sample;
	Array_u64_init(&sample, (u64*) sample_memory.memblock.begin, (u64*) sample_memory.memblock.end);

	static char newline[] = "\n\r";
	nt_Tokenizer line_tokenizer;
	nt_Tokenizer_init_canonical(&line_tokenizer, newline, cstr_end(newline));
	nt_Tokenizer_reset_text(&line_tokenizer, mapped_file.begin, mapped_file.begin + mapped_file.size);
	nt_Token *line_token = &line_tokenizer.token;

	nt_Tokenizer column_tokenizer;
	nt_Tokenizer_init_canonical(&column_tokenizer, separator.begin, separator.end);

	/* limit number of columns */
	MemoryBlock buffer[1024];
	ArrayMemoryBlock tokens;
	ArrayMemoryBlock_init(&tokens, buffer, buffer + 1024);

	/* read header line */
	u32 line = 0;
	if (nt_Tokenizer_next(&line_tokenizer)) {
		++line;
		nt_Tokenizer_reset_text(&column_tokenizer, line_token->begin, line_token->end);
		ArrayMemoryBlock_clear(&tokens);
		while (nt_Tokenizer_next(&column_tokenizer)) {
			ArrayMemoryBlock_append(&tokens, nt_Tokenizer_token_memblock(&column_tokenizer));
		}
	}

	/* search latitude and longitude column indices */
	b8  ok = 1;
	s64 latitude_index  = -1;
	s64 longitude_index = -1;
	for (u32 i=0;i<ArrayMemoryBlock_size(&tokens);++i) {
		MemoryBlock value = ArrayMemoryBlock_get(&tokens, i);
		if (pt_compare_memory(latitude.begin, latitude.end, value.begin, value.end) == 0) {
			if (latitude_index < 0) {
				latitude_index = i;
			} else {
				ok = 0;
				break;
			}
		}
		if (pt_compare_memory(longitude.begin, longitude.end, value.begin, value.end) == 0) {
			if (longitude_index < 0) {
				longitude_index = i;
			} else {
				ok = 0;
				break;
			}
		}
	}

	if (!ok) {
		Request_msg(request, "[qpart] couldn't find latitude or longitude columns.\n");
		return;
	}

	u64 point_index    = 0;
	u32 problems       = 0;
	s64 offset         = -1;
	while (nt_Tokenizer_next(&line_tokenizer)) {
		++offset;
		++line;

		if (filter) {
			if (offset < filter_offset) {
				continue;
			} else if (offset > filter_offset + filter_count) {
				break;
			}
		}

		nt_Tokenizer_reset_text(&column_tokenizer, line_token->begin, line_token->end);
		ArrayMemoryBlock_clear(&tokens);
		while (nt_Tokenizer_next(&column_tokenizer)) {
			ArrayMemoryBlock_append(&tokens, nt_Tokenizer_token_memblock(&column_tokenizer));
		}

		if (latitude_index >= ArrayMemoryBlock_size(&tokens)
		    || latitude_index >= ArrayMemoryBlock_size(&tokens)) {
			++problems;
			continue;
		}

		f32 lat, lon;
		MemoryBlock lat_token = ArrayMemoryBlock_get(&tokens, latitude_index);
		MemoryBlock lon_token = ArrayMemoryBlock_get(&tokens, longitude_index);
		if (!pt_parse_f32(lat_token.begin, lat_token.end, &lat) ||
				!pt_parse_f32(lon_token.begin, lon_token.end, &lon)) {
			++problems;
			continue;
		}

		if (snap_function) {
			if (!snap_function(&lat, &lon)) {
				++problems;
				continue;
			}
		}

// 		Print_clear(print);
// 		Print_cstr(print, "[");
// 		Print_f64(print, (f64) lat);
// 		Print_align(print, 12, 1, ' ');
// 		Print_cstr(print, ",");
// 		Print_f64(print, (f64) lon);
// 		Print_align(print, 12, 1, ' ');
// 		Print_cstr(print, "]");
// 		Print_cstr(print, "\n");
// 		Request_print(request, print);

		/* convert to path using mercator projection */
		u64 path_number  = app_quadtree_path_number(lat, lon);

// 		Print_clear(print);
// 		Print_u64(print, path_number);
// 		Print_cstr(print, "\n");
// 		Request_print(request, print);

		if (point_index >= sample_size) {
			/* choose random point to replace */
			rnd_next();
			u64 index = (u64) rnd_state % (point_index+1);
			if (index < sample_size) {
				Array_u64_set(&sample, index, path_number);
			}
		} else {
			Array_u64_append(&sample, path_number);
		}

		++point_index;
	}

	if (Array_u64_size(&sample) > 0) {

		/* sort sample and return the k-th moments */
		qsort_u64(sample.begin, sample.end);
		if (!check_sorted_u64(sample.begin, sample.end)) {
			Print_clear(print);
			Print_cstr(print, "PROBLEM IN SORTING!\n");
			Request_print(request, print);
		}

		if (uniq) {
			sample.end = u64_remove_duplicates(sample.begin, sample.end);
			if (!check_sorted_uniqueness_u64(sample.begin, sample.end)) {
				Print_clear(print);
				Print_cstr(print, "PROBLEM IN UNIQUENESS!\n");
				Request_print(request, print);
			}
		}

		u64 n = Array_u64_size(&sample);
		f64 coef = (f64) n / (k+1);
		for (u64 i=0;i<k;++i) {
			u64 index = (u64) ((i+1) * coef);
			u64 value = Array_u64_get(&sample, index);
			Print_clear(print);
			Print_u64(print, value);
			Print_char(print,'\n');
			Request_print(request, print);
		}
	}

	platform.free_memory(&sample_memory);
	platform.close_mmap_file(&mapped_file);

	if (snap_function) {
		platform.close_mmap_file(&snap_mapped_file);
	}

}

internal void
service_qpart2(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;


	MemoryBlock csv_filename = { .begin=0, .end=0 };
	MemoryBlock separator    = { .begin=0, .end=0 };
	MemoryBlock latitude1    = { .begin=0, .end=0 };
	MemoryBlock longitude1   = { .begin=0, .end=0 };
	MemoryBlock latitude2    = { .begin=0, .end=0 };
	MemoryBlock longitude2   = { .begin=0, .end=0 };
	u64         sample_size  = 0;
	u64         k            = 0;


	static char *sep = ",";
	separator.begin = sep;
	separator.end   = cstr_end(sep);
	if (op_Options_find_cstr(options,"-sep")) {
		if (!op_Options_named_str_cstr(options,"-sep",0,&separator)) {
			Request_msg(request, "[qpart] invalid separator\n");
			Request_msg(request, "[qpart] option usage: -sep=<sep>\n");
			Request_msg(request, "[qpart] option usage: -filter=<offset>,<count>\n");
			return;
		}
	}

	/* make quantile cuts after erasing duplicates ? */
	// b8 uniq = op_Options_find_cstr(options,"-unique") != 0;

	b8  filter = 0;
	u64 filter_offset = 0;
	u64 filter_count  = 0; // zero means no constraint
	if (op_Options_find_cstr(options,"-filter")) {
		if (!op_Options_named_u64_cstr(options,"-filter",0,&filter_offset)
		    ||!op_Options_named_u64_cstr(options,"-filter",1,&filter_count)) {
			Request_msg(request, "[qpart] invalid offset or count in option -filter.\n");
			Request_msg(request, "[qpart] option usage: -filter=<offset>,<count>\n");
			return;
		} else {
			filter = 1;
		}
	}


	pt_MappedFile              snap_mapped_file;
	cm_SnappingLatLonFunction *snap_function = 0;
	if (op_Options_find_cstr(options,"-snap")) {
		if (!op_Options_named_f32_cstr(options,"-snap",1,&g_snap_maxdist)) {
			Request_msg(request, "[qpart] missing maxdist\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
		}
		MemoryBlock roadmap_filename = {.begin=0, .end=0};
		if (!op_Options_named_str_cstr(options,"-snap",0, &roadmap_filename)) {
			Request_msg(request, "[qpart] -snap: missing roadmap filename\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		snap_mapped_file = platform.open_mmap_file(roadmap_filename.begin, roadmap_filename.end, 1, 0);
		if (!snap_mapped_file.mapped) {
			Request_msg(request, "[qpart] couldn't open snap file\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		/* set graph */
		al_Allocator *allocator = (al_Allocator*) snap_mapped_file.begin;
		g_snap_graph = (rg_Graph*) al_Allocator_get_root(allocator);
		if (!g_snap_graph) {
			Request_msg(request, "[qpart] couldn't find snap graph inside roadmap file\n");
			Request_msg(request, "[qpart] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		snap_function = g_snap;
	}

	if (!op_Options_str(options, 1, &csv_filename)
	    || !op_Options_str(options, 2, &latitude1)
	    || !op_Options_str(options, 3, &longitude1)
	    || !op_Options_str(options, 4, &latitude2)
	    || !op_Options_str(options, 5, &longitude2)
	    || !op_Options_u64(options, 6, &sample_size)
	    || !op_Options_u64(options, 7, &k)) {
		Request_msg(request, "[qpart] missing some parameters.\n");
		Request_msg(request, "[qpart] usage:  qpart <csv-fname> <latcol1> <loncol1> <latcol2> <loncol2> <sample-size> <k>\n");
		Request_msg(request, "[qpart] option: -sep=<sep>\n");
		Request_msg(request, "[qpart] option: -filter=<offset>,<count>\n");
		Request_msg(request, "[qpart] option: -snap=<roadmap-filename>,<maxdist-f32>\n");
		return;
	}

	pt_MappedFile mapped_file = platform.open_mmap_file(csv_filename.begin, csv_filename.end, 1, 0);
	if (!mapped_file.mapped) {
		Request_msg(request, "[qpart] couldn't map .csv file.\n");
		return;
	}

	pt_Memory sample_memory = platform.allocate_memory(sample_size * sizeof(u128), 3, 0);
	Array_u128 sample;
	Array_u128_init(&sample, (u128*) sample_memory.memblock.begin, (u128*) sample_memory.memblock.end);

	static char newline[] = "\n\r";
	nt_Tokenizer line_tokenizer;
	nt_Tokenizer_init_canonical(&line_tokenizer, newline, cstr_end(newline));
	nt_Tokenizer_reset_text(&line_tokenizer, mapped_file.begin, mapped_file.begin + mapped_file.size);
	nt_Token *line_token = &line_tokenizer.token;

	nt_Tokenizer column_tokenizer;
	nt_Tokenizer_init_canonical(&column_tokenizer, separator.begin, separator.end);

	/* limit number of columns */
	MemoryBlock buffer[1024];
	ArrayMemoryBlock tokens;
	ArrayMemoryBlock_init(&tokens, buffer, buffer + 1024);

	/* read header line */
	u32 line = 0;
	if (nt_Tokenizer_next(&line_tokenizer)) {
		++line;
		nt_Tokenizer_reset_text(&column_tokenizer, line_token->begin, line_token->end);
		ArrayMemoryBlock_clear(&tokens);
		while (nt_Tokenizer_next(&column_tokenizer)) {
			ArrayMemoryBlock_append(&tokens, nt_Tokenizer_token_memblock(&column_tokenizer));
		}
	}

	/* search latitude and longitude column indices */
	b8  ok = 1;
	s64 latitude1_index  = -1;
	s64 longitude1_index = -1;
	s64 latitude2_index  = -1;
	s64 longitude2_index = -1;
	for (u32 i=0;i<ArrayMemoryBlock_size(&tokens);++i) {
		MemoryBlock value = ArrayMemoryBlock_get(&tokens, i);
		if (pt_compare_memory(latitude1.begin, latitude1.end, value.begin, value.end) == 0) {
			if (latitude1_index < 0) {
				latitude1_index = i;
			} else {
				ok = 0;
				break;
			}
		}
		if (pt_compare_memory(longitude1.begin, longitude1.end, value.begin, value.end) == 0) {
			if (longitude1_index < 0) {
				longitude1_index = i;
			} else {
				ok = 0;
				break;
			}
		}
		if (pt_compare_memory(latitude2.begin, latitude2.end, value.begin, value.end) == 0) {
			if (latitude2_index < 0) {
				latitude2_index = i;
			} else {
				ok = 0;
				break;
			}
		}
		if (pt_compare_memory(longitude2.begin, longitude2.end, value.begin, value.end) == 0) {
			if (longitude2_index < 0) {
				longitude2_index = i;
			} else {
				ok = 0;
				break;
			}
		}
	}

	if (!ok) {
		Request_msg(request, "[qpart] couldn't find latitude or longitude columns.\n");
		return;
	}

	u64 point_index    = 0;
	u32 problems       = 0;
	s64 offset         = -1;
	while (nt_Tokenizer_next(&line_tokenizer)) {
		++offset;
		++line;

		if (filter) {
			if (offset < filter_offset) {
				continue;
			} else if (offset > filter_offset + filter_count) {
				break;
			}
		}

		nt_Tokenizer_reset_text(&column_tokenizer, line_token->begin, line_token->end);
		ArrayMemoryBlock_clear(&tokens);
		while (nt_Tokenizer_next(&column_tokenizer)) {
			ArrayMemoryBlock_append(&tokens, nt_Tokenizer_token_memblock(&column_tokenizer));
		}

		if (latitude1_index >= ArrayMemoryBlock_size(&tokens)
		    || longitude1_index >= ArrayMemoryBlock_size(&tokens)
		    || latitude2_index >= ArrayMemoryBlock_size(&tokens)
		    || longitude2_index >= ArrayMemoryBlock_size(&tokens)) {
			++problems;
			continue;
		}

		f32 lat1, lon1, lat2, lon2;
		MemoryBlock lat1_token = ArrayMemoryBlock_get(&tokens, latitude1_index);
		MemoryBlock lon1_token = ArrayMemoryBlock_get(&tokens, longitude1_index);
		MemoryBlock lat2_token = ArrayMemoryBlock_get(&tokens, latitude2_index);
		MemoryBlock lon2_token = ArrayMemoryBlock_get(&tokens, longitude2_index);
		if (!pt_parse_f32(lat1_token.begin, lat1_token.end, &lat1) ||
		    !pt_parse_f32(lon1_token.begin, lon1_token.end, &lon1) ||
		    !pt_parse_f32(lat2_token.begin, lat2_token.end, &lat2) ||
		    !pt_parse_f32(lon2_token.begin, lon2_token.end, &lon2)) {
			++problems;
			continue;
		}

		if (snap_function) {
			if (!snap_function(&lat1, &lon1)) {
				++problems;
				continue;
			}
			if (!snap_function(&lat2, &lon2)) {
				++problems;
				continue;
			}
		}

// 		Print_clear(print);
// 		Print_cstr(print, "[");
// 		Print_f64(print, (f64) lat);
// 		Print_align(print, 12, 1, ' ');
// 		Print_cstr(print, ",");
// 		Print_f64(print, (f64) lon);
// 		Print_align(print, 12, 1, ' ');
// 		Print_cstr(print, "]");
// 		Print_cstr(print, "\n");
// 		Request_print(request, print);

		/* convert to path using mercator projection */
		u128 path_number  = app_quadtree2_path_number(lat1, lon1, lat2, lon2);

// 		Print_clear(print);
// 		Print_u64(print, path_number);
// 		Print_cstr(print, "\n");
// 		Request_print(request, print);

		if (point_index >= sample_size) {
			/* choose random point to replace */
			rnd_next();
			u64 index = (u64) rnd_state % (point_index+1);
			if (index < sample_size) {
				Array_u128_set(&sample, index, path_number);
			}
		} else {
			Array_u128_append(&sample, path_number);
		}

		++point_index;
	}

	if (Array_u128_size(&sample) > 0) {

		/* sort sample and return the k-th moments */
		qsort_u128(sample.begin, sample.end);
		if (!check_sorted_u128(sample.begin, sample.end)) {
			Print_clear(print);
			Print_cstr(print, "PROBLEM IN SORTING!\n");
			Request_print(request, print);
		}

// 		if (uniq) {
// 			sample.end = u64_remove_duplicates(sample.begin, sample.end);
// 			if (!check_sorted_uniqueness_u64(sample.begin, sample.end)) {
// 				Print_clear(print);
// 				Print_cstr(print, "PROBLEM IN UNIQUENESS!\n");
// 				Request_print(request, print);
// 			}
// 		}

		u64 n = Array_u128_size(&sample);
		f64 coef = (f64) n / (k+1);
		for (u64 i=0;i<k;++i) {
			u64 index = (u64) ((i+1) * coef);
			u128 value = Array_u128_get(&sample, index);
			Print_clear(print);
			Print_u64(print, value.high);
			Print_char(print,' ');
			Print_u64(print, value.low);
			Print_char(print,'\n');
			Request_print(request, print);
		}
	}

	platform.free_memory(&sample_memory);
	platform.close_mmap_file(&mapped_file);

	if (snap_function) {
		platform.close_mmap_file(&snap_mapped_file);
	}

}

internal void
service_qpcount(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	b8  filter = 0;
	u64 filter_offset = 0;
	u64 filter_count  = 0; // zero means no constraint
	if (op_Options_find_cstr(options,"-filter")) {
		if (!op_Options_named_u64_cstr(options,"-filter",0,&filter_offset)
		    ||!op_Options_named_u64_cstr(options,"-filter",1,&filter_count)) {
			Request_msg(request, "[csv] invalid offset or count in option -filter.\n");
			Request_msg(request, "[csv] option usage: -filter=<offset>,<count>\n");
			return;
		} else {
			filter = 1;
		}
	}

	pt_MappedFile              snap_mapped_file;
	cm_SnappingLatLonFunction *snap_function = 0;
	if (op_Options_find_cstr(options,"-snap")) {
		if (!op_Options_named_f32_cstr(options,"-snap",1,&g_snap_maxdist)) {
			Request_msg(request, "[csv] missing maxdist\n");
			Request_msg(request, "[csv] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
		}
		MemoryBlock roadmap_filename = {.begin=0, .end=0};
		if (!op_Options_named_str_cstr(options,"-snap",0, &roadmap_filename)) {
			Request_msg(request, "[csv] -snap: missing roadmap filename\n");
			Request_msg(request, "[csv] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		snap_mapped_file = platform.open_mmap_file(roadmap_filename.begin, roadmap_filename.end, 1, 0);
		if (!snap_mapped_file.mapped) {
			Request_msg(request, "[csvsnap] couldn't open snap file\n");
			Request_msg(request, "[csv] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		/* set graph */
		al_Allocator *allocator = (al_Allocator*) snap_mapped_file.begin;
		g_snap_graph = (rg_Graph*) al_Allocator_get_root(allocator);
		if (!g_snap_graph) {
			Request_msg(request, "[csvsnap] couldn't find snap graph inside roadmap file\n");
			Request_msg(request, "[csv] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
			return;
		}
		snap_function = g_snap;
	}



	MemoryBlock csv_filename = { .begin=0, .end=0 };

	app_QPart qpart;
	app_QPart_init(&qpart);
	if (!op_Options_str(options,1,&csv_filename)
	    || !op_Options_str(options,2,&qpart.filename)
	    || !op_Options_str(options,3,&qpart.lat_name)
	    || !op_Options_str(options,4,&qpart.lon_name)) {
		Request_msg(request, "[csv] problem with required parameters.\n");
		Request_msg(request, "[csv] option usage: qpcount <csv-filename> <qpart-filename> <latloc> <loncol>\n");
		return;
	} else {
		qpart.active = 1;
		if (!app_QPart_read(&qpart)) {
			Request_msg(request, "[csv] -qpart problem (could not read u64 entries from input file).\n");
			Request_msg(request, "[csv] option usage: -qpart=<qpart-filename>,<include-bitset>,<latcol>,<loncol>\n");
			return;
		}
	}

	for (;;) {

		// if (spec is valid) -> start reading the input csv file
		pt_Memory ftok_buffer_memory = platform.allocate_memory(Kilobytes(64), 12, 0);


		pt_File csv_file = platform.open_read_file(csv_filename.begin, csv_filename.end);

		nu_FileTokenizer ftok;
		nu_FileTokenizer_init(&ftok,
				      '\n',
				      ftok_buffer_memory.memblock.begin,
				      ftok_buffer_memory.memblock.end,
				      &csv_file);

		pt_Memory tokens_array_memory = platform.allocate_memory(Kilobytes(4), 12, 0);

		nu_TokensArray tokens;
		nu_TokensArray_init(&tokens,
				    tokens_array_memory.memblock.begin,
				    tokens_array_memory.memblock.end);

		nu_TokensArray tokens_tokens;

		// parse tokens
		if (nu_FileTokenizer_next(&ftok)) {
			nu_TokensArray_parse(&tokens,',',ftok.token.begin, ftok.token.end);
			if (!tokens.parse_overflow) {
				nu_TokensArray_split(&tokens, &tokens_tokens);
			} else {
				Request_msg(request, "[csv] parse overflow when reading tokens tokens.\n");
				break;
			}
		} else {
			Request_msg(request, "[csv] tokens line not found.");
			break;
		}

		/* get name of columns */
		if (qpart.active) {
			s32 idx;
			idx = nu_TokensArray_find(&tokens_tokens, qpart.lat_name.begin, qpart.lat_name.end);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart latitude column not found.\n");
				return;
			}
			qpart.lat_index = (u32) idx;
			idx = nu_TokensArray_find(&tokens_tokens, qpart.lon_name.begin, qpart.lon_name.end);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart longitude column not found.\n");
				return;
			}
			qpart.lon_index = (u32) idx;
		}

		u64 counts[1024];
		pt_fill((char*) counts, (char*) counts + sizeof(counts), 0);

		// insert records
		u64 records_inserted = 0;
		s64 offset  = -1;
		u64 line_no =  1;
		while (nu_FileTokenizer_next(&ftok)) {
			++offset;
			++line_no;

			if (filter) {
				if (offset < filter_offset) {
					continue;
				} else if (offset >= filter_offset + filter_count) {
					break;
				}
			}
			nu_TokensArray_parse(&tokens, ',', ftok.token.begin, ftok.token.end);

			f32 lat=0.0f, lon=0.0f;
			if (!app_QPart_read_latlon(&qpart, tokens.begin, tokens.end, &lat, &lon)) {
				continue;
			}

			if (snap_function) {
				if (!(*snap_function)(&lat, &lon)) {
					continue;
				}
			}
			u32 part_number = app_QPart_part_number(&qpart, lat, lon);
			Assert(part_number < 1024);
			++counts[part_number];
			++records_inserted;
		}

		platform.close_file(&csv_file);
		platform.free_memory(&tokens_array_memory);
		platform.free_memory(&ftok_buffer_memory);

		// write to file

		Print_clear(print);
		for (s64 i=1;i<=(qpart.cut_end - qpart.cut_begin + 1);++i) {
			Print_cstr(print, "[qpcount] records in part ");
			Print_u64(print, (u64) i);
			Print_align(print, 4, 1, ' ');
			Print_cstr(print, " -> ");
			Print_u64(print, counts[i]);
			Print_align(print, 12, 1, ' ');
			Print_u64(print, (counts[i]*100)/records_inserted);
			Print_align(print, 12, 1, ' ');
			Print_cstr(print, "%\n");

		}
		Print_cstr(print, "[qpcount] records inserted ");
		Print_u64(print, records_inserted);
		Print_cstr(print, "\n");
		Request_print(request,print);
		Print_clear(print);
		break;
	}

	//
	// the sequence of index and measure dimension
	// will be used to apply the right dispatch the
	// right conversion procedure (text to paths)
	// as well as specify the nanocube vector schema.
	//

}

/* @TODO(llins): maybe move this to the platform */
internal b8
app_create_tmp_file(MemoryBlock filename, u64 size)
{
	Assert(size > 0);
	pt_File file = platform.open_write_file(filename.begin, filename.end);
	if (!file.open) {
		return 0;
	}
	char ch = 0;
	platform.seek_file(&file, size - 1);
	platform.write_to_file(&file, &ch, &ch + 1);
	platform.close_file(&file);
	return 1;
}

internal void
service_test_file_backed_mmap(Request *request)
{

	Print      *print   = request->print;
	op_Options *options = &request->options;

	u64 prealloc_memory = 4;
	if (op_Options_find_cstr(options,"-gigs")) {
		if (!op_Options_named_u64_cstr(options,"-gigs",0,&prealloc_memory)) {
			Request_msg(request, "invalid memory value on -gigs (default: 4GB)\n");
			return;
		}
	}

	MemoryBlock filename     = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &filename)) {
		Request_msg(request, "no filename provided\n");
		return;
	}

	f64 time = platform.get_time();

	app_create_tmp_file(filename, Gigabytes(prealloc_memory));

	time = platform.get_time() - time;

	Print_clear(print);
	Print_cstr(print, "saved file ");
	Print_str(print, filename.begin, filename.end);
	Print_cstr(print, " in ");
	Print_f64(print, time);
	Print_cstr(print, "s.\n");
	Request_print(request, print);

	time = platform.get_time();
	pt_MappedFile file2 = platform.open_mmap_file(filename.begin, filename.end, 1, 1);
	if (!file2.mapped) {
		Request_msg(request, "could not map file for reading/writing\n");
	}
	u64 size = file2.size/sizeof(u64);
	u64 *it = (u64*) file2.begin;
	for (u64 i=0;i<size;++i) {
		*it = i;
		++it;
	}
	time = platform.get_time() - time;
	Print_clear(print);
	Print_cstr(print, "wrote to file ");
	Print_str(print, filename.begin, filename.end);
	Print_cstr(print, " in ");
	Print_f64(print, time);
	Print_cstr(print, "s.\n");
	Request_print(request, print);

	time = platform.get_time();
	platform.close_mmap_file(&file2);
	time = platform.get_time() - time;
	Print_clear(print);
	Print_cstr(print, "unmapped file ");
	Print_str(print, filename.begin, filename.end);
	Print_cstr(print, " in ");
	Print_f64(print, time);
	Print_cstr(print, "s.\n");
	Request_print(request, print);

}

/* csv */
internal void
service_create_usage(Request *request, char *preamble_cstr)
{
	Print *print   = request->print;
	if (preamble_cstr) {
		Print_clear(print);
		Print_cstr(print, "[create] ");
		Print_cstr(print, preamble_cstr);
		Print_char(print,'\n');
		Request_print(request,print);
	}
// 	Request_msg(request, "[csv] usage:  csv <csv-fname> <mapping-fname> <output-fname>\n");
// 	Request_msg(request, "[csv] usage:  csv -stdin <mapping-fname> <output-fname>\n");
// 	Request_msg(request, "[csv] option: -filter=<offet>,<count>\n");
// 	Request_msg(request, "[csv] option: -snap=<roadmap-file>,<maxdist>\n");
// 	Request_msg(request, "[csv] option: -qpart=<qpart-filename>,<include-bitset>,<latcol>,<loncol> \n");
// 	Request_msg(request, "[csv] option: -qpart2=<qpart2-filename>,<include-bitset>,<latcol1>,<loncol1>,<latcol2>,<loncol2> \n");
// 	Request_msg(request, "[csv] option: -tpart2=<tpart2-filename>,<include-bitset>,<latcol1>,<loncol1>,<latcol2>,<loncol2> \n");
// 	Request_msg(request, "[csv] missing maxdist\n");
// 	Request_msg(request, "[csv] option usage: -snap=<roadmap-filename>,<maxdist-f32>\n");
}

internal b8
service_create_save_arena(Request *request, al_Allocator *allocator, char *filename_begin, char *filename_end, u64 part_number)
{
	Print *print = request->print;

	/* save current arena and create a smaller one from scratch for the new records */
	FilePath filepath;
	FilePath_init(&filepath, filename_begin, filename_end);

	if (part_number > 0) {
		Print_clear(print);
		Print_str(print, filepath.full_path, filepath.extension);
		Print_cstr(print,"-");
		Print_u64(print,part_number);
		Print_align(print, 3, 1, '0');
		Print_str(print,filepath.extension, filepath.end);
		FilePath_init(&filepath, print->begin, print->end);
	}

	pt_File output_file = platform.open_write_file(filepath.full_path, filepath.end);
	if (!output_file.open) {
		Print_clear(print);
		Print_cstr(print,"[csv] couldn't open file to write: ");
		Print_str(print,filepath.full_path, filepath.end);
		Print_cstr(print,"\n");
		Request_print(request, print);
		return 0;
	}

	/* fit memory to used pages */
	al_Allocator_fit(allocator);

	char *begin = (char*) allocator;
	u64  used_memory = al_Allocator_used_memory(allocator);
	if (!platform.write_to_file(&output_file, begin, begin + used_memory)) {
		Print_clear(print);
		Print_cstr(print,"[csv] couldn't write to file ");
		Print_str(print,filepath.full_path, filepath.end);
		Print_cstr(print,"\n");
		Print_cstr(print,"[csv] when trying to write ");
		Print_u64(print,used_memory);
		Print_cstr(print," bytes\n");
		Request_print(request, print);
		return 0;
	}

	return 1;
}

internal void
service_create_print_temporal_hint(Print *print, nm_TimeBinning *time_binning)
{
	tm_Label time_label;
	tm_Label_init(&time_label, time_binning->base_time);
	Print_clear(print);
	Print_cstr(print,"temporal|");
	tm_Label_print(&time_label, print);
	Print_format(print, "_%ds", time_binning->bin_width);
}

//
// if previous part nanocube is available is not null, copy
// the key value store from it. Otherwise assume creating from
// scratch.
//
internal al_Allocator*
service_create_prepare_allocator_and_nanocube(Request *request, cm_Spec *spec, char *data_memory_begin, char *data_memory_end, nv_Nanocube *previous_part)
{
	Print *print = request->print;

	al_Allocator *allocator      = al_Allocator_new(data_memory_begin, data_memory_end);

	// watermark new allocator
	MemoryBlock watermark = al_Allocator_watermark_area(allocator);
	Print print_watermark;
	Print_init(&print_watermark, watermark.begin, watermark.end);
	Print_cstr(&print_watermark, nanocube_executable_version_doc);
	Print_char(&print_watermark, 0);

	al_Cache     *nanocube_cache = al_Allocator_create_cache(allocator, "nv_Nanocube", sizeof(nv_Nanocube));
	nv_Nanocube  *nanocube       = (nv_Nanocube*) al_Cache_alloc(nanocube_cache);
	nv_Nanocube_init(nanocube);
	nv_Nanocube_init_key_value_store(nanocube, allocator);

	// set root object of the allocator as the new empty nanocube
	al_Allocator_set_root(allocator, nanocube);

	if (previous_part == 0) {
		// setup nanocube vector dimensions
		for (s32 i=0;i<cm_Spec_dimensions(spec);++i) {
			cm_Dimension *dim = cm_Spec_get_dimension(spec,i);
			if (dim->type == cm_INDEX_DIMENSION) {
				switch(dim->mapping_spec.index_mapping.type) {
				case cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE: {
					u8 levels = dim->mapping_spec.index_mapping.latlon.depth;
					nv_Nanocube_insert_index_dimension(nanocube, 2, levels, dim->name.begin, dim->name.end);
					nv_Nanocube_set_dimension_hint_cstr(nanocube, dim->name.begin, dim->name.end, "spatial", print);
				} break;
				case cm_INDEX_MAPPING_IP_HILBERT: {
					u8 levels = dim->mapping_spec.index_mapping.ip_hilbert.depth;
					nv_Nanocube_insert_index_dimension(nanocube, 2, levels, dim->name.begin, dim->name.end);
					nv_Nanocube_set_dimension_hint_cstr(nanocube, dim->name.begin, dim->name.end, "spatial:ip", print);
				} break;
				case cm_INDEX_MAPPING_TIME: {
					u8 levels = dim->mapping_spec.index_mapping.time.depth;
					nv_Nanocube_insert_index_dimension(nanocube, 1, levels, dim->name.begin, dim->name.end);
#if 1
					/* TODO(llins): cleanup this insertion of key value */
					Print_clear(print);
					Print_str(print,dim->name.begin, dim->name.end);
					Print_char(print,':');
					Print_cstr(print,"time_binning");
					/* copy bytes of the representation of base_time */
					char *key_end = print->end;
					nm_TimeBinning *time_binning = &dim->mapping_spec.index_mapping.time.time_binning;
					for (s32 j=0;j<sizeof(nm_TimeBinning);++j) {
						Print_char(print,*((char*)time_binning + j));
					}
					nv_Nanocube_insert_key_value(nanocube, print->begin, key_end, key_end, print->end);

					Print_clear(print);
					service_create_print_temporal_hint(print, time_binning);
					nv_Nanocube_set_dimension_hint(nanocube, dim->name.begin, dim->name.end, print->begin, print->end, print);
#endif
				} break;
				case cm_INDEX_MAPPING_HOUR: {
					u8 levels = 1;
					// 5 bits per level (0-32 fits 0-23)
					nv_Nanocube_insert_index_dimension(nanocube, 5, levels, dim->name.begin, dim->name.end);
					nv_Nanocube_set_dimension_hint_cstr(nanocube, dim->name.begin, dim->name.end, "categorical", print);
				} break;
				case cm_INDEX_MAPPING_WEEKDAY: {
					u8 levels = 1;
					// 5 bits per level (0-32 fits 0-23)
					nv_Nanocube_insert_index_dimension(nanocube, 3, levels, dim->name.begin, dim->name.end);
					/* @TODO(llins): insert names of the dates */
					nv_Nanocube_set_dimension_hint_cstr(nanocube, dim->name.begin, dim->name.end, "categorical", print);
				} break;
				case cm_INDEX_MAPPING_CATEGORICAL: {
					nv_Nanocube_insert_index_dimension(nanocube, dim->mapping_spec.index_mapping.categorical.bits,
									   dim->mapping_spec.index_mapping.categorical.levels, dim->name.begin,
									   dim->name.end);
					nv_Nanocube_set_dimension_hint_cstr(nanocube, dim->name.begin, dim->name.end, "categorical", print);
				}
				default: {
				} break;
				}
			} else if (dim->type == cm_MEASURE_DIMENSION) {
				nv_Nanocube_insert_measure_dimension(nanocube, dim->mapping_spec.measure_mapping.storage_type,
								     dim->name.begin, dim->name.end);
			}
		}
	} else {
		// no hints are stored here, just initialize the dimensions
		// setup nanocube vector dimensions
		for (s32 i=0;i<cm_Spec_dimensions(spec);++i) {
			cm_Dimension *dim = cm_Spec_get_dimension(spec,i);
			if (dim->type == cm_INDEX_DIMENSION) {
				switch(dim->mapping_spec.index_mapping.type) {
				case cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE: {
					u8 levels = dim->mapping_spec.index_mapping.latlon.depth;
					nv_Nanocube_insert_index_dimension(nanocube, 2, levels, dim->name.begin, dim->name.end);
				} break;
				case cm_INDEX_MAPPING_IP_HILBERT: {
					u8 levels = dim->mapping_spec.index_mapping.ip_hilbert.depth;
					nv_Nanocube_insert_index_dimension(nanocube, 2, levels, dim->name.begin, dim->name.end);
				} break;
				case cm_INDEX_MAPPING_TIME: {
					u8 levels = dim->mapping_spec.index_mapping.time.depth;
					nv_Nanocube_insert_index_dimension(nanocube, 1, levels, dim->name.begin, dim->name.end);
				} break;
				case cm_INDEX_MAPPING_HOUR: {
					u8 levels = 1;
					// 5 bits per level (0-32 fits 0-23)
					nv_Nanocube_insert_index_dimension(nanocube, 5, levels, dim->name.begin, dim->name.end);
				} break;
				case cm_INDEX_MAPPING_WEEKDAY: {
					u8 levels = 1;
					// 5 bits per level (0-32 fits 0-23)
					nv_Nanocube_insert_index_dimension(nanocube, 3, levels, dim->name.begin, dim->name.end);
				} break;
				case cm_INDEX_MAPPING_CATEGORICAL: {
					nv_Nanocube_insert_index_dimension(nanocube, dim->mapping_spec.index_mapping.categorical.bits,
									   dim->mapping_spec.index_mapping.categorical.levels, dim->name.begin,
									   dim->name.end);
				}
				default: {
				} break;
				}
			} else if (dim->type == cm_MEASURE_DIMENSION) {
				nv_Nanocube_insert_measure_dimension(nanocube, dim->mapping_spec.measure_mapping.storage_type, dim->name.begin, dim->name.end);
			}
		}

		// copy key_value_store
		bt_Iter it;
		bt_Iter_init(&it, &previous_part->key_value_store);
		bt_Hash hash;
		MemoryBlock key;
		MemoryBlock value;
		while ( bt_Iter_next(&it, &hash, &key, &value) ) {
			nv_Nanocube_insert_key_value(nanocube, key.begin, key.end, value.begin, value.end);
		}
	}

	// initialize index dimensions data structure
	// within the nanocube vector
	nv_Nanocube_init_index(nanocube, allocator);

#if 0
	Print_clear(print);
	Print_cstr(print, "Fresh nanocube address: ");
	Print_u64(print, (u64) nanocube);
	Print_cstr(print, "\n");
	Request_print(request, print);
#endif

	return allocator;
}

csv_PULL_CALLBACK(service_create_pull_callback)
{
	pt_File *file = (pt_File*) user_data;

	if (file->eof) {
		return 0;
	}

	platform.read_next_file_chunk(file, buffer, buffer + length);
	Assert(file->last_read <= length);

// 	Print *print = &debug_request->print;
// 	Print_clear(print);
// 	Print_cstr(print, "[service_create_test_pull_callback] buffer length: ");
// 	Print_u64(print, length);
// 	Print_cstr(print, "\n");
// 	Request_print(debug_request, print);

	if (file->last_read > 0) {
		return buffer + file->last_read;
	} else if (file->eof) {
		/* indicates eof */
		return 0;
	} else {
		/* weird path */
		return buffer + file->last_read;
	}
}

#define service_create_PARSE_BUFFER_SIZE_MAPPING_CSV Megabytes(32)
// #define service_create_SCAN_CSV_BUFFER Megabytes(4)
#define service_create_MAX_FIELDS 1024

internal void
app_util_fill_nx_Array_with_path_from_u64(nx_Array *target, u8 bits, u8 levels, u64 value)
{
	Assert(bits <= 8);
	Assert(levels == target->length);
	u32 mod = (1 << bits);
	for (s32 lev=0;lev<levels;++lev) {
		nx_Label label = (nx_Label) ((value >> ((levels - 1 - lev) * bits)) % mod);
		nx_Array_set(target, lev, label);
	}
}

#define NANOCUBE_SERVE_WHILE_CREATE


#ifdef NANOCUBE_SERVE_WHILE_CREATE

#define service_create_serve_NOT_INITIALIZED 0
#define service_create_serve_RUNNING 1
#define service_create_serve_FAILED 2
#define service_create_serve_DONE 3

#define service_create_serve_refresh_NO_REFRESH 0
#define service_create_serve_refresh_LAST_REFRESH 1
#define service_create_serve_refresh_INTERMEDIATE_REFRESH 2

typedef struct {
	s32 num_threads;
	s32 port;
	app_NanocubesAndAliases *info;
	Request *request;
	volatile u32 status;
	// refresh status and data
	//
	// when refresh status is 1 then
	// the server should update the index
	//
	volatile u32 refresh_status;
	al_Allocator *refresh_data;
} service_create_ServeConfig;

PLATFORM_WORK_QUEUE_CALLBACK(service_create_serve)
{
	service_create_ServeConfig *serve_config = (service_create_ServeConfig*) data;
	Request *request = serve_config->request;
	Print   *print   = request->print;
	app_NanocubesAndAliases  *serve_info = serve_config->info;

	static char *alias_name = "x";

	// @todo rename serve data to serve buffers
	ServeData serve_data = {
		.buffers = (serve_QueryBuffers*) BasicAllocator_alloc(&serve_info->blocks, serve_config->num_threads * sizeof(serve_QueryBuffers),3),
		.request = serve_config->request,
		.num_buffers = serve_config->num_threads,
		.pause_and_active_count = 0
	};
	Assert(serve_data.buffers);
	nv_payload_services_init(&serve_data.payload_services);

	for (u32 i=0;i<serve_config->num_threads;++i) {

		serve_QueryBuffers *buffer = serve_data.buffers + i;
		buffer->context = &serve_data;

		// reserve heap memory for ast, types, and symbols
		MemoryBlock parse_and_compile_buffer   = BasicAllocator_alloc_memblock(&serve_info->blocks, app_service_serve_MEM_COMPILER,            3);
		MemoryBlock table_index_columns_buffer = BasicAllocator_alloc_memblock(&serve_info->blocks, app_service_serve_MEM_TABLE_INDEX_COLUMNS, 3);
		MemoryBlock table_value_columns_buffer = BasicAllocator_alloc_memblock(&serve_info->blocks, app_service_serve_MEM_TABLE_VALUE_COLUMNS, 3);
		MemoryBlock print_result_buffer        = BasicAllocator_alloc_memblock(&serve_info->blocks, app_service_serve_MEM_PRINT_RESULT,        3);
		MemoryBlock print_header_buffer        = BasicAllocator_alloc_memblock(&serve_info->blocks, app_service_serve_MEM_PRINT_HEADER,        3);
		MemoryBlock http_channel_buffer        = BasicAllocator_alloc_memblock(&serve_info->blocks, app_service_serve_MEM_HTTP_CHANNEL,        3);

		/* initialize compiler allocator */
		BilinearAllocator_init(&buffer->compiler_allocator, parse_and_compile_buffer.begin, parse_and_compile_buffer.end);

		/* init compiler */
		np_Compiler_init(&buffer->compiler, &buffer->compiler_allocator);
		nv_Compiler_init(&buffer->compiler);

		/* init table_index_columns_allocator */
		LinearAllocator_init(&buffer->table_index_columns_allocator, table_index_columns_buffer.begin, table_index_columns_buffer.begin, table_index_columns_buffer.end);

		/* init table_value_columns_allocator*/
		LinearAllocator_init(&buffer->table_value_columns_allocator, table_value_columns_buffer.begin, table_value_columns_buffer.begin, table_value_columns_buffer.end);

		/* init print_result */
		Print_init(&buffer->print_result, print_result_buffer.begin, print_result_buffer.end);

		/* init print_result */
		Print_init(&buffer->print_header, print_header_buffer.begin, print_header_buffer.end);

		/* insert symbol into compiler pointing to a measure linked to the current nanocube */
		Assert(serve_info->num_nanocubes == 1);
		nv_Compiler_insert_nanocube(&buffer->compiler, serve_info->nanocubes[0], serve_info->aliases[0].begin, serve_info->aliases[0].end);

		/* initialize parser table */
		np_initialize_tokenizer(&buffer->scanner, 0, 0);
		np_Parser_init(&buffer->parser, &buffer->scanner, buffer->compiler.memory);

		// mark checkpoint on compiler as the fresh
		// starting point for new compilations
		buffer->compiler_chkpt = np_Compiler_checkpoint(&buffer->compiler);

		/* initialize http channel memory */
		http_Channel_init(&buffer->http_channel, http_channel_buffer.begin, http_channel_buffer.end,
				  service_serve_http_request_line_callback, service_serve_http_header_field_callback,
				  buffer); /* user data will be the buffer*/

		/* initialize socket */
		buffer->socket = 0;
		buffer->payload_services = &serve_data.payload_services;
	}

	pt_TCP tcp = platform.tcp_create();
	Assert(tcp.handle);

	// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port, void *user_data, PlatformTCPCallback *callback, u32 *status)
	pt_TCP_Feedback feedback;
	platform.tcp_serve(tcp, serve_config->port, (void*) &serve_data, serve_request_handler, &feedback);

	// should now be available
	platform.tcp_process_events(tcp, 0);

	if (feedback.status != pt_TCP_SOCKET_OK) {
		pt_memory_barrier();
		pt_atomic_exchange_u32(&serve_config->status, service_create_serve_FAILED);
		return;
	}

	pt_WorkQueue *work_queue = 0;
	if (serve_config->num_threads > 1) {
		work_queue = platform.work_queue_create(serve_config->num_threads);
	}

	pt_memory_barrier();
	pt_atomic_exchange_u32(&serve_config->status, service_create_serve_RUNNING);

	while (!global_app_state->interrupted) {

		platform.tcp_process_events(tcp, work_queue);

		// is there any request to update the served data?
		if (serve_config->refresh_status == service_create_serve_refresh_LAST_REFRESH) {

			//
			// we should replace the nanocube in the compiler engine with the
			// given one, but first we need to make sure that no query is being
			// solved with the current nanocube.
			//
			// once all current queries finish, replace index, release previous
			// active index and resume query solving engine
			//

			for (;;) {
				// keep trying to pause the queries
				u64 current_value = serve_data.pause_and_active_count;
				u64 new_value     = current_value | app_ServeData_PAUSE_MASK;
				u64 what_was_there = pt_atomic_cmp_and_swap_u64(&serve_data.pause_and_active_count, current_value, new_value);
				if (what_was_there == current_value) {
					break;
				}
			}

			// wait for the active count to get to zero
			for (;;) {
				// keep trying to pause the queries
				u64 current_value = serve_data.pause_and_active_count;
				if ((current_value & app_ServeData_ACTIVE_COUNT_MASK) == 0) {
					// yes we are safe
					break;
				} else {
					platform.thread_sleep(1);
				}
			}

			// we are safe to swap the old nanocube with the new one
			for (u32 i=0;i<serve_config->num_threads;++i) {
				serve_QueryBuffers *buffer = serve_data.buffers + i;
				nv_Nanocube *nanocube = (nv_Nanocube*) al_Allocator_get_root(serve_config->refresh_data);
				nv_Compiler_update_singleton_nanocube_symbol(&buffer->compiler, nanocube, alias_name, cstr_end(alias_name));
			}

			// we can actually erase all the content
			{
				u64 what_was_there = pt_atomic_cmp_and_swap_u64(&serve_data.pause_and_active_count, app_ServeData_PAUSE_MASK, 0);

				// we only go here because we got to zero
				Assert(what_was_there == app_ServeData_PAUSE_MASK);
			}

			// free the nanocube stored in the first slot of serve info
			app_NanocubesAndAliases_free_and_pop_nanocube(serve_info, 0);

			//
			serve_config->refresh_data = 0;

			// this will signal the update procedure that the refresh signal
			// was processed. note that they work in a synced fashion
			serve_config->refresh_status = service_create_serve_refresh_NO_REFRESH;

		} else if (serve_config->refresh_status == service_create_serve_refresh_INTERMEDIATE_REFRESH) {

			nv_Nanocube *nanocube = (nv_Nanocube*) al_Allocator_get_root(serve_config->refresh_data);

			b8 copy_ok = app_NanocubesAndAliases_copy_and_register_nanocube(serve_info, nanocube, alias_name, cstr_end(alias_name));

			Assert(serve_info->num_nanocubes == 2);

			nv_Nanocube *nanocube_copy = serve_info->nanocubes[1]; // serve_info->num_nanocubes-1];

			if (!copy_ok) {

				Request_msg(request, "[create] PROBLEM: not enough memory to copy updated index to serve area.");

				serve_config->refresh_data = 0;

				pt_memory_barrier();

				serve_config->refresh_status = service_create_serve_refresh_NO_REFRESH;

			} else {

				//
				// we should replace the nanocube in the compiler engine with the
				// given one, but first we need to make sure that no query is being
				// solved with the current nanocube.
				//
				// once all current queries finish, replace index, release previous
				// active index and resume query solving engine
				//

				for (;;) {
					// keep trying to pause the queries
					u64 current_value = serve_data.pause_and_active_count;
					u64 new_value     = current_value | app_ServeData_PAUSE_MASK;
					u64 what_was_there = pt_atomic_cmp_and_swap_u64(&serve_data.pause_and_active_count, current_value, new_value);
					if (what_was_there == current_value) {
						break;
					}
				}

				// wait for the active count to get to zero
				for (;;) {
					// keep trying to pause the queries
					u64 current_value = serve_data.pause_and_active_count;
					if ((current_value & app_ServeData_ACTIVE_COUNT_MASK) == 0) {
						// yes we are safe
						break;
					} else {
						platform.thread_sleep(1);
					}
				}

				// we are safe to swap the old nanocube with the new one
				for (u32 i=0;i<serve_config->num_threads;++i) {
					serve_QueryBuffers *buffer = serve_data.buffers + i;
					nv_Compiler_update_singleton_nanocube_symbol(&buffer->compiler, nanocube_copy, alias_name, cstr_end(alias_name));
				}

				// we can actually erase all the content
				{
					u64 what_was_there = pt_atomic_cmp_and_swap_u64(&serve_data.pause_and_active_count, app_ServeData_PAUSE_MASK, 0);

					// we only go here because we got to zero
					Assert(what_was_there == app_ServeData_PAUSE_MASK);
				}

				// free the nanocube stored in the first slot of serve info
				app_NanocubesAndAliases_free_and_pop_nanocube(serve_info, 0);

				//
				serve_config->refresh_data = 0;

				// this will signal the update procedure that the refresh signal
				// was processed. note that they work in a synced fashion
				serve_config->refresh_status = service_create_serve_refresh_NO_REFRESH;

			}
		}
	}

	/* TODO(llins): somehow there should be a way to signal the server to stop */
	platform.work_queue_destroy(work_queue);

	// cleanup tcp
	platform.tcp_destroy(tcp);

	pt_memory_barrier();
	pt_atomic_exchange_u32(&serve_config->status, service_create_serve_DONE);

}

#endif


/*
BEGIN_DOC_STRING nanocube_csv_doc
Ver:   __VERSION__
Usage: nanocube create INPUT MAPPING OUTPUT [OPTIONS]
       nanocube create -stdin MAPPING OUTPUT [OPTIONS]
Create a nanocube index from INPUT .csv file (or stdin)
using the mapping configuration specified in the MAPPING file
and writes the resulting nanocube index in the OUTPUT file.

Possible OPTIONS:

    -size0=S
         initial nanocube index file size. This size doubles
	 everytime the usage gets closer to the current capacity.
	 Once the building process finishes the resulting index
	 file shrinks to the actual used pages. Example values for S:
	 4M, 16M, 256M, 1G.
    -max-size=S
         every time we need to increase the current capacity (initially
	 equals to size0), we check to see if the new capacity exceeds
	 max-size. If so, we create a new nanocube index file.
    -scan-buffer-size=S
         read S bytes before processing input records. Note that this
	 buffer needs to fit the largest record size (.csv row bytes).
	 The default is 4MB, which might be slow for slow input
	 streams.
    -filter=OFFSET,COUNT
         insert csv records in the range [OFFSET,OFFSET+COUNT).
	 Note this is zero-based. The first .csv record corresponds
	 to OFFSET==0.
    -header=F
         if the -header file option is defined, then we assume the
	 input .csv file has no header and that the header is on the
	 the separate file given by F.
    -sep=S
         set separator of columns in both header and INPUT. Default
	 separator is comma ','.
    -report-frequency=N
         report progress every N records.
    -report-cache={0,1}
         show cache utilization on every report (slower).
    -serve=port
         port to serve cube while building and after finished.
    -serve-refresh-rate=SECONDS
         a large memory copy operation will be performed every SECONDS
	 to install a copy of the current NC index in a new safe memory
	 section. Note that this should not interrupt the current server
	 from solving queries.
    -serve-threads=N
         number of threads for solving queries.

The MAPPING file consists of a series of specifications of which index
and measure dimensions we want the output nanocube index to have based
on the columns of the input .csv file

    index_dimension(NAME, INPUT, INDEX_SPEC)
        NAME        is the name of the nanocube dimension.
        INPUT       identifies the .csv column names that will be used
       	            as input by the MAPSPEC rule to generate the info for
		    the nanocube index dimension

	            input()              # no input columns needed
		    input('lat','lon')   # two input values coming
                                         # from columns 'lat' and
                                         # 'lon'
	INDEX_SPEC  identifies (possibly-parameterized) the index dimension
	            encoding (flat-tree, binary tree, quad-tree, k-ary tree)
		    and resolution (number of levels), and how to map input
		    values into 'bins' in this dimension.

		    categorical(B,L)
		    categorical(B,L,S)
		        flat-tree with B bits of resolution (eg. 8) in L
			levels. expects one input column and every distinct
			value that appears in the .csv input column is mapped
			into a unique number with L digits in {0,1,...,2^(B)-1}.
			Numbers are automatically generated by their appearence
			order in the .csv file.

			S is a string encoding an alias table. In its simplest
			form (A), we just name nodes on the deepest level of the
			hierarchy. In its more elaborate form (B) we allow
			naming intermediate nodes.
			(A)
				in_lbl_1 <nl>
				in_lbl_2 <nl>
				...
				in_lbl_n <nl>
			or
				in_lbl_1 <tab> out_lbl_1 <nl>
				in_lbl_2 <nl>
				...
				in_lbl_n <tab> out_lbl_1 <nl>
			or
				in_lbl_1_1 <tab> ... <tab> in_lbl_1_k <tab> out_lbl_1 <nl>
				in_lbl_2 <nl>
				...
				in_lbl_n <tab> out_lbl_n <nl>
			(B)
				@hierarchy
				alias_root
				<tab> alias_level_1
				<tab> <tab> alias_level_2
				<tab> <tab> <tab> input_text (== alias_level_3)
				<tab> <tab> <tab> input_text_1 <tab> input_text_2 <tab> alias_leaf_level
				<tab> <tab> <tab> input_text (== alias_level_3)
				<tab> alias_level_1
				<tab> <tab> alias_level_2
				<tab> <tab> <tab> input_text (== alias_level_3)
				<tab> <tab> <tab> input_text (== alias_level_3)
			or
				@hierarchy
				<tab> alias_level_1
				<tab> <tab> <tab> input_text (== alias_level_3)
				<tab> <tab> <tab> input_text_1 <tab> input_text_2 <tab> alias_leaf_level
				<tab> <tab> <tab> input_text (== alias_level_3)
				<tab> alias_level_1
				<tab> <tab> <tab> input_text (== alias_level_3)
				<tab> <tab> <tab> input_text (== alias_level_3)

		    latlon(L)
		        creates a quad-tree with L levels using the mercator
			projection. Expects two input columns with floating
			pointing numbers for latitude and longitude.

		    time(L,BASE,WIDTH_SECS,OFFSET_SECS)
		        creates a binary-tree with L levels. Expects one input
			column with a string that is convertible to a timestamp.
			Some accepted formats are:
		           '2000-01-01T00:00:00-06:00.125'
		           '2000-01-01T00:00:00-06:00'
		           '2000-01-01T00:00-06:00'
		           '2000-01-01T00:00'
		           '2000-01-01T00'
		           '2000-01-01'
                        It uses BASE the timestamp (also in a format from the
			above) as the alignment point for temporal bins. The
			bins have width given in seconds: WIDTH_SECS; and a
			conversion is possibly applied to align cases where
			data will come for instance in local time and we want
			to correct it to UTC.

		    unixtime(L,BASE,WIDTH_SECS,OFFSET_SECS)
			Analogous to time, but instead of expecting a date
			and time string, it expects a string with the number
			of seconds since unix epoch (1970-01-01 UTC).

		    ip(L)
		        creates a quad-tree with L levels mapping IPv4 entries
			(eg. 123.122.122.98) into a corresponding entry using the
			hilbert space-filling curve convention.

    measure_dimension(NAME, INPUT, MEASURE_SPEC)
        NAME and INPUT are the same as in the index_dimension.
	MEASURE_SPEC
	    either a primitive scalar type like
	    signed integer, unsigned integer, or floating point
	    numbers with 32/64 bits. Or a pre-defined function to
	    convert input columns into something meaningful

	    u32, u64
	    f32, f64
	    row_bitset()

    file(F)
	reads content of file F as a string. Can be used together
	with categorical index dimensions to point to alias mapping
	descriptions (see categorical).

Here is an example of a MAPPING file (it accepts line comments using #)

    # I1. quadtree index dimension for the pickup location
    index_dimension('pickup_location',input('pickup_latitude','pickup_longitude'),latlon(25));
    # I2. quadtree index dimension for the dropoff location
    index_dimension('dropoff_location',input('dropoff_latitude','dropoff_longitude'),latlon(25));
    # I3. quadtree index dimension for the dropoff location
    index_dimension('pickup_time', input('tpep_pickup_datetime'), time(17,'2009-01-01T00:00:00-05:00',3600,5*60));
    # I4. weekday of pickup
    index_dimension('weekday', input('tpep_pickup_datetime'), weekday());
    # I5. hour of pickup
    index_dimension('hour', input('tpep_pickup_datetime'), hour());

    # M1. measure dimension for counting  (u32 means integral non-negative
    #     and 2^32 - 1 max value) if no input .csv column is used in a measure
    #     dimention, it functions as a count (ie. one for each input record)
    measure_dimension('count',input(),u32);
    # M2. duration
    measure_dimension('duration',input('tpep_pickup_datetime','tpep_dropoff_datetime') ,duration(f32,60));
    # M3. duration squared (for stddev computations)
    measure_dimension('duration2',input('tpep_pickup_datetime','tpep_dropoff_datetime') ,duration2(f32,60));
    # M4. Map fare_amount into a 32-bit floating point value
    measure_dimension('fare',input('fare_amount'),f32);


END_DOC_STRING
*/

internal void
service_create(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	pt_MappedFile              snap_mapped_file;
	cm_SnappingLatLonFunction *snap_function = 0;

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_csv_doc);
		Request_print(request, print);
		return;
	}

	if (op_Options_find_cstr(options,"-snap")) {

		if (!op_Options_named_f32_cstr(options,"-snap",1,&g_snap_maxdist)) {
			service_create_usage(request, "missing maxdist");
			return;
		}

		MemoryBlock roadmap_filename = {.begin=0, .end=0};
		if (!op_Options_named_str_cstr(options,"-snap",0, &roadmap_filename)) {
			service_create_usage(request, "missing roadmap filename (-snap)");
			return;
		}

		snap_mapped_file = platform.open_mmap_file(roadmap_filename.begin, roadmap_filename.end, 1, 0);
		if (!snap_mapped_file.mapped) {
			service_create_usage(request, "couldn't open snap file (-snap)");
			return;
		}

		/* set graph */
		al_Allocator *allocator = (al_Allocator*) snap_mapped_file.begin;
		g_snap_graph = (rg_Graph*) al_Allocator_get_root(allocator);

		if (!g_snap_graph) {
			service_create_usage(request, "couldn't find snap graph inside roadmap file (-snap)");
			return;
		}

		snap_function = g_snap;

	}

	s64 size0 = Megabytes(16);
	if (op_Options_find_cstr(options,"-size0")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-size0",0,&st)) {
			service_create_usage(request, "invalid memory value on -size0 (default: 16M)");
			return;
		} else {
			size0 = ut_parse_storage_size(st.begin, st.end);
			if (size0 < Megabytes(4)) {
				service_create_usage(request, "invalid memory value on -size0 (needs to be at least 4M)");
				return;
			}
		}
	}

	b8 detail = op_Options_find_cstr(options, "-detail") != 0;

	s64 max_file_size = Terabytes(16);
	if (op_Options_find_cstr(options,"-max-size")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-max-size",0,&st)) {
			service_create_usage(request, "invalid memory value on -max-size (default: 16T)");
			return;
		} else {
			max_file_size = ut_parse_storage_size(st.begin, st.end);
			if (max_file_size < size0) {
				service_create_usage(request, "invalid memory value on -max-size (needs to be at least size0)");
				return;
			}
		}
	}

	s64 scan_buffer_size = Megabytes(4);
	if (op_Options_find_cstr(options,"-scan-buffer-size")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-scan-buffer-size",0,&st)) {
			service_create_usage(request, "invalid scan buffer size (default: 4M)");
			return;
		} else {
			scan_buffer_size = ut_parse_storage_size(st.begin, st.end);
			if (scan_buffer_size < 128) {
				service_create_usage(request, "scan buffer size of at least 128 bytes");
				return;
			}
		}
	}
	// add space for separator indices for the csv scan procedure
	scan_buffer_size += sizeof(u32) * service_create_MAX_FIELDS;

	u64 report_frequency = 100000;
	if (op_Options_find_cstr(options,"-report-frequency")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-report-frequency",0,&st)) {
			service_create_usage(request, "invalid report frequency value (default: 100000)");
			return;
		} else {
			if (!pt_parse_u64(st.begin, st.end, &report_frequency)) {
				service_create_usage(request, "invalid report frequency value (default: 100000)");
				return;
			}
		}
	}

	u64 report_cache = 1;
	if (op_Options_find_cstr(options,"-report-cache")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-report-cache",0,&st)) {
			service_create_usage(request, "invalid report cache value (default: 1)");
			return;
		} else {
			if (!pt_parse_u64(st.begin, st.end, &report_cache)) {
				service_create_usage(request, "invalid report cache value (default: 1)");
				return;
			}
		}
	}

	b8  filter = 0;
	u64 filter_offset = 0;
	u64 filter_count  = 0; // zero means no constraint
	if (op_Options_find_cstr(options,"-filter")) {
		if (!op_Options_named_u64_cstr(options,"-filter",0,&filter_offset)
		    ||!op_Options_named_u64_cstr(options,"-filter",1,&filter_count)) {
			service_create_usage(request, "invalid filter offset or count values");
			return;
		} else {
			filter = 1;
		}
	}

	app_QPart qpart;
	app_QPart_init(&qpart);
	if (op_Options_find_cstr(options,"-qpart")) {
		if (!op_Options_named_str_cstr(options,"-qpart",0,&qpart.filename)
		    || !op_Options_named_u64_cstr(options,"-qpart",1,&qpart.set)
		    || !op_Options_named_str_cstr(options,"-qpart",2,&qpart.lat_name)
		    || !op_Options_named_str_cstr(options,"-qpart",3,&qpart.lon_name)) {
			service_create_usage(request, "invalid -qpart");
			return;
		} else {
			qpart.active = 1;
			if (!app_QPart_read(&qpart)) {
				service_create_usage(request, "-qpart problem (could not read u64 entries from input file)");
				return;
			}
		}
	}

	app_QPart2 qpart2;
	app_QPart2_init(&qpart2);
	if (op_Options_find_cstr(options,"-qpart2")) {
		if (!op_Options_named_str_cstr(options,"-qpart2",0,&qpart2.filename)
		    || !op_Options_named_u64_cstr(options,"-qpart2",1,&qpart2.set)
		    || !op_Options_named_str_cstr(options,"-qpart2",2,&qpart2.lat1_name)
		    || !op_Options_named_str_cstr(options,"-qpart2",3,&qpart2.lon1_name)
		    || !op_Options_named_str_cstr(options,"-qpart2",4,&qpart2.lat2_name)
		    || !op_Options_named_str_cstr(options,"-qpart2",5,&qpart2.lon2_name)) {
			service_create_usage(request, "-qpart2 problem");
			return;
		} else {
			qpart2.active = 1;
			if (!app_QPart2_read(&qpart2)) {
				service_create_usage(request, "qpart2 problem (could not read u64 entries from input file)");
				return;
			}
		}
	}

	if (qpart.active && qpart2.active) {
		service_create_usage(request, "only one of the options can be active: -qpart or -qpart2");
		return;
	}

	app_TPart2 tpart2;
	app_TPart2_init(&tpart2);
	if (op_Options_find_cstr(options,"-tpart2")) {
		if (!op_Options_named_str_cstr(options,"-tpart2",0,&tpart2.filename)
		    || !op_Options_named_u64_cstr(options,"-tpart2",1,&tpart2.set)
		    || !op_Options_named_str_cstr(options,"-tpart2",2,&tpart2.lat1_name)
		    || !op_Options_named_str_cstr(options,"-tpart2",3,&tpart2.lon1_name)
		    || !op_Options_named_str_cstr(options,"-tpart2",4,&tpart2.lat2_name)
		    || !op_Options_named_str_cstr(options,"-tpart2",5,&tpart2.lon2_name)) {
			service_create_usage(request, "problem with -tpart2 parameters");
			return;
		} else {
			tpart2.active = 1;
			if (!app_TPart2_read(&tpart2)) {
				service_create_usage(request, "problem with -tpart2 parameters");
				return;
			}
		}
	}

	b8 use_stdin = op_Options_find_cstr(options, "-stdin") != 0;

	MemoryBlock csv_filename     = { .begin=0, .end=0 };
	MemoryBlock mapping_filename = { .begin=0, .end=0 };
	MemoryBlock output_filename  = { .begin=0, .end=0 };

	if (use_stdin) {
		if (op_Options_num_positioned_parameters(options) != 3) {
			service_create_usage(request, "csv with -stdin uses two required parameters: <mapping-fname> <output-fname>");
			return;
		} else if (!op_Options_str(options, 1, &mapping_filename)
			   || !op_Options_str(options, 2, &output_filename)) {
			service_create_usage(request, "[csv] couldn't parse csv -stdin required params");
			return;
		}
	} else {
		if (op_Options_num_positioned_parameters(options) != 4) {
			service_create_usage(request, "csv without -stdin uses three required parameters: <csv-fname> <mapping-fname> <output-fname>");
			return;
		} else if (!op_Options_str(options, 1, &csv_filename)
			   || !op_Options_str(options, 2, &mapping_filename)
			   || !op_Options_str(options, 3, &output_filename)) {
			service_create_usage(request, "couldn't parser three parameters for csv without -stdin");
			return;
		}
	}
	pt_Memory mapping_text;
	mapping_text.handle = 0;

	/* .csv header comes from an external file */
	b8 external_header = 0;
	MemoryBlock external_header_filename = { .begin = 0, .end = 0 };
	if (op_Options_find_cstr(options, "-header")) {
		if (!op_Options_named_str_cstr(options, "-header", 0, &external_header_filename)) {
			service_create_usage(request, "invalid header option: missing filename;");
			return;
		} else {
			external_header = 1;
		}
	}

	/* set filed separator character */
	char sep = ',';
	if (op_Options_find_cstr(options, "-sep")) {
		MemoryBlock st = {.begin = 0, .end = 0};
		if (!op_Options_named_str_cstr(options, "-sep", 0, &st)) {
			service_create_usage(request, "invalid header option: missing filename;");
			return;
		} else if (MemoryBlock_length(&st) > 1) {
			service_create_usage(request, "-sep should be one character only");
			return;
		} else {
			sep = *st.begin;
		}
	}

	/* try loading the whole file into a buffer */
	{
		pt_File pfh = platform.open_read_file(mapping_filename.begin, mapping_filename.end);
		if (!pfh.open) {
			Request_msg(request, "[csv] could not open mapping filename.\n");
			return;
		}

		if (pfh.size==0) {
			Request_msg(request, "[csv] file is empty.\n");
			return;
		}
		mapping_text = platform.allocate_memory(pfh.size,0,0);
		platform.read_next_file_chunk(&pfh, mapping_text.memblock.begin, mapping_text.memblock.end);
		platform.close_file(&pfh);
	}

	// initialize an empty csv to schema mapping spec
	cm_Spec spec;
	cm_Spec_init(&spec);

	/* this memory will hold the cm_Dimension objects */
	pt_Memory csv_mapping_parse_and_compile_buffer = platform.allocate_memory(service_create_PARSE_BUFFER_SIZE_MAPPING_CSV, 0, 0);
	BilinearAllocator csv_mapping_parse_and_compile_allocator;
	BilinearAllocator_init(&csv_mapping_parse_and_compile_allocator, csv_mapping_parse_and_compile_buffer.memblock.begin, csv_mapping_parse_and_compile_buffer.memblock.end);
	np_Compiler csv_mapping_compiler;
	np_Compiler_init(&csv_mapping_compiler, &csv_mapping_parse_and_compile_allocator);
	cm_init_compiler_csv_mapping_infrastructure(&csv_mapping_compiler);
	cm_compiler_register_number_storage_types(&csv_mapping_compiler);
	np_Parser csv_mapping_parser;
	/* same AST parser as the query language of nanocube */
	nt_Tokenizer tok;
	np_initialize_tokenizer(&tok, mapping_text.memblock.begin, mapping_text.memblock.end);
	np_Parser_init(&csv_mapping_parser, &tok, &csv_mapping_parse_and_compile_allocator);

	/* dummy loop just to goto relese resources bt using break */
	for (;;) {

		// parse
		if (!np_Parser_run(&csv_mapping_parser)) {
			Request_msg(request, "[csv] parsing error.\n");
			Request_print(request, &csv_mapping_parser.log);
			break;
		}

		// compile
		np_Compiler_reduce(&csv_mapping_compiler, csv_mapping_parser.ast_first, mapping_text.memblock.begin, mapping_text.memblock.end);
		if (!csv_mapping_compiler.reduce.success) {
			Request_print(request, &csv_mapping_compiler.reduce.log);
			Request_msg(request, "[csv] compile error.\n");
			break;
		}

		// cm_Spec
		np_TypeValueList *it = csv_mapping_compiler.reduce.statement_results;
		while (it) {
			if (it->data.type_id == cm_compiler_types.dimension_id) {

				cm_Dimension *dim = (cm_Dimension*) (it->data.value);
				cm_Spec_insert_dimension(&spec, dim);


			} // found a dimension
			it = it->next;
		}

		if (!cm_Spec_is_valid(&spec)) {
			Request_msg(request, "[csv] csv mapping spec is"
				    " incompatible with a nanocube: needs at"
				    " least one indexing and one measure"
				    " dimension");
			break;
		}

		// if (spec is valid) -> start reading the input csv file
		pt_Memory ftok_buffer_memory = platform.allocate_memory(Kilobytes(64), 12, 0);


		pt_File csv_file;
		if (!use_stdin) {
			csv_file = platform.open_read_file(csv_filename.begin, csv_filename.end);
			if (!csv_file.open) {
				service_create_usage(request, "couldn't open csv file.");
				return;
			}
		}
		pt_File *csv_file_ptr = use_stdin ? request->pfh_stdin : &csv_file;


		csv_Stream       csv_stream;
		/* every record extracted from the .csv file has its fields pointed to by this array */
		MemoryBlock      csv_fields[service_create_MAX_FIELDS];
		/* copy header fields into the content buffer */
		char             csv_header_content_buffer[Kilobytes(4)];
		/* keep links to field names using the csv_header_fields */
		MemoryBlock      csv_header_fields_storage[service_create_MAX_FIELDS];
		ArrayMemoryBlock csv_header_fields;
		ArrayMemoryBlock_init(&csv_header_fields, csv_header_fields_storage, csv_header_fields_storage + service_create_MAX_FIELDS);
		pt_Memory csv_buffer = platform.allocate_memory(scan_buffer_size,3,0);

		/* reader header file either from csv file or from a header file */
		{
			pt_File csv_header_file;
			if (external_header) {
				csv_header_file = platform.open_read_file(external_header_filename.begin, external_header_filename.end);
				if (!csv_header_file.open) {
					service_create_usage(request, "couldn't open external header file;");
					return;
				}
				csv_Stream_init(&csv_stream, sep,
						csv_buffer.memblock.begin, csv_buffer.memblock.end - csv_buffer.memblock.begin,
						service_create_MAX_FIELDS, service_create_pull_callback, &csv_header_file);
			} else {
				/* initialize csv_stream to csv_file */
				csv_Stream_init(&csv_stream, sep,
						csv_buffer.memblock.begin, csv_buffer.memblock.end - csv_buffer.memblock.begin,
						service_create_MAX_FIELDS, service_create_pull_callback, csv_file_ptr);
			}

			/* read header fields */
			if (csv_Stream_next(&csv_stream)) {
				u32 csv_fields_count = csv_Stream_num_fields(&csv_stream);
				if (csv_fields_count > service_create_MAX_FIELDS) {
					service_create_usage(request, ".csv header has too many fields.");
					return;
				}
				b8 ok = csv_Stream_get_fields(&csv_stream, csv_fields, 0, csv_fields_count);
				if (!ok) {
					service_create_usage(request, "couldn't read .csv header fields.");
					return;
				}

				Print header_print;
				Print_init(&header_print, csv_header_content_buffer, csv_header_content_buffer + sizeof(csv_header_content_buffer));
				for (u32 i=0;i<csv_fields_count;++i) {
					char *field_begin = header_print.end;
					// TODO(llins): maybe it is the right time to un-2dqute or un-escape
					Print_str(&header_print, csv_fields[i].begin, csv_fields[i].end);
					csv_fields[i].begin = field_begin;
					csv_fields[i].end   = header_print.end;
					ArrayMemoryBlock_append(&csv_header_fields, csv_fields[i]);
					Print_char(&header_print,0);
				}

				if (header_print.overflow) {
					service_create_usage(request, "couldn't copy header fields.");
					return;
				}
			} else {
				service_create_usage(request, "couldn't read .csv header;");
				return;
			}

			if (external_header) {
				platform.close_file(&csv_header_file);

				/* initialize csv_stream to csv_file */
				csv_Stream_init(&csv_stream, sep,
						csv_buffer.memblock.begin, csv_buffer.memblock.end - csv_buffer.memblock.begin,
						service_create_MAX_FIELDS, service_create_pull_callback, csv_file_ptr);
			}
		}

#define service_create_which_field(name) \
		pt_MemoryBlock_find_first_match(csv_header_fields.begin, csv_header_fields.end, name.begin, name.end);

		if (qpart.active) {
			s64 idx;
			idx = service_create_which_field(qpart.lat_name);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart latitude column not found.\n");
				return;
			}
			qpart.lat_index = (u32) idx;
			idx = service_create_which_field(qpart.lon_name);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart longitude column not found.\n");
				return;
			}
			qpart.lon_index = (u32) idx;
		}

		if (qpart2.active) {
			s32 idx;
			idx = service_create_which_field(qpart2.lat1_name);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart2 latitude1 column not found.\n");
				return;
			}
			qpart2.lat1_index = (u32) idx;
			idx = service_create_which_field(qpart2.lon1_name);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart2 longitude1 column not found.\n");
				return;
			}
			qpart2.lon1_index = (u32) idx;
			idx = service_create_which_field(qpart2.lat2_name);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart2 latitude2 column not found.\n");
				return;
			}
			qpart2.lat2_index = (u32) idx;
			idx = service_create_which_field(qpart2.lon2_name);
			if (idx < 0) {
				Request_msg(request, "[csv] qpart2 longitude2 column not found.\n");
				return;
			}
			qpart2.lon2_index = (u32) idx;
		}

		if (tpart2.active) {
			s32 idx;
			idx = service_create_which_field(tpart2.lat1_name);
			if (idx < 0) {
				Request_msg(request, "[csv] tpart2 latitude1 column not found.\n");
				return;
			}
			tpart2.lat1_index = (u32) idx;
			idx = service_create_which_field(tpart2.lon1_name);
			if (idx < 0) {
				Request_msg(request, "[csv] tpart2 longitude1 column not found.\n");
				return;
			}
			tpart2.lon1_index = (u32) idx;
			idx = service_create_which_field(tpart2.lat2_name);
			if (idx < 0) {
				Request_msg(request, "[csv] tpart2 latitude2 column not found.\n");
				return;
			}
			tpart2.lat2_index = (u32) idx;
			idx = service_create_which_field(tpart2.lon2_name);
			if (idx < 0) {
				Request_msg(request, "[csv] tpart2 longitude2 column not found.\n");
				return;
			}
			tpart2.lon2_index = (u32) idx;
		}

		//
		// check if all spec columns exist in the tokens
		// and annotate its index
		//
		/* store max_idx of a columns used */
		s64 max_idx = -1;

		b8 ok = 1;
		Print_clear(print);
		for (s32 i=0;i<cm_Spec_dimensions(&spec);++i) {
			cm_Dimension *dim = cm_Spec_get_dimension(&spec,i);
			cm_ColumnRef *it_cols  = dim->input_columns.begin;
			cm_ColumnRef *end = dim->input_columns.end;
			while (it_cols != end) {
				if (it_cols->is_index) {
					// @TODO(llins): implement numberic
					// column references
					Assert(0 && "Not Implemented");
				} else if (it_cols->is_name) {
					s32 idx = service_create_which_field(it_cols->name);
					if (idx < 0) {
						Print_str(print,it_cols->name.begin,it_cols->name.end);
						Print_char(print,'\n');
						ok = 0;
					} else {
						it_cols->index = (b8) idx;
						it_cols->is_index = 1;
						max_idx = MAX(max_idx,idx);
					}
				}
				++it_cols;
			}
		}
		if (!ok) {
			char *chkpt = print->end;
			Print_cstr(print,"[csv] Columns not found:\n");
			pt_rotate(print->begin, chkpt, print->end);
			Request_print(request, print);
			break;
		}

		// @TODO(llins): prepare the schema of the nanocube
		// vector and initialize one to start loading records

		// @TODO(llins): provide better mechanism than pre-allocating
		// lot of RAM without any support to go beyond this initial
		// allocated memory.
		// pt_Memory data_memory = platform.allocate_memory(Gigabytes(prealloc_memory), 12, Terabytes(2));

		/* make it file backed */
		s64 size = size0;
#define xxxxCSV_USING_MMAPED_SHARED_FILE
#ifdef CSV_USING_MMAPED_SHARED_FILE
		if (!app_create_tmp_file(output_filename, size)) {
			Print_cstr(print,"[csv] couldn't create output file\n");
			break;
		}
		pt_MappedFile mmap_output = platform.open_mmap_file(output_filename.begin, output_filename.end, 1, 1);
		if (!mmap_output.mapped) {
			Print_cstr(print,"[csv] couldn't mmap output file\n");
			break;
		}
		char *data_memory_begin = mmap_output.begin;
		char *data_memory_end   = mmap_output.begin + mmap_output.size;
#else
		pt_Memory data_memory = platform.allocate_memory(size, 12, 0);
		char *data_memory_begin = data_memory.memblock.begin;
		char *data_memory_end   = data_memory.memblock.end;
		// pt_fill(data_memory_begin, data_memory_end, 0);
		u64 part_number         = 1;
#endif

		al_Allocator *allocator = service_create_prepare_allocator_and_nanocube(request, &spec, data_memory_begin, data_memory_end, 0);
		nv_Nanocube  *nanocube  = (nv_Nanocube*) al_Allocator_get_root(allocator);

		pt_Memory insert_buffer_memory = platform.allocate_memory(sizeof(nx_Threads) + Megabytes(1), 12, 0);
		// ~4MB of buffer for insertion:  sizeof(Threads): 4259856

		// reserve space for the sequence of paths
		// and references to those paths
		nx_Label           paths[Kilobytes(2)];
		nx_LabelArrayList  address[nx_MAX_NUM_DIMENSIONS]; // some max dimensions
		nx_Label          *it_paths = &paths[0];
		nx_LabelArrayList *it_addr  = address;
		nx_LabelArrayList *end_addr = address + nx_MAX_NUM_DIMENSIONS;
		// setup nanocube vector dimensions
		for (s32 i=0;i<cm_Spec_dimensions(&spec);++i) {
			cm_Dimension *dim = cm_Spec_get_dimension(&spec,i);
			if (dim->type == cm_INDEX_DIMENSION) {
				switch(dim->mapping_spec.index_mapping.type) {
				case cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE: {
					Assert(it_addr != end_addr);
					u8 levels = dim->mapping_spec.index_mapping.latlon.depth;
					if (it_addr != address) {
						(it_addr - 1)->next = it_addr;
					}
					it_addr->next = 0;
					nx_Array_init(&it_addr->labels, it_paths, levels);
					it_paths += levels;
					++it_addr;
				} break;
				case cm_INDEX_MAPPING_IP_HILBERT: {
					Assert(it_addr != end_addr);
					u8 levels = dim->mapping_spec.index_mapping.ip_hilbert.depth;
					if (it_addr != address) {
						(it_addr - 1)->next = it_addr;
					}
					it_addr->next = 0;
					nx_Array_init(&it_addr->labels, it_paths, levels);
					it_paths += levels;
					++it_addr;
				} break;
				case cm_INDEX_MAPPING_TIME: {
					Assert(it_addr != end_addr);
					u8 levels = dim->mapping_spec.index_mapping.time.depth;
					if (it_addr != address) {
						(it_addr - 1)->next = it_addr;
					}
					it_addr->next = 0;
					nx_Array_init(&it_addr->labels, it_paths, levels);
					it_paths += levels;
					++it_addr;
				} break;
				case cm_INDEX_MAPPING_HOUR: {
					Assert(it_addr != end_addr);
					if (it_addr != address) {
						(it_addr - 1)->next = it_addr;
					}
					u8 levels = 1;
					it_addr->next = 0;
					nx_Array_init(&it_addr->labels, it_paths, levels);
					it_paths += levels;
					++it_addr;
				} break;
				case cm_INDEX_MAPPING_WEEKDAY: {
					Assert(it_addr != end_addr);
					if (it_addr != address) {
						(it_addr - 1)->next = it_addr;
					}
					u8 levels = 1;
					it_addr->next = 0;
					nx_Array_init(&it_addr->labels, it_paths, levels);
					it_paths += levels;
					++it_addr;
				}
				case cm_INDEX_MAPPING_CATEGORICAL: {
					Assert(it_addr != end_addr);
					if (it_addr != address) {
						(it_addr - 1)->next = it_addr;
					}
					u8 levels = dim->mapping_spec.index_mapping.categorical.levels;
					it_addr->next = 0;
					nx_Array_init(&it_addr->labels, it_paths, levels);
					it_paths += levels;
					++it_addr;
				} break;
				default: {
				} break;
				}
			}
		}


#ifdef NANOCUBE_SERVE_WHILE_CREATE

		//
		// copy the current empty nanocube into a query only region
		// create serve threads, create connection/listen thread,
		// open port with the listen thread
		//
		// we need a mechanism to sync all the query threads when
		// we flip the switch on which cube is available
		//

		u64 serve_num_threads = 1;
		if (op_Options_find_cstr(options,"-serve-threads")) {
			if (!op_Options_named_u64_cstr(options,"-serve-threads",0,&serve_num_threads)) {
				Request_msg(request, "[create] incorrect usage of options: -serve-threads=<num-threads>\n");
				return;
			}
		}

		u64 serve_port = 0;
		if (op_Options_find_cstr(options,"-serve-port")) {
			if (!op_Options_named_u64_cstr(options,"-serve-port",0,&serve_port)) {
				Request_msg(request, "[create] incorrect usage of options: -serve-port=<num-port>\n");
				return;
			}
		}

		u64 serve_refresh_rate = 60; // sixty seconds is the default
		if (op_Options_find_cstr(options,"-serve-refresh-rate")) {
			if (!op_Options_named_u64_cstr(options,"-serve-refresh-rate",0,&serve_refresh_rate)) {
				Request_msg(request, "[create] incorrect usage of options: -serve-refresh-rate=<seconds>\n");
				return;
			}
		}


		// @continue
		app_NanocubesAndAliases serve_info;
		app_NanocubesAndAliases_init(&serve_info);

		service_create_ServeConfig serve_config = {
			.info = &serve_info,
			.num_threads = (s32) serve_num_threads,
			.port = (s32) serve_port,
			.request = request,
			.status = service_create_serve_NOT_INITIALIZED,
			.refresh_status = service_create_serve_refresh_NO_REFRESH,
			.refresh_data = 0
		};

		pt_WorkQueue *serve_work_queue = 0;

		u64 serve_last_update = 0;

		// create a work queue and schedule the serve
		// procedure to run

		if (serve_port > 0) {

			char *name = "x";

			b8 copy_ok = app_NanocubesAndAliases_copy_and_register_nanocube(&serve_info, nanocube, name, cstr_end(name));

			if (!copy_ok) {
				Request_msg(request, "[create] not enough memory to create and serve.\n");
				return;
			}

			serve_work_queue = platform.work_queue_create(1);

			if (!serve_work_queue) {
				Request_msg(request, "[create] could not create work queue for server listener loop.\n");
				return;
			}

			// schedule thread to initialize and start serving
			platform.work_queue_add_entry(serve_work_queue, service_create_serve, &serve_config);

			u32 status = service_create_serve_NOT_INITIALIZED;
			for (;;) {
				status = serve_config.status;
				if (status == service_create_serve_NOT_INITIALIZED) {
					platform.thread_sleep(10);
				} else {
					break;
				}
			}

			if (status == service_create_serve_FAILED) {
				Print_clear(print);
				Print_cstr(print,"[create] failed to initialize 'serve' engine.\n");
				Request_print(request, print);
				return;
			} else if (status == service_create_serve_RUNNING) {
				Print_clear(print);
				Print_format(print,"[create] 'serve' engine running on port %d with %d threads.\n", serve_config.port, serve_config.num_threads);
				Request_print(request, print);
			} else if (status == service_create_serve_DONE) {
				Print_clear(print);
				Print_format(print,"[create] 'serve' engine is done. didn't expect it to happen.\n");
				Request_print(request, print);
				return;
			}

			serve_last_update = platform.get_time();

		}

#endif

		// reserve space for the payload (it should be aligned as
		// the way we store the numbers in the nanocube)
		char payload[sizeof(f64) * nv_MAX_MEASURE_DIMENSIONS];

		f64 t0 = platform.get_time();

		ntp_Parser time_parser;
		ntp_Parser_init(&time_parser);

#ifdef PROFILE
		pf_begin();
#endif

		//----------------------------------------------------------
		// process aliases
		//----------------------------------------------------------

		// @note we are assuming that there is enough space to load alias tables
		it_addr  = address;
		for (s32 dim_index=0;dim_index<cm_Spec_dimensions(&spec);++dim_index) {
			cm_Dimension *dim = cm_Spec_get_dimension(&spec,dim_index);


			/* associate a set on the categorical dimensions */
			if (dim->mapping_spec.mapping_type == cm_INDEX_DIMENSION_MAPPING &&
			    dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_CATEGORICAL) {

				//
				// TODO(llins): make sure we
				//     1. release this memory
				//     2. have a set_Set grow mechanism to accomodate all
				//        the different names.
				//     for now assume we can fit everything in 16M
				// @leak
				//
				pt_Memory dim_memory = platform.allocate_memory(Megabytes(16),3,0);

				set_Set *set = (set_Set*) dim_memory.memblock.begin;
				u32 set_offset = RALIGN(sizeof(set_Set),8);
				char *set_buffer_begin = dim_memory.memblock.begin + set_offset;
				char *set_buffer_end   = dim_memory.memblock.end;
				set_Set_init(set, set_buffer_begin, set_buffer_end - set_buffer_begin);

				// allocate memory and set
				dim->user_data = set;

				// pre-populate the set with values defined in the mapping file
				if (cm_Mapping_categorical_defined_with_labels_table(&dim->mapping_spec)) {

					MemoryBlock labels_table = dim->mapping_spec.index_mapping.categorical.labels_table;

					b8 from_file = dim->mapping_spec.index_mapping.categorical.is_file;

					pt_MappedFile mapped_file;
					if (from_file) {
						mapped_file = platform.open_mmap_file(labels_table.begin, labels_table.end, 1, 0);
						if (!mapped_file.mapped) {
							Print_clear(print);
							Print_cstr(print, "Couldn't open labels table file '");
							Print_str(print, labels_table.begin, labels_table.end);
							Print_cstr(print, "' for dimension '");
							Print_str(print, dim->name.begin, dim->name.end);
							Print_cstr(print, "'\n");
							Request_print(request, print);
							return;
						}
						labels_table.begin = mapped_file.begin;
						labels_table.end   = mapped_file.begin + mapped_file.size;
					}

					char *it  = labels_table.begin;
					char *end = labels_table.end;


					// depending on the first line we have two modes of interpretation
					// to the input table

					// TODO(llins): make it more robust to support \r\n
					char *end_of_first_line = pt_find_char(it, end, '\n');

					b8 mode_normal = pt_compare_memory_cstr(it, end_of_first_line, "@hierarchy") != 0;

					// dim info
					u8 bits   = dim->mapping_spec.index_mapping.categorical.bits;
					u8 levels = dim->mapping_spec.index_mapping.categorical.levels;

					// storage space for path. guaranteed to fit any path
					u8 path[128];
					nx_Array path_array;
					nx_Array_init(&path_array, path, levels);

					if (mode_normal) {

						// find new line
						while (it < end) {

							// TODO(llins): make it more robust to support \r\n
							char *end_of_line = pt_find_char(it, end, '\n');

							//
							char *first_tab = pt_find_char(it, end_of_line, '\t');

							// simple input: key and value are the same
							if (first_tab == end_of_line) {
								s32 status;
								set_Set_insert(set, it, end_of_line, &status);

								// insert into key_value_store
								u64 value = set->counter;
								app_util_fill_nx_Array_with_path_from_u64(&path_array, bits, levels, set->counter);
								b8 ok = nv_Nanocube_set_dimension_path_name(nanocube, dim->name.begin, dim->name.end,
													    path, levels,
													    it, end_of_line, print);
								Assert(ok && "problem setting dimension path name");

							} else {
								char *last_tab = first_tab;
								char *next_begin = first_tab + 1;
								for (;;) {
									char *next_tab = pt_find_char(next_begin,end_of_line,'\t');
									if (next_tab == end_of_line) {
										break;
									} else {
										last_tab   = next_tab;
										next_begin = next_tab + 1;
									}
								}

								// store value
								set_Block value = set_Set_store_value(set, last_tab + 1, end_of_line);
								app_util_fill_nx_Array_with_path_from_u64(&path_array, bits, levels, set->counter);
								b8 ok = nv_Nanocube_set_dimension_path_name(nanocube, dim->name.begin, dim->name.end,
													    path, levels,
													    last_tab + 1, end_of_line, print);
								Assert(ok && "problem setting dimension path name");

								// now store keys mapping to the same value
								char *it_keys     = it;
								char *tab         = first_tab;
								char *it_keys_end = last_tab + 1;
								while (it_keys != it_keys_end) {
									s32 status;
									set_Set_insert_with_value(set, it_keys, tab, value, &status);
									// if (status == set_INSERT_EXISTS) {
									// break everything with an error message
									// }
									it_keys = tab + 1;
									char *next_tab = pt_find_char(it_keys,it_keys_end,'\t');
								}
							}
							set_Set_increment_counter(set);

							// might exceed end but should be safe
							it = end_of_line + 1;
						}

					} else {

						// hierachical odometer for tracking the right slot
						nu_HOdom hodom;
						nu_HOdom_init(&hodom, bits);

						s32 current_level = 0;
						u64 serial = 0;

						it = end_of_first_line + 1;

						b8 root_alias_defined = 0;


						// find new line
						while (it < end) {

							// TODO(llins): make it more robust to support \r\n
							char *end_of_line = pt_find_char(it, end, '\n');

							u8 line_level = 0;
							while (it != end_of_line && *it == '\t') {
								++line_level;
								++it;
							}

							char *first_tab = pt_find_char(it, end_of_line, '\t');

							// every time we return to
							if (line_level == 0) {
								if (root_alias_defined) {
									Request_msg(request, "[create] @hierarchy: multiple definition of root alias\n");
									return;
								}

								nx_Array_init(&path_array, path, 0);
								b8 ok = nv_Nanocube_set_dimension_path_name(nanocube,
													    dim->name.begin, dim->name.end,
													    it_addr->labels.begin, levels,
													    it, end_of_line, print);
								Assert(ok && "problem setting dimension path name");

							} else {

								b8 ok = nu_HOdom_advance(&hodom, line_level-1);
								if (!ok) {
									Request_msg(request, "[create] @hierarchy: overflow");
									return;
								}

								u64 counter = 0;
								for (s32 i=0;i<line_level;++i) {
									path[i] = pt_safe_s32_u8(hodom.values[i]);
									counter = (counter << bits) + path[i];
								}
								nx_Array_init(&path_array, path, line_level-1);

								if (line_level < levels - 1) {
									if (first_tab != end_of_line) {
										Request_msg(request, "[create] @hierarchy: intermediate levels cannot have multiple tab separated text\n");
										return;
									}

									b8 ok = nv_Nanocube_set_dimension_path_name(nanocube,
														    dim->name.begin, dim->name.end,
														    path, line_level,
														    it, end_of_line, print);
									Assert(ok && "problem setting dimension path name");
								} else {
									set_Set_set_counter(set, counter);
									// register multiple names on the set and
									if (first_tab == end_of_line) {
										s32 status;
										set_Set_insert(set, it, end_of_line, &status);
										b8 ok = nv_Nanocube_set_dimension_path_name(nanocube,
															    dim->name.begin, dim->name.end,
															    path, line_level,
															    it, end_of_line, print);
									} else {
										char *last_tab = first_tab;
										char *next_begin = first_tab + 1;
										for (;;) {
											char *next_tab = pt_find_char(next_begin,end_of_line,'\t');
											if (next_tab == end_of_line) {
												break;
											} else {
												last_tab   = next_tab;
												next_begin = next_tab + 1;
											}
										}

										// store value
										set_Block value = set_Set_store_value(set, last_tab + 1, end_of_line);
										b8 ok = nv_Nanocube_set_dimension_path_name(nanocube,
															    dim->name.begin, dim->name.end,
															    path, line_level,
															    last_tab+1, end_of_line, print);

										// now store keys mapping to the same value
										char *it_keys     = it;
										char *tab         = first_tab;
										char *it_keys_end = last_tab + 1;
										while (it_keys != it_keys_end) {
											s32 status;
											set_Set_insert_with_value(set, it_keys, tab, value, &status);
											// if (status == set_INSERT_EXISTS) {
											// break everything with an error message
											// }
											it_keys = tab + 1;
											char *next_tab = pt_find_char(it_keys,it_keys_end,'\t');
										}

									} // case custom alias different from input

								} // end last dimension case

							} // end of else (dimension > 0)

							// might exceed end but should be safe
							it = end_of_line + 1;

						} // loop through lines of the alias map

					} // end of @hierarchy

					if (from_file) {
						platform.close_mmap_file(&mapped_file);
					}

				} // categorical dimension spec comes with an alias table

			} // categorical dimension
		}

		//----------------
		// insert records
		//----------------

		u64 records_inserted =  0;
		s64 offset           = -1;
		s64 last_printed     = -2;
		u64 line_no          =  external_header ? 1 : 0; // depends a header was used or not
		while (csv_Stream_next(&csv_stream)) {

#if 0
			// simulate slow insertion
			platform.thread_sleep(10000);
			// check every line beign inserted
// 			Print_clear(print);
// 			Print_str(print, csv_stream.buffer.record, csv_stream.buffer.cursor);
// 			Request_print(request, print);
#endif


			if (global_app_state->interrupted) {
				break;
			}

			// make sure we don't increment offset if it is after the filter interval
			if (filter && (offset+1) >= filter_offset + filter_count) {
				break;
			}

			++offset;
			++line_no;

			if (filter && offset < filter_offset) {
				goto finalize_insertion;
			}

#ifdef NANOCUBE_SERVE_WHILE_CREATE

			if (serve_port > 0) {

// 				Print_clear(print);
// 				Print_format(print, "[create] Updating index being served\n");
// 				Request_print(request, print);

				u64 current_time = platform.get_time();
				if (current_time - serve_last_update >= serve_refresh_rate) {
					// time to update the index

					// there might be a server running.
					// signal the serve that a new finalized index is ready
					// to be served and wait for the server to finish before
					// exiting this program
					Assert(serve_work_queue);
					serve_config.refresh_data = allocator;

					pt_memory_barrier();

					pt_atomic_exchange_u32(&serve_config.refresh_status, service_create_serve_refresh_INTERMEDIATE_REFRESH);

					pt_memory_barrier();
					while (serve_config.refresh_status == service_create_serve_refresh_INTERMEDIATE_REFRESH) {
						platform.thread_sleep(1);
					}
					serve_last_update = platform.get_time();
				}

// 				Print_clear(print);
// 				Print_format(print, "[create] Time elapsed to update index being served in seconds: %llu\n", serve_last_update - current_time);
// 				Request_print(request, print);

			}
#endif

			/* read in field positions of the current record from csv stream */
			u32 csv_fields_count = csv_Stream_num_fields(&csv_stream);
			Assert(csv_fields_count <= service_create_MAX_FIELDS);
			if (!csv_Stream_get_fields(&csv_stream, csv_fields, 0, csv_fields_count)) {
				goto finalize_insertion;
			}

			if (qpart.active) {
				f32 lat, lon;
				app_QPart_read_latlon(&qpart, csv_fields, csv_fields + csv_fields_count, &lat, &lon);
				if (snap_function) {
					if (!(*snap_function)(&lat, &lon)) {
						/* couldn't snap point */
						goto finalize_insertion;
					}
				}
				if (!app_QPart_consider_point(&qpart, lat, lon)) {
					goto finalize_insertion;
				}
			}

			if (qpart2.active) {
				f32 lat1, lon1, lat2, lon2;
				app_QPart2_read_latlon(&qpart2, csv_fields, csv_fields + csv_fields_count, &lat1, &lon1, &lat2, &lon2);
				if (snap_function) {
					if (!(*snap_function)(&lat1, &lon1)) {
						/* couldn't snap point */
						goto finalize_insertion;
					}
					if (!(*snap_function)(&lat2, &lon2)) {
						/* couldn't snap point */
						goto finalize_insertion;
					}
				}
				if (!app_QPart2_consider_point(&qpart2, lat1, lon1, lat2, lon2)) {
					goto finalize_insertion;
				}
			}

			if (tpart2.active) {
				f32 lat1, lon1, lat2, lon2;
				app_TPart2_read_latlon(&tpart2, csv_fields, csv_fields + csv_fields_count, &lat1, &lon1, &lat2, &lon2);
				if (snap_function) {
					if (!(*snap_function)(&lat1, &lon1)) {
						/* couldn't snap point */
						goto finalize_insertion;
					}
					if (!(*snap_function)(&lat2, &lon2)) {
						/* couldn't snap point */
						goto finalize_insertion;
					}
				}
				if (!app_TPart2_consider_point(&tpart2, lat1, lon1, lat2, lon2)) {
					goto finalize_insertion;
				}
			}

			/* check if columns we need to access exist */
			if (csv_fields_count <= max_idx) {
				// @TODO(llins): including logging mechanism for
				// malfomed lines
				goto finalize_insertion;
			}

			// pf_BEGIN_BLOCK("PrepareRecord");

			it_addr          = address;
			char *it_payload = payload;
			b8 could_prepare_record = 1;
			for (s32 i=0;i<cm_Spec_dimensions(&spec);++i) {
				cm_Dimension *dim = cm_Spec_get_dimension(&spec,i);
				if (dim->type == cm_INDEX_DIMENSION) {
					b8 lok = 0;
					switch (dim->mapping_spec.index_mapping.type) {
					case cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE: {
						lok = cm_mapping_latlon(dim, &it_addr->labels, csv_fields, snap_function);
					} break;
					case cm_INDEX_MAPPING_IP_HILBERT: {
						lok = cm_mapping_ip_hilbert(dim, &it_addr->labels, csv_fields);
					} break;
					case cm_INDEX_MAPPING_TIME: {
						lok = cm_mapping_time(dim, &it_addr->labels, csv_fields, &time_parser);
					} break;
					case cm_INDEX_MAPPING_HOUR: {
						lok = cm_mapping_hour(dim, &it_addr->labels, csv_fields, &time_parser);
					} break;
					case cm_INDEX_MAPPING_WEEKDAY: {
						lok = cm_mapping_weekday(dim, &it_addr->labels, csv_fields, &time_parser);
					} break;
					case cm_INDEX_MAPPING_CATEGORICAL: {

						// pf_BEGIN_BLOCK("categorical_resolve");

						set_Set *set   = (set_Set*) dim->user_data;
						Assert(dim->input_columns.end - dim->input_columns.begin == 1);
						u32 cat_col_index = dim->input_columns.begin->index;
						u32 bits   = dim->mapping_spec.index_mapping.categorical.bits;
						u32 levels = dim->mapping_spec.index_mapping.categorical.levels;
						Assert(bits * levels < 64);
						u64 max_entries =  1ULL << (bits * levels);
						MemoryBlock *cat_text = csv_fields + cat_col_index;
						set_Entry *entry = set_Set_find(set, cat_text->begin, cat_text->end);

						//
						// TODO(llins): allow for a policy option 'fixed' or 'extensible'
						// if extensible, new values can be included in the mapping.
						// Let it be extensible for now.
						//
						if (!entry) {
							if (set->num_entries < max_entries)  {
								s32 insert_status = 0;
								entry = set_Set_insert(set, cat_text->begin, cat_text->end, &insert_status);
								set_Set_increment_counter(set);
								if (entry && insert_status == set_INSERT_INSERTED) {

									// register into key-value store
									// TODO(llins): move this :kv: encoding to a higher level function nanocube vector
									// procedure. Something like nv_Nanocube_set_path_name. This will
									Print_clear(print);

									u64 value = entry->counter;
									app_util_fill_nx_Array_with_path_from_u64(&it_addr->labels, bits, levels, value);

									b8 ok = nv_Nanocube_set_dimension_path_name(nanocube,
														    dim->name.begin, dim->name.end,
														    it_addr->labels.begin, levels,
														    cat_text->begin, cat_text->end,
														    print);
									Assert(ok && "problem setting dimension path name");

								} else if (insert_status == set_INSERT_FULL) {
									entry = 0;
									Print_clear(print);
									Print_cstr(print, "[Warning] Categorical names buffer is full. Disconsidering record\n");
									Request_print(request, print);
								} else {
									Assert(0 && "should not reach this point");
								}
							} else {
								Print_clear(print);
								Print_cstr(print, "[Warning] Discarding record with category '");
								Print_str(print, cat_text->begin, cat_text->end);
								Print_format(print, "' (max. of %d categories already reached).", max_entries);
								Print_format(print, " [line: %lld]\n", (s64) line_no);
								Request_print(request, print);
							}
						}

						if (!entry) {
							lok = 0;
						} else {
							u64 value = entry->counter;
							// @note in the case a new entry was inserted in the label set, we are
							// initializing it_addr->labels twice. Since we don't expect lot's
							// of new entries in the label sets, we are not avoiding this redundant
							// case.
							app_util_fill_nx_Array_with_path_from_u64(&it_addr->labels, bits, levels, value);
// 							u32 mod   = (1 << bits);
// 							for (s32 lev=0;lev<levels;++lev) {
// 								nx_Label label = (nx_Label) ((value >> ((levels - 1 - lev) * bits)) % mod);
// 								nx_Array_set(&it_addr->labels, lev, label);
// 							}
							lok = 1;
						}

						// pf_END_BLOCK();
					} break;
					default: {
					} break;
					}
					if (!lok) {
						Print_clear(print);
						Print_format(print, "[Warning] Could not prepare record on line %lld because of index dimension '", line_no);
						Print_str(print, dim->name.begin, dim->name.end);
						Print_cstr(print, "'\n");
						Request_print(request, print);
						could_prepare_record = 0;
						break;
					}
					++it_addr;
				} else if (dim->type == cm_MEASURE_DIMENSION) {
					b8 lok = cm_mapping_measure(dim, offset, &it_payload, csv_fields, &time_parser);
					if (!lok) {
						could_prepare_record = 0;
						break;
					}
				}
			}

			// pf_END_BLOCK();

			if (!could_prepare_record) {
// 				Print_clear(print);
// 				Print_cstr(print, "[service_create] Error parsing record on line ");
// 				Print_u64(print,(u64) offset + 1);
// 				Print_cstr(print,"\n");
// 				Request_print(request, print);
// 				Print_clear(print);
				// continue;
				goto finalize_insertion;
			}

			nx_NanocubeIndex_insert(&nanocube->index, address, payload, nanocube, insert_buffer_memory.memblock);
			++records_inserted;


#ifdef PROFILE
			pf_generate_report();
			pf_clear_events();
			Request_print(request, &pf_report.print);
#endif


			/* TODO(llins): check if utilization of output file is above a threshold? */
			/* if it is, grow the file. If there is no more disk space, use truncate. */

			/* if a new set of page */
			u64 growth_capacity = (u64) al_PAGE_SIZE * (allocator->page_capacity - allocator->used_pages);

			/*
			 * if unlucky, each cache might need a new slab,
			 * and the new slab hits a new untouched page range.
                         * An untouched page range needs 96 pages for the slab
                         * mapping. So grossly speaking number of allocator caches
                         * that can grow in an insertion times 384KB plus the pages.
                         * Hand waving method: needs at least 4MB of unused pages
			 */
			if (growth_capacity < Megabytes(4)) {
#ifdef CSV_USING_MMAPED_SHARED_FILE
				/* double the file size */
				f64 time_to_resize = platform.get_time();

				/* TODO(llins): shrink this file. */
				platform.close_mmap_file(&mmap_output);

				Print_clear(print);
				Print_format(print, "[csv] resizing file from %lluM to %lluM\n", size/Megabytes(1), (2*size)/Megabytes(1));
				Request_print(request,print);
				size = 2 * size;
				/* @TODO(llins): check for a clean exit in these return cases */
				if (!platform.resize_file(output_filename.begin, output_filename.end, size)) {
					Print_clear(print);
					Print_cstr(print, "[csv] Couldn't resize file exiting! Partial file should be consistent");
					Request_print(request,print);
					return;
				}

				mmap_output = platform.open_mmap_file(output_filename.begin, output_filename.end, 1, 1);
				if (!mmap_output.mapped) {
					Print_clear(print);
					Print_cstr(print,"[csv] couldn't mmap output file\n");
					Request_print(request, print);
					return;
				}

				allocator = (al_Allocator*) mmap_output.begin;
				if (!al_Allocator_resize(allocator, mmap_output.size)) {
					Print_clear(print);
					Print_cstr(print,"[csv] couldn't resize allocator size (strange)\n");
					Request_print(request, print);
					return;
				}
				nanocube  = (nv_Nanocube*) al_Allocator_get_root(allocator);

				time_to_resize = platform.get_time() - time_to_resize;
				Print_clear(print);
				Print_format(print, "Time to resize in seconds: %llu\n", time_to_resize);
				Request_print(request, print);
#else
				/* try doubling the arena size */
				if (2*size <= max_file_size && platform.resize_memory(&data_memory, 2*size, 12)) {
					size = 2*size;

					Print_clear(print);
					Print_format(print,"[create] resizing arena of part %llu to %llu bytes\n", part_number, size);
					Request_print(request, print);

					data_memory_begin = data_memory.memblock.begin;
					data_memory_end   = data_memory.memblock.end;
					allocator = (al_Allocator*) data_memory_begin;
					if (!al_Allocator_resize(allocator, size)) {
						Print_clear(print);
						Request_msg(request,"[create] couldn't resize allocator size (strange)\n");
						return;
					}
					nanocube  = (nv_Nanocube*) al_Allocator_get_root(allocator);
					/* ready to continue inserting records into this chunk */
				} else {
					/* save current arena and create a smaller one from scratch for the new records */

					/* fit memory to used pages to make arena more consistent */
					al_Allocator_fit(allocator);

					if (!service_create_save_arena(request, allocator, output_filename.begin, output_filename.end, part_number)) {
						Print_clear(print);
						Request_msg(request,"[create] aborting loop\n");
						return;
					}

					u64 used_memory = al_Allocator_used_memory(allocator);
					Print_clear(print);
					Print_format(print,"[create] saved part %llu size %llu bytes\n", part_number, used_memory);
					Request_print(request, print);

					platform.free_memory(&data_memory);

					++part_number;
					size = size0;

					/* TODO(llins): fix the platform API. allocate_memory can fail and that should be handled */
					data_memory = platform.allocate_memory(size, 12, 0);
					data_memory_begin = data_memory.memblock.begin;
					data_memory_end   = data_memory.memblock.end;
					// pt_fill(data_memory_begin, data_memory_end, 0);

					Print_clear(print);
					Print_format(print,"[create] starting a fresh arena for part %llu\n", part_number);
					Request_print(request, print);

					// we need to initialize the key value store of the next part
					// with a copy of the current part. current solution can be
					// slow, but we assume this op doesnt happen often. It will
					// be loaded from mmapped file.

					pt_MappedFile mapped_file = platform.open_mmap_file(output_filename.begin, output_filename.end, 1, 0);
					if (!mapped_file.mapped) {
						Request_msg(request, "[create] couldn't open previous part.\n");
						return;
					}

					al_Allocator*  previous_part_allocator = (al_Allocator*) mapped_file.begin;
					nv_Nanocube*   previous_part_nanocube = (nv_Nanocube*) al_Allocator_get_root(previous_part_allocator);

					allocator = service_create_prepare_allocator_and_nanocube(request, &spec, data_memory_begin, data_memory_end, previous_part_nanocube);
					nanocube  = (nv_Nanocube*) al_Allocator_get_root(allocator);


					platform.close_mmap_file(&mapped_file);

				}
				/* end of doubling arena size or saving file and starting a new part */
#endif
			} /* treatment of high arena occupancy */

finalize_insertion:
			if (((offset+1) % report_frequency) == 0) {
				Print_clear(print);
				Print_format(print, "%10I64u # | %10I64u s | %10.1f MB | %5.1f mem%% | %12I64u ins | %5.1f ins%%\n",
					     (u64) offset + 1,
					     (u64) (platform.get_time() - t0),
					     (f32) ((f32)al_Allocator_used_memory(allocator) / Megabytes(1)),
					     (f32) ((100.0f*al_Allocator_used_memory(allocator)) / al_Allocator_capacity(allocator)),
					     (u64) records_inserted,
					     (f32) (100.0f*(f32)records_inserted/(f32)(offset + 1)));
				Request_print(request, print);
				Print_clear(print);
				last_printed = offset;

				/* @TODO(llins): add an option mechanism */
				/* print the memory usage of each cache */
				/* @NOTE(llins): if cube doesn't fit in memory, scanning all the records is very slow */
				if (report_cache) {
					nu_log_memory(allocator, &nanocube->index, print, detail, 0);
					Request_print(request, print);
				}
			}

		} /* input records loop */

		/* print final numbers if needed */
		if (offset != last_printed) {
			Print_clear(print);
			Print_format(print, "%10I64u # | %10I64u s | %10.1f MB | %5.1f mem%% | %12I64u ins | %5.1f ins%%\n",
				     (u64) offset + 1,
				     (u64) (platform.get_time() - t0),
				     (f32) ((f32)al_Allocator_used_memory(allocator) / Megabytes(1)),
				     (f32) ((100.0f*al_Allocator_used_memory(allocator)) / al_Allocator_capacity(allocator)),
				     (u64) records_inserted,
				     (f32) (100.0f*(f32)records_inserted/(f32)(offset+1)));
			Request_print(request,print);
			Print_clear(print);
		}


#ifdef PROFILE
		pf_end();
#endif


		if (!use_stdin) {
			platform.close_file(&csv_file);
		}
		platform.free_memory(&csv_buffer);

		// write to file

		f64 t0s = platform.get_time();

		// pt_File pfh_db = platform.open_write_file (output_filename.begin, output_filename.end);
		// platform.write_to_file (&pfh_db, (char*) allocator, (char*) allocator + al_Allocator_used_memory(allocator));
		// platform.close_file(&pfh_db);


#ifdef CSV_USING_MMAPED_SHARED_FILE
		/* fit memory to used pages */
		al_Allocator_fit(allocator);
		u64 used_size = al_Allocator_used_memory(allocator);
		/* TODO(llins): shrink this file. */
		platform.close_mmap_file(&mmap_output);
		Print_clear(print);
		Print_cstr(print, "[csv] resizing file...");
		if (platform.resize_file(output_filename.begin, output_filename.end, used_size)) {
			Print_cstr(print, "OK\n");
		} else {
			Print_cstr(print, "FAILED\n");
		}
		Request_print(request,print);
#else
		if (!service_create_save_arena(request, allocator, output_filename.begin, output_filename.end, part_number > 1 ? part_number : 0)) {
			Print_clear(print);
			Print_cstr(print,"[csv] couldn't save arena\n");
			Request_print(request, print);
			return;
		}

#ifndef NANOCUBE_SERVE_WHILE_CREATE

		platform.free_memory(&data_memory);

#else
		// there might be a server running.
		// signal the serve that a new finalized index is ready
		// to be served and wait for the server to finish before
		// exiting this program
		if (serve_port == 0) {
			platform.free_memory(&data_memory);
		} else {
			Assert(serve_work_queue);
			serve_config.refresh_data = allocator;
			pt_memory_barrier();
			pt_atomic_exchange_u32(&serve_config.refresh_status, service_create_serve_refresh_LAST_REFRESH);

			// @note just waiting for queue to end all its current tasks
			platform.work_queue_complete_work(serve_work_queue);

			// when queue is completed: no more listening to events,
			// destroy the serve listen work queue
			platform.work_queue_destroy(serve_work_queue);

			// free memory on exit
			platform.free_memory(&data_memory);
		}
#endif

#endif
		break;
	}

	//
	// the sequence of index and measure dimension
	// will be used to apply the right dispatch the
	// right conversion procedure (text to paths)
	// as well as specify the nanocube vector schema.
	//

	platform.free_memory(&mapping_text);
	platform.free_memory(&csv_mapping_parse_and_compile_buffer);

}

internal void
service_bits(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	u64 offset = 0, bits = 0, repeat = 1;
	if (!op_Options_u64(options, 1, &offset)) {
		Request_msg(request, "[bits] missing offset (usage: bits <offset> <bits> <repeat>)\n");
		return;
	}
	if (!op_Options_u64(options, 2, &bits)) {
		Request_msg(request, "[bits] missing offset (usage: bits <offset> <bits> <repeat>)\n");
		return;
	}
	if (!op_Options_u64(options, 3, &repeat)) {
		Request_msg(request, "[bits] missing offset (usage: bits <offset> <bits> <repeat>)\n");
		return;
	}


	u64 value  = 0xFEDCBA9876543210ull;
	s32 freq = 1000000;

	u64 count  = 0;
	u64 sum    = 0;
	u64 MinCyclesInAHit = 1000000;
	u64 MaxCyclesInAHit = 0;
	u64 sum_output = 0;
	for (u64 i=0;i<10000000;++i) {
		u64 output = 0;
		u64 cy = 0;
		for (u64 j=0;j<repeat;++j) {
			u64 cy_local = platform.cycle_count_cpu();
			pt_read_bits2((char*) &value, (u32) (offset + j*bits), (u32) bits, (char*) &output);
			cy_local = platform.cycle_count_cpu() - cy_local;
			sum_output += output;
			cy += cy_local;
		}
		++count;
		++value;
		sum += cy;

		MinCyclesInAHit = MIN(cy, MinCyclesInAHit);
		MaxCyclesInAHit = MAX(cy, MaxCyclesInAHit);

		if ((count % freq) == 0) {
			Print_clear(print);

			Print_cstr(print,"hits ");
			Print_u64(print, count);
			Print_align(print,  14, 1, ' ');

			Print_cstr(print,"    cycles ");
			Print_u64(print, sum);
			Print_align(print,  14, 1, ' ');

			Print_cstr(print,"    min,avg,max cycles/hit ");
			Print_u64(print, MinCyclesInAHit);
			Print_align(print,  14, 1, ' ');
			Print_u64(print, sum/count);
			Print_align(print,  14, 1, ' ');
			Print_u64(print, MaxCyclesInAHit);
			Print_align(print,  14, 1, ' ');

			Print_char(print,'\n');
			Request_print(request, print);
		}
	}
	Print_clear(print);
	Print_cstr(print,"sum outputs: ");
	Print_u64(print, sum_output);
	Print_cstr(print,"\n");
	Request_print(request, print);

}

//------------------------------------------------------------------------------
// service_client: test tcp client functionality
//------------------------------------------------------------------------------

// #define PLATFORM_TCP_CALLBACK(name) void name(pt_TCP_Socket *socket, char *buffer, u64 length)
internal
PLATFORM_TCP_CALLBACK(service_client_callback)
{
	Request *request = (Request*) socket->user_data;
	Print   *print   = request->print;
	Print_clear(print);
	Print_cstr(print, "[client receive data]:\n");
	Print_str(print, buffer, buffer+length);
	Request_print(request, print);
}

internal void
service_client(Request *request)
{
	// same program might be the client and the server
	// connection has a type (server or client)
	// send_message can write on the outgoing connection right away
	// incoming messages should be serialized
	// HTTP serializer
	// <server> <port>

	op_Options *options = &request->options;

	MemoryBlock hostname;
	u64 port_u64;
	if (!op_Options_str(options, 1, &hostname)) {
		Request_msg(request, "[service_client] Usage <hostname> <port>.\n");
		return;
	}
	*hostname.end = 0; // make sure it is null terminated (hack)
	if (!op_Options_u64(options, 2, &port_u64)) {
		Request_msg(request, "[service_client] Usage <hostname> <port>.\n");
		return;
	}
	s32 port = (s32) port_u64;

	pt_TCP tcp = platform.tcp_create();
	Assert(tcp.handle);

	pt_TCP_Feedback feedback;
	// #define PLATFORM_TCP_CLIENT(name) void name(pt_TCP *tcp, s32 port,
	// char *hostname, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
	platform.tcp_client(tcp, port, hostname.begin, (void*) request, service_client_callback, &feedback);

	// should now be available
	platform.tcp_process_events(tcp, 0);

	if (feedback.status != pt_TCP_SOCKET_OK) {
		Request_msg(request, "[service_client] Could not connect to server:port.\n");
		return;
	}

	char message[] = "GET /taxi.b(%22pickup_location%22,dive(8)); HTTP/1.1\r\nUser-Agent: nanocube client\r\n\r\n";
	platform.tcp_write(&feedback.socket, message, cstr_end(message) - message);

	while (!global_app_state->interrupted) {

		platform.tcp_process_events(tcp, 0);

	}

	platform.tcp_destroy(tcp);

# if 0
	pt_TCP *tcp_engine = platform.tcp_create();
	// create a thread and start
	platform.tcp_serve(tcp_engine, 8001, 0, request_handler);
	// create a bi-directional TCP channel to send data providing a callback
	pt_TCP_Connection *tcp_connection = platform.tcp_connect(tcp_engine, "server", 8001, 0, input_stream_handler);
	platform.tcp_write(output_stream, msg, msg_length);
	platform.tcp_write(output_stream, msg, msg_length);
	platform.tcp_write(output_stream, msg, msg_length);
	platform.tcp_close(tcp_engine, tcp_ostream);

	// using an http wrapper for the tcp channel
// 	http_Channel http_channel;
// 	pt_TCPOutputStream *output_stream = platform.tcp_connect(tcp_engine, "server", 8001, &http_channel, channel_receive);
// 	http_Response response1 = http_query(&http_channel, "taxi;");
// 	http_Response response2 = http_query(&http_channel, "taxi.b(\"pickup_location\",dive(8));");

	// wait on some reponse
	platform.tcp_close(tcp_engine, output_stream);

#endif

}

//------------------------------------------------------------------------------
// service_http: test http parsing functionality
//------------------------------------------------------------------------------

// #define PLATFORM_TCP_CALLBACK(name) void name(pt_TCP_Socket *socket, char *buffer, u64 length)
internal
PLATFORM_TCP_CALLBACK(service_http_tcp_server_callback)
{
	http_Channel *http_channel = (http_Channel*) socket->user_data;

	Print *print = debug_request->print;
	Print_clear(print);
	Print_cstr(print, "[service_tcp_server_callback]: dispatching raw tcp bytes to http message parser\n");
	Print_str(print, buffer, buffer+length);
	Request_print(debug_request, print);

	http_Channel_receive_data(http_channel, buffer, length);
	// Request_print(request, print);
}

// #define http_REQUEST_LINE_CALLBACK(name)
// 	void name(http_Response *response,
// 		  char *method_begin, char *method_end,
// 		  char *request_target_begin, char *request_target_end,
// 		  char *http_version_begin, char *http_version_end)
internal
http_REQUEST_LINE_CALLBACK(service_http_request_line_callback)
{
	/* print request_target */
	Print *print = debug_request->print;
	Print_clear(print);
	Print_cstr(print, "[service_http_request_line_callback]: request-target is ");
	/* rewrite on buffer of request_target */
	request_target_end = http_URI_decode_percent_encoded_symbols(request_target_begin, request_target_end, 0);
	if (request_target_end == 0) {
		Print_cstr(print, "couldn't translate target!");
	} else {
		Print_str(print, request_target_begin, request_target_end);
	}
	Print_cstr(print, "\n");
	Request_print(debug_request, print);
}

// #define http_HEADER_FIELD_CALLBACK(name)
// 	void name(http_Response *response,
// 		  char *field_name_begin, char *field_name_end,
// 		  char *field_value_begin, char *field_value_end)
internal
http_HEADER_FIELD_CALLBACK(service_http_header_field_callback)
{
	/* print request_target */
	Print *print = debug_request->print;
	Print_clear(print);
	Print_cstr(print, "[service_http_header_field_callback]: header field is key:'");
	Print_str(print, field_name_begin, field_name_end);
	Print_cstr(print, "', value:'");
	Print_str(print, field_value_begin, field_value_end);
	Print_cstr(print, "'\n");
	Request_print(debug_request, print);
}

internal void
service_http(Request *request)
{
	/* read port */
	op_Options *options = &request->options;
	u64 port_u64;
	if (!op_Options_u64(options, 1, &port_u64)) {
		Request_msg(request, "[service_http] Usage <port>.\n");
		return;
	}
	s32 port = (s32) port_u64;

	pt_Memory mem = platform.allocate_memory(Megabytes(32),3,0);

	/* prepare http parser */
	http_Channel http;
	http_Channel_init(&http, mem.memblock.begin, mem.memblock.end,
			  service_http_request_line_callback,
			  service_http_header_field_callback,
			  0);

	pt_TCP tcp = platform.tcp_create();
	Assert(tcp.handle);

	pt_TCP_Feedback feedback;

	// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port,
	// void *user_data, PlatformTCPCallback *callback, u32 *status)
	platform.tcp_serve(tcp, port, &http, service_http_tcp_server_callback, &feedback);

	// should now be available
	platform.tcp_process_events(tcp, 0);

	if (feedback.status != pt_TCP_SOCKET_OK) {
		Request_msg(request, "[service_http] Could not connect to server:port.\n");
		return;
	}

	while (!global_app_state->interrupted) {
		platform.tcp_process_events(tcp, 0);
	}
}





//
// service_create_test
//

csv_PULL_CALLBACK(service_create_test_pull_callback)
{
	pt_File *file = (pt_File*) user_data;
	platform.read_next_file_chunk(file, buffer, buffer + length);
	Assert(file->last_read <= length);

	Print *print = debug_request->print;
	Print_clear(print);
	Print_cstr(print, "[service_create_test_pull_callback] buffer length: ");
	Print_u64(print, length);
	Print_cstr(print, "\n");
	Request_print(debug_request, print);

	if (file->last_read > 0) {
		return buffer + file->last_read;
	} else if (file->eof) {
		/* indicates eof */
		return 0;
	} else {
		/* weird path */
		return buffer + file->last_read;
	}
}

internal void
service_create_test(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	// get next two tokens
	MemoryBlock input_filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &input_filename)) {
		Request_msg(request, "[csv-test] usage: nanocube csv-test <filename>.\n");
		return;
	}

	pt_Memory buffer = platform.allocate_memory(Megabytes(1),3,0);

// internal char*
// csv_Stream_init(csv_Stream *self, char sep, char *buffer, u64 length, u32 max_separators,
// 		csv_PullCallback *pull_callback, void *user_data)
// {

	pt_File file = platform.open_read_file(input_filename.begin, input_filename.end);
	if (!file.open) {
		Print_clear(print);
		Print_cstr(print, "[csv-test] couldn't open file: ");
		Print_str(print, input_filename.begin, input_filename.end);
		Print_cstr(print, "\n");
		Request_print(request, print);
		return;
	}

	csv_Stream csv_stream;
	csv_Stream_init(&csv_stream, ',',
			buffer.memblock.begin, buffer.memblock.end - buffer.memblock.begin,
			100, service_create_test_pull_callback, &file);

	MemoryBlock fields[30];

	u32 line = 0;
	while (csv_Stream_next(&csv_stream)) {
		++line;
		u32 csv_fields_count = csv_Stream_num_fields(&csv_stream);

		Print_clear(print);
		Print_cstr(print,"line ");
		Print_u64(print, line);
		Print_cstr(print," has ");
		Print_u64(print, (u64) csv_fields_count);
		Print_cstr(print," columns\n");
		Request_print(request, print);

		Assert(csv_fields_count < ArrayCount(fields));

		b8 ok = csv_Stream_get_fields(&csv_stream, fields, 0, csv_fields_count);
		if (ok) {
			for (u32 i=0;i<csv_fields_count;++i) {
				Print_clear(print);
				Print_cstr(print,"   [");
				Print_u64(print, i);
				Print_align(print, 2, 1, ' ');
				Print_cstr(print,"] -> ");
				Print_str(print,fields[i].begin,fields[i].end);
				Print_cstr(print,"\n");
				Request_print(request, print);
			}
		} else {
			Request_msg(request, "[csv-test]     could not read num fields");
		}

	}

	if (csv_stream.error != csv_OK) {
		Request_msg(request, "[csv-test] error on parsing csv file");
	}

}

//
// service_create_test
//

csv_PULL_CALLBACK(service_create_col_pull_callback)
{
	pt_File *file = (pt_File*) user_data;
	if (file->eof)
		return 0;
	platform.read_next_file_chunk(file, buffer, buffer + length);
	Assert(file->last_read <= length);

// 	Print *print = &debug_request->print;
// 	Print_clear(print);
// 	Print_cstr(print, "[service_create_col_pull_callback] buffer length: ");
// 	Print_u64(print, length);
// 	Print_cstr(print, "\n");
// 	Request_print(debug_request, print);

	if (file->last_read > 0) {
		return buffer + file->last_read;
	} else if (file->eof) {
		/* indicates eof */
		return 0;
	} else {
		/* weird path */
		return buffer + file->last_read;
	}
}

/*
BEGIN_DOC_STRING nanocube_csv_col_doc
Ver:   __VERSION__
Usage: nanocube csv-col CSVFILE COLUMN [OPTIONS]
Scan column COLUMN of the CSVFILE and include all distinct values
in a set. Report the set in the end.
Options are:
    -buffer-size=S
        Size of buffer. Default is 16M.
    -record-size=S
        Size of record. Default is 16M.

END_DOC_STRING
*/

internal void
service_create_col(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;

	if (op_Options_find_cstr(options,"-help") || op_Options_num_positioned_parameters(options) == 1) {
		Print_clear(print);
		Print_cstr(print, nanocube_csv_col_doc);
		Request_print(request, print);
		return;
	}

	// get next two tokens
	MemoryBlock input_filename = { .begin=0, .end=0 };
	if (!op_Options_str(options, 1, &input_filename)) {
		Request_msg(request, "[csv-test] usage: nanocube csv-col <filename> <column>"
			   " [-buffer-size=<memory>]"
			   " [-record-size=<memory>]"
			   );
		return;
	}

	s32 column = 0;
	if (!op_Options_s32(options, 2, &column)) {
		Request_msg(request, "[serve] could not parse port number.\n");
		return;
	}

	u64 buffer_size = Megabytes(16);
	if (op_Options_find_cstr(options,"-buffer-size")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-buffer-size",0,&st)) {
			Request_msg(request,  "invalid memory value on -buffer-size (default: 16M)");
			return;
		} else {
			buffer_size = ut_parse_storage_size(st.begin, st.end);
			if (buffer_size < Kilobytes(2)) {
				Request_msg(request,  "invalid memory value on -buffer-size (needs to be at least 2K)");
				return;
			}
		}
	}

	u64 record_size = Megabytes(16);
	if (op_Options_find_cstr(options,"-record-size")) {
		MemoryBlock st;
		if (!op_Options_named_str_cstr(options,"-record-size",0,&st)) {
			Request_msg(request,  "invalid memory value on -record-size (default: 16M)");
			return;
		} else {
			record_size = ut_parse_storage_size(st.begin, st.end);
			if (record_size < Kilobytes(2)) {
				Request_msg(request,  "invalid memory value on -record-size (needs to be at least 2K)");
				return;
			}
		}
	}

	pt_File file = platform.open_read_file(input_filename.begin, input_filename.end);
	if (!file.open) {
		Print_clear(print);
		Print_cstr(print, "[csv-col] couldn't open file: ");
		Print_str(print, input_filename.begin, input_filename.end);
		Print_cstr(print, "\n");
		Request_print(request, print);
		return;
	}

	pt_Memory set_buffer = platform.allocate_memory(buffer_size,3,0);
	char *set_buffer_begin = set_buffer.memblock.begin;
	char *set_buffer_end   = set_buffer.memblock.end;
	set_Set set;
	set_Set_init(&set, set_buffer.memblock.begin, set_buffer.memblock.end-set_buffer.memblock.begin);


	pt_Memory buffer = platform.allocate_memory(record_size,3,0);
	csv_Stream csv_stream;
	csv_Stream_init(&csv_stream, ',',
			buffer.memblock.begin, buffer.memblock.end - buffer.memblock.begin,
			100, service_create_col_pull_callback, &file);

	MemoryBlock fields[1];

	u32 offset = 0;
	while (csv_Stream_next(&csv_stream)) {
		++offset;

		if (offset % 100000 == 0) {
			Print_clear(print);
			Print_format(print, "records: %9d    unique values: %6d\n", offset, set.num_entries);
			Request_print(request, print);
// 			for (s32 i=0;i<set.num_entries;++i) {
// 				MemoryBlock st = set_Set_get(&set, i);
// 				Print_clear(print);
// 				Print_format(print, "%3d ", i+1);
// 				Print_str(print, st.begin, st.end);
// 				Print_char(print, '\n');
// 				Request_print(request, print);
// 			}
		}

		u32 csv_fields_count = csv_Stream_num_fields(&csv_stream);
		if (csv_fields_count <= column) {
			Print_clear(print);
			Print_cstr(print,"offset ");
			Print_u64(print, offset);
			Print_cstr(print," has not enough columns (");
			Print_u64(print, (u64) csv_fields_count);
			Print_cstr(print,")\n");
			Request_print(request, print);
			continue;
		}

		b8 ok = csv_Stream_get_fields(&csv_stream, fields, column, 1);
		if (ok) {
			s32 insert_status = 0;
			set_Set_insert(&set, fields[0].begin, fields[0].end, &insert_status);
		} else {
			Request_msg(request, "[csv-log]     could not read num fields");
		}
	}

	Print_clear(print);
	// Print_format(print, "[%9d] ---------------------- [final]\n", offset);
	for (s32 i=0;i<set.num_entries;++i) {
		MemoryBlock st = set_Set_get_key(&set, i);
		Print_clear(print);
		Print_format(print, "%3d ", i+1);
		Print_str(print, st.begin, st.end);
		Print_char(print, '\n');
		Request_print(request, print);
	}

	if (csv_stream.error != csv_OK) {
		Request_msg(request, "[csv-col] error on parsing csv file");
	}

}




#ifdef POLYCOVER

internal void
service_polycover(Request *request)
{
	PolycoverAPI *polycover = &global_app_state->polycover;

	Print      *print   = request->print;
	op_Options *options = &request->options;

	// lat,lon pairs
	f32 points[] = {
		-50.0f,-100.0f,
		-50.0f, 100.0f,
		 50.0f, 100.0f,
		 50.0f,-100.0f
	};
	polycover_Shape shape = polycover->new_shape(points, 4, 8);
	pt_Memory memory = platform.allocate_memory(Megabytes(4),12,0);
	s32 size = 0;
	s32 ok = polycover->get_code(shape,memory.memblock.begin,memory.memblock.end,&size);
	if (!ok) {
		Print_cstr(print, "not enough memory to get the code of the shape\n");
	} else {
		Print_cstr(print, memory.memblock.begin);
		Print_cstr(print, "\n");
	}
	Request_print(request, print);
	platform.free_memory(&memory);
}

#else

internal void
service_polycover(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;
	Print_cstr(print, "polycover is not available\n");
	Request_print(request, print);
}

#endif

internal void
service_api(Request *request)
{
	Print      *print   = request->print;
	op_Options *options = &request->options;
	Print_cstr(print, nanocube_api_doc);
	Request_print(request, print);
}

/*
 * ROADMAP RELATED FUNCTIONS
 * @TODO(llins)
 */
#include "app_roadmap.c"

#define cmd_is(name) \
	pt_compare_memory_cstr(command.begin, command.end, name) == 0


#ifdef PROFILE
pf_Table *global_profile_table = 0;
#endif

/*
BEGIN_DOC_STRING nanocube_doc
Ver:   __VERSION__
Usage: nanocube COMMAND [OPTIONS]
Front-end program to access Nanocube and Snap tools.

Nanocube-related COMMANDs:
    api........................print API documentation
    create.....................creates a nanocube index from .csv file
    draw.......................generate grapviz .dot given a nanocube
    query......................query nanocube indices from the command line
    memory.....................show memory usage information of a NC index
    serve......................start an http server to serve one or more
                               nanocube indices
    version....................report version of previously created NC index
Snap-related COMMANDs:
    snap.......................start an http server to serve a snap index
    snap-cli...................run a snap query through the command line
    snap-create................creates a snap index based an OSM-like .xml file
Other COMMANDs:
    ast........................check syntax of a text to see if the NC custom
                               syntax parser recognizes the text. This syntax is
			       used in multiple places: querying a nanocube,
			       querying a snap index, specifying how to map a
			       .csv into a nanocube index
    csv-col....................scan specific column of a .csv file and report
                               its distinct values
    qpart......................find a balanced partition of quadtree locates
                               in a .csv file.

END_DOC_STRING
*/

internal Print*
print_new(PlatformAPI *platform, u64 size)
{
	u64 adjusted_size = RALIGN(size,Kilobytes(4));
	u64 print_offset = RALIGN(sizeof(Print),8);
	void *buffer = platform->allocate_memory(adjusted_size,3,0).memblock.begin;

	Assert(buffer);
	Print *print = buffer;
	char *begin = (char*) buffer + print_offset;
	char *end   = (char*) buffer + adjusted_size;
	Print_init(print, begin, end);
	return print;
}


APPLICATION_PROCESS_REQUEST(application_process_request)
{
	/* copy platform function pointers */
	global_app_state = app_state;
	platform = app_state->platform;

	/* initialize nx_ module */
	nx_start();

// #ifdef POLYCOVER
// 	fprintf(stderr,"polycover->get_code: %p\n", global_app_state->polycover.get_code);
// #else
// 	fprintf(stderr,"NO POLYCOVER\n");
// #endif

#ifdef PROFILE
	global_profile_table = (pf_Table*) app_state->global_profile_table;
#endif

	Request request = {
		.app_state = app_state,
		.pfh_stdout = pfh_stdout,
		.pfh_stdin = pfh_stdin,
		.print = print_new(&platform, Kilobytes(64))
	};

	pt_Memory options_memory = platform.allocate_memory(Kilobytes(16),3,0);
	op_Options_init(&request.options, options_memory.memblock.begin, options_memory.memblock.end);
	op_Parser options_parser;
	op_Parser_init(&options_parser, &request.options);
	if (!op_Parser_run(&options_parser, request_begin, request_end)) {
		Request_msg(&request,"[fail] problem parsing command line: <command> <params> <options>\n");
		return;
	}

	MemoryBlock command = {.begin = 0, .end = 0 };
	if (!op_Options_str(&request.options, 0, &command)) {
		Request_msg(&request, nanocube_doc);
		return;
	}

// 	Print *print = &request.print;
// 	Print_clear(print);
// 	Print_cstr(print, "command: ");
// 	Print_str(print, command.begin, command.end);
// 	Print_cstr(print, "\n");
// 	Request_print(&request, print);

	debug_request = &request;

	// switch case based on first token
	if (cmd_is("demo_create")) {
		service_demo_create(&request);
	} else if (cmd_is("memory")) {
		service_memory(&request);
	} else if (cmd_is("api")) {
		service_api(&request);
	} else if (cmd_is("version")) {
		service_version(&request);
	} else if (cmd_is("query")) {
		service_query(&request);
	} else if (cmd_is("test")) {
		service_test(&request);
	} else if (cmd_is("ast")) {
		service_ast(&request);
	} else if (cmd_is("sizes")) {
		service_sizes(&request);
	} else if (cmd_is("serve")) {
		service_serve(&request);
	} else if (cmd_is("btree")) {
		service_btree(&request);
	} else if (cmd_is("create")) {
		service_create(&request);
	} else if (cmd_is("draw")) {
		service_draw(&request);
	} else if (cmd_is("time")) {
		service_time(&request);
	} else if (cmd_is("bits")) {
		service_bits(&request);
	} else if (cmd_is("client")) {
		service_client(&request);
	} else if (cmd_is("http")) {
		service_http(&request);
	} else if (cmd_is("csv-test")) {
		service_create_test(&request);
	} else if (cmd_is("csv-col")) {
		service_create_col(&request);
	} else if (cmd_is("qpart")) {
		service_qpart(&request);
	} else if (cmd_is("qpart2")) {
		service_qpart2(&request);
	} else if (cmd_is("qpcount")) {
		service_qpcount(&request);
	} else if (cmd_is("bkmmap")) {
		service_test_file_backed_mmap(&request);
	} else if (cmd_is("polycover")) {
		service_polycover(&request);
	} else if (cmd_is("scanner")) {
		ra_service_scanner(&request);
	} else if (cmd_is("parser")) {
		ra_service_parser(&request);
	} else if (cmd_is("snap")) {
		ra_service_snap(&request);
	} else if (cmd_is("snap-create")) {
		ra_service_create_snap(&request);
	} else if (cmd_is("snap-cli")) {
		ra_service_snap_cli(&request);
	} else if (cmd_is("btree")) {
		ra_service_btree(&request);
	} else if (cmd_is("print")) {
		ra_service_print(&request);
	} else if (cmd_is("latlon")) {
		ra_service_latlon(&request);
	} else if (cmd_is("closest")) {
		ra_service_closest(&request);
	} else if (cmd_is("ksmall")) {
		ra_service_ksmall(&request);
	} else {
		Request_msg(&request,"[fail] first parameter is not a valid command\n");
	}

	/* deinitialize nx_ module */
	nx_finish();
}


