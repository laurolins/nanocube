# Dimensions and Bins

Nanocubes is all about binning multi-dimenstional data. As a running
example, let's consider a table like the Chicago crime one:

    latitude | longitude | timestamp | type

How many records in my geo-location table fall into a certain spatial
bin? Or fall

# Paths

A *path* is the identifier of a *bin* in a *dimension* of a
nanocube. This *bin* can be either an aggregate one or a finest
resolution one. For example, if we have a dimension for a categorical
variable "device" represented as a two level tree where the root is

# Target

A *target* for a dimension restricts the set of records of interest to
the ones that are in a particular set of *bins* in that dimension. For
example, if we have a categorical dimension with US States as *bins*,
we can think of `{NY, LA, UT}` as a *target* for that dimension. More
formally, a *target* for dimension *d* specifies a set of *paths* that
should be visited every time the execution engine that is solving a
query needs to traverse a *binning hierarchy* on dimensions *d*. If a
certain *path* in the *target* is not present for a particular *bin
hierarchy* instance, then, obviously, this path won't be visited.


## Multi-Target

Sometimes we want to have multiple targets on a single dimension. For
example, we might want to query multiple sequential intervals from a
binary tree representation for time.

Here is an example

    http://localhost:29510/volume.a("location",dive([2,1,2],8))
    { "layers":[ "anchor:location" ], "root":{ "path":[], "children":[ { "path":[2,1,2,0,0,0,0,1,3,2,2], "val":5350 }, { "path":[2,1,2,0,0,0,0,1,3,0,3], "val":12191 }, { "path":[2,1,2,0,0,0,0,1,3,1,2], "val":215 }, { "path":[2,1,2,0,0,0,0,1,3,2,3], "val":7360 }, { "path":[2,1,2,0,0,0,0,1,2,3,3], "val":250 }, { "path":[2,1,2,0,0,0,0,1,3,2,1], "val":16614 }, { "path":[2,1,2,0,0,0,0,1,3,2,0], "val":6795 }, { "path":[2,1,2,0,0,0,0,1,3,0,2], "val":411 } ] } }

    http://localhost:29510/volume.r("time",mt_interval_sequence(480,24,10))
    { "layers":[ "multi-target:time" ], "root":{ "path":[], "children":[ { "path":[0], "val":625 }, { "path":[1], "val":723 }, { "path":[2], "val":663 }, { "path":[3], "val":518 }, { "path":[4], "val":411 }, { "path":[5], "val":588 }, { "path":[6], "val":717 }, { "path":[7], "val":715 }, { "path":[8], "val":703 }, { "path":[9], "val":618 } ] } }

    http://localhost:29510/volume.a("location",dive([2,1,2],8)).r("time",mt_interval_sequence(480,24,10))
    { "layers":[ "anchor:location", "multi-target:time" ], "root":{ "path":[], "children":[ { "path":[2,1,2,0,0,0,0,1,3,2,2], "children":[ { "path":[0], "val":82 }, { "path":[1], "val":77 }, { "path":[2], "val":65 }, { "path":[3], "val":53 }, { "path":[4], "val":62 }, { "path":[5], "val":72 }, { "path":[6], "val":88 }, { "path":[7], "val":79 }, { "path":[8], "val":76 }, { "path":[9], "val":58 } ] }, { "path":[2,1,2,0,0,0,0,1,3,0,3], "children":[ { "path":[0], "val":143 }, { "path":[1], "val":194 }, { "path":[2], "val":147 }, { "path":[3], "val":135 }, { "path":[4], "val":90 }, { "path":[5], "val":151 }, { "path":[6], "val":183 }, { "path":[7], "val":162 }, { "path":[8], "val":163 }, { "path":[9], "val":161 } ] }, { "path":[2,1,2,0,0,0,0,1,3,1,2], "children":[ { "path":[0], "val":1 }, { "path":[1], "val":1 }, { "path":[2], "val":2 }, { "path":[3], "val":1 }, { "path":[4], "val":5 }, { "path":[5], "val":2 }, { "path":[6], "val":3 }, { "path":[7], "val":5 }, { "path":[8], "val":3 }, { "path":[9], "val":1 } ] }, { "path":[2,1,2,0,0,0,0,1,3,2,3], "children":[ { "path":[0], "val":80 }, { "path":[1], "val":119 }, { "path":[2], "val":101 }, { "path":[3], "val":95 }, { "path":[4], "val":59 }, { "path":[5], "val":70 }, { "path":[6], "val":89 }, { "path":[7], "val":107 }, { "path":[8], "val":102 }, { "path":[9], "val":84 } ] }, { "path":[2,1,2,0,0,0,0,1,2,3,3], "children":[ { "path":[0], "val":2 }, { "path":[1], "val":6 }, { "path":[2], "val":6 }, { "path":[3], "val":5 }, { "path":[4], "val":3 }, { "path":[5], "val":5 }, { "path":[6], "val":4 }, { "path":[7], "val":5 }, { "path":[8], "val":6 }, { "path":[9], "val":1 } ] }, { "path":[2,1,2,0,0,0,0,1,3,2,1], "children":[ { "path":[0], "val":227 }, { "path":[1], "val":227 }, { "path":[2], "val":250 }, { "path":[3], "val":162 }, { "path":[4], "val":119 }, { "path":[5], "val":175 }, { "path":[6], "val":233 }, { "path":[7], "val":235 }, { "path":[8], "val":259 }, { "path":[9], "val":216 } ] }, { "path":[2,1,2,0,0,0,0,1,3,2,0], "children":[ { "path":[0], "val":85 }, { "path":[1], "val":93 }, { "path":[2], "val":87 }, { "path":[3], "val":61 }, { "path":[4], "val":72 }, { "path":[5], "val":106 }, { "path":[6], "val":107 }, { "path":[7], "val":117 }, { "path":[8], "val":86 }, { "path":[9], "val":93 } ] }, { "path":[2,1,2,0,0,0,0,1,3,0,2], "children":[ { "path":[0], "val":5 }, { "path":[1], "val":6 }, { "path":[2], "val":5 }, { "path":[3], "val":6 }, { "path":[4], "val":1 }, { "path":[5], "val":7 }, { "path":[6], "val":10 }, { "path":[7], "val":5 }, { "path":[8], "val":8 }, { "path":[9], "val":4 } ] } ] } }

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


wget http://pypi.python.org/packages/source/v/virtualenv/virtualenv-1.11.4.tar.gz
tar xfz virtualenv-1.11.4.tar.gz
python virtualenv-1.11.4/virtualenv.py  myPy
source myPy/bin/activate
pip install argparse numpy pandas


# ToDo

- Access of Multiple Variables






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

