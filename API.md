# Dimensions and Bins

Nanocubes is all about binning multi-dimenstional data. As a running
example, let's consider a table like the Chicago crime one:

    latitude | longitude | timestamp | type

How many records in my geo-location table fall into a certain spatial
bin? Or fall

# Address

Address can be interpreted as the "name" of a "bin" in a "dimension"
of a nanocube. This bin can be either an aggregate one or a finest
resolution one. For example, if we have a dimension for a categorical
variable "device" represented as a two level tree where the root is

# Target

Specifies a set of potential bins we want to visit in a certain
traversal of a nanocube for a cetain dimension. If we have a
geo-spatial dimension we might want to restrict our query only to bins
that are inside a certain regions (e.g. a zip-code area, or a state).

# Branching Target

Sometimes we want to have multiple targets on a single dimension. For
example, we might want to query multiple sequential intervals from a
binary tree representation for time.

## branch target

Here is an example

    http://localhost:12321/volume.a("src",dive([2],4)).r("time",bt_interval_sequence(19780,10,10))
    http://localhost:12321/volume.r("src",range2d(tile2d(2,2,2),tile2d(2,3,2))).r("time",bt_interval_sequence(19780,1,101))

    bt_intseq(0,24,10) # branch target

# Services

## `.schema`

Describes the schema of 

## `.volume`

Sum of values in a certain product bin.

Pairs of queries and results:

    http://localhost:29512/volume.json()
    { "levels":[  ], "root":{ "addr":[], "value":{"volume_count":1000000} } }

    http://localhost:29512/volume.r(0,[2])
    { "levels":[  ], "root":{ "addr":[], "value":{"volume_count":999985} } }

    http://localhost:29512/volume.r("space",[2])
    { "levels":[  ], "root":{ "addr":[], "value":{"volume_count":999985} } }

    http://localhost:29512/volume.a("space",[])
    { "levels":[ "L0" ], "root":{ "addr":[], "children":[ { "addr":[], "value":{"volume_count":1000000} } ] } }

    http://localhost:29512/volume.a("space",[2])
    { "levels":[ "L0" ], "root":{ "addr":[], "children":[ { "addr":[2], "value":{"volume_count":999985} } ] } }


### Output Encoding

There are three kinds of encodings: json (default), text, and
binary. To activate these methods (the last activated will be used)
three functions are avaiable on the queries: `.json()`, `.text()`,
`.bin()`.

## .range
## .topk
## .unique
## .id









<!-- sophia@oreilly.com -->

<!--     server.port = options.query_port.getValue(); -->
    
<!--     bool json        = true; -->
<!--     bool binary      = false; -->
<!--     bool compression = true; -->
<!--     bool plain       = false; -->
    
<!--     auto json_query_handler    = std::bind(&NanocubeServer::serveQuery, this, std::placeholders::_1, json,       plain); -->
<!--     auto binary_query_handler  = std::bind(&NanocubeServer::serveQuery, this, std::placeholders::_1, binary,     plain); -->
<!--     auto json_tquery_handler   = std::bind(&NanocubeServer::serveTimeQuery, this, std::placeholders::_1, json,   plain); -->
<!--     auto binary_tquery_handler = std::bind(&NanocubeServer::serveTimeQuery, this, std::placeholders::_1, binary, plain); -->
<!--     // auto json_query_comp_handler    = std::bind(&NanocubeServer::serveQuery, this, std::placeholders::_1, json,       compression); -->
<!--     // auto json_tquery_comp_handler   = std::bind(&NanocubeServer::serveTimeQuery, this, std::placeholders::_1, json,   compression); -->
<!--     auto binary_query_comp_handler  = std::bind(&NanocubeServer::serveQuery, this, std::placeholders::_1, binary,     compression); -->
<!--     auto binary_tquery_comp_handler = std::bind(&NanocubeServer::serveTimeQuery, this, std::placeholders::_1, binary, compression); -->
<!--     auto stats_handler         = std::bind(&NanocubeServer::serveStats, this, std::placeholders::_1); -->
<!--     auto binary_schema_handler = std::bind(&NanocubeServer::serveSchema,     this, std::placeholders::_1, binary); -->
<!--     auto schema_handler        = std::bind(&NanocubeServer::serveSchema,     this, std::placeholders::_1, json); -->
<!--     auto valname_handler       = std::bind(&NanocubeServer::serveSetValname, this, std::placeholders::_1); -->
<!--     auto version_handler       = std::bind(&NanocubeServer::serveVersion,    this, std::placeholders::_1); -->
<!--     auto tbin_handler          = std::bind(&NanocubeServer::serveTBin, this, std::placeholders::_1); -->
<!--     auto summary_handler       = std::bind(&NanocubeServer::serveSummary, this, std::placeholders::_1); -->
<!--     auto graphviz_handler      = std::bind(&NanocubeServer::serveGraphViz, this, std::placeholders::_1); -->
<!--     auto timing_handler        = std::bind(&NanocubeServer::serveTiming, this, std::placeholders::_1); -->
<!--     auto tile_handler         = std::bind(&NanocubeServer::serveTile, this, std::placeholders::_1); -->
    









<!-- # API 1 -->

<!-- http://nanocubes.net/nanocube/14/tile/4/8/7/10/0/10000000000/ -->
<!-- http://nanocubes.net/nanocube/14/query/region/0/0/0/1/1/where/hour_of_day=05 -->

<!-- # API 2 -->

<!-- http://lion5.research.att.com:29527/query/time=16224:8392:1/src=<qaddr(999,829,10),qaddr(0,829,10),qaddr(0,460,10),qaddr(999,460,10)>/@device=255+1 -->

<!-- # API 3 -->

