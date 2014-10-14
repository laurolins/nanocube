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
example, we might want to query multiple consecutive intervals from a
binary tree representation for time. Each interval data can be
"solved" by visiting a (minimal) set of time bins that is a cover
for it (the interval).

# Services

## `.schema`

Describes the Nanocube schema.

    # Schema
    http://localhost:29510/schema
    { "fields":[ { "name":"location", "type":"nc_dim_quadtree_25", "valnames":{  } }, { "name":"crime", "type":"nc_dim_cat_1", "valnames":{ "CRIM_SEXUAL_ASSAULT":7, "WEAPONS_VIOLATION":30, "KIDNAPPING":13, "OFFENSE_INVOLVING_CHILDREN":19, "CONCEALED_CARRY_LICENSE_VIOLATION":4, "SEX_OFFENSE":27, "INTIMIDATION":12, "PROSTITUTION":23, "ARSON":0, "BURGLARY":3, "ROBBERY":26, "OTHER_OFFENSE_":22, "CRIMINAL_TRESPASS":6, "THEFT":29, "HOMICIDE":10, "OBSCENITY":18, "OTHER_NARCOTIC_VIOLATION":20, "MOTOR_VEHICLE_THEFT":15, "GAMBLING":9, "OTHER_OFFENSE":21, "DECEPTIVE_PRACTICE":8, "NARCOTICS":16, "STALKING":28, "CRIMINAL_DAMAGE":5, "NON-CRIMINAL_(SUBJECT_SPECIFIED)":17, "PUBLIC_PEACE_VIOLATION":25, "BATTERY":2, "ASSAULT":1, "PUBLIC_INDECENCY":24, "LIQUOR_LAW_VIOLATION":14, "INTERFERENCE_WITH_PUBLIC_OFFICER":11 } }, { "name":"time", "type":"nc_dim_time_2", "valnames":{  } }, { "name":"count", "type":"nc_var_uint_4", "valnames":{  } } ], "metadata":[ { "key":"tbin", "value":"2013-12-01_00:00:00_3600s" }, { "key":"location__origin", "value":"degrees_mercator_quadtree25" }, { "key":"name", "value":"crime50k.csv" } ] }

## `.count`

Sum of values in a certain product bin.

Pairs of queries and results:

    ## No constraints example
    http://localhost:29510/count
    { "layers":[  ], "root":{ "val":49186 } }

    ## split on space (quadtree path 2,1,2) on a 256x256 image
    http://localhost:29510/count.a("location",dive([2,1,2],8))
    { "layers":[ "anchor:location" ], "root":{ "children":[ { "path":[2,1,2,0,0,0,0,1,3,2,2], "val":5350 }, { "path":[2,1,2,0,0,0,0,1,3,0,3], "val":12191 }, { "path":[2,1,2,0,0,0,0,1,3,1,2], "val":215 }, { "path":[2,1,2,0,0,0,0,1,3,2,3], "val":7360 }, { "path":[2,1,2,0,0,0,0,1,2,3,3], "val":250 }, { "path":[2,1,2,0,0,0,0,1,3,2,1], "val":16614 }, { "path":[2,1,2,0,0,0,0,1,3,2,0], "val":6795 }, { "path":[2,1,2,0,0,0,0,1,3,0,2], "val":411 } ] } }

    ## restrict to a rectangular area (see figure below)
    http://localhost:29510/count.r("location",range2d(tile2d(1049,2571,12),tile2d(1050,2572,12)))
    { "layers":[  ], "root":{ "val":11044 } }

    ## split on time base time bin = 480, bucket has 24 bins, get 10 buckets (if they exist)
    http://localhost:29510/count.r("time",mt_interval_sequence(480,24,10))
    { "layers":[ "multi-target:time" ], "root":{ "children":[ { "path":[0], "val":625 }, { "path":[1], "val":723 }, { "path":[2], "val":663 }, { "path":[3], "val":518 }, { "path":[4], "val":411 }, { "path":[5], "val":588 }, { "path":[6], "val":717 }, { "path":[7], "val":715 }, { "path":[8], "val":703 }, { "path":[9], "val":618 } ] } }

    ## combine location and time queries above (time series for each pixel)
    http://localhost:29510/count.a("location",dive([2,1,2],8)).r("time",mt_interval_sequence(480,24,10))
    { "layers":[ "anchor:location", "multi-target:time" ], "root":{ "children":[ { "path":[2,1,2,0,0,0,0,1,3,2,2], "children":[ { "path":[0], "val":82 }, { "path":[1], "val":77 }, { "path":[2], "val":65 }, { "path":[3], "val":53 }, { "path":[4], "val":62 }, { "path":[5], "val":72 }, { "path":[6], "val":88 }, { "path":[7], "val":79 }, { "path":[8], "val":76 }, { "path":[9], "val":58 } ] }, { "path":[2,1,2,0,0,0,0,1,3,0,3], "children":[ { "path":[0], "val":143 }, { "path":[1], "val":194 }, { "path":[2], "val":147 }, { "path":[3], "val":135 }, { "path":[4], "val":90 }, { "path":[5], "val":151 }, { "path":[6], "val":183 }, { "path":[7], "val":162 }, { "path":[8], "val":163 }, { "path":[9], "val":161 } ] }, { "path":[2,1,2,0,0,0,0,1,3,1,2], "children":[ { "path":[0], "val":1 }, { "path":[1], "val":1 }, { "path":[2], "val":2 }, { "path":[3], "val":1 }, { "path":[4], "val":5 }, { "path":[5], "val":2 }, { "path":[6], "val":3 }, { "path":[7], "val":5 }, { "path":[8], "val":3 }, { "path":[9], "val":1 } ] }, { "path":[2,1,2,0,0,0,0,1,3,2,3], "children":[ { "path":[0], "val":80 }, { "path":[1], "val":119 }, { "path":[2], "val":101 }, { "path":[3], "val":95 }, { "path":[4], "val":59 }, { "path":[5], "val":70 }, { "path":[6], "val":89 }, { "path":[7], "val":107 }, { "path":[8], "val":102 }, { "path":[9], "val":84 } ] }, { "path":[2,1,2,0,0,0,0,1,2,3,3], "children":[ { "path":[0], "val":2 }, { "path":[1], "val":6 }, { "path":[2], "val":6 }, { "path":[3], "val":5 }, { "path":[4], "val":3 }, { "path":[5], "val":5 }, { "path":[6], "val":4 }, { "path":[7], "val":5 }, { "path":[8], "val":6 }, { "path":[9], "val":1 } ] }, { "path":[2,1,2,0,0,0,0,1,3,2,1], "children":[ { "path":[0], "val":227 }, { "path":[1], "val":227 }, { "path":[2], "val":250 }, { "path":[3], "val":162 }, { "path":[4], "val":119 }, { "path":[5], "val":175 }, { "path":[6], "val":233 }, { "path":[7], "val":235 }, { "path":[8], "val":259 }, { "path":[9], "val":216 } ] }, { "path":[2,1,2,0,0,0,0,1,3,2,0], "children":[ { "path":[0], "val":85 }, { "path":[1], "val":93 }, { "path":[2], "val":87 }, { "path":[3], "val":61 }, { "path":[4], "val":72 }, { "path":[5], "val":106 }, { "path":[6], "val":107 }, { "path":[7], "val":117 }, { "path":[8], "val":86 }, { "path":[9], "val":93 } ] }, { "path":[2,1,2,0,0,0,0,1,3,0,2], "children":[ { "path":[0], "val":5 }, { "path":[1], "val":6 }, { "path":[2], "val":5 }, { "path":[3], "val":6 }, { "path":[4], "val":1 }, { "path":[5], "val":7 }, { "path":[6], "val":10 }, { "path":[7], "val":5 }, { "path":[8], "val":8 }, { "path":[9], "val":4 } ] } ] } }

    ## tile2d example tile2d(x,y,level)... x,y in {0,...,2^level-1}x{0,...,2^level-1}
    http://localhost:29510/count.a("location",dive(tile2d(1,2,2),8))
    { "layers":[ "anchor:location" ], "root":{ "children":[ { "path":[2,1,2,0,0,0,0,1,3,2], "val":36119 }, { "path":[2,1,2,0,0,0,0,1,3,1], "val":215 }, { "path":[2,1,2,0,0,0,0,1,2,3], "val":250 }, { "path":[2,1,2,0,0,0,0,1,3,0], "val":12602 } ] } }

    ## same as above, but passing the "img" formatting hint for bidimensional image addresses (relative address to tile(1,2,2))
    http://localhost:29510/count.a("location",dive(tile2d(1,2,2),8),"img")
    { "layers":[ "anchor:location" ], "root":{ "children":[ { "x":6, "y":131, "val":36119 }, { "x":7, "y":130, "val":215 }, { "x":5, "y":131, "val":250 }, { "x":6, "y":130, "val":12602 } ] } }

    ## branch on the "crime" type
    http://localhost:29510/count.a("crime",dive([],1))
    { "layers":[ "anchor:crime" ], "root":{ "children":[ { "path":[0], "val":66 }, { "path":[1], "val":2718 }, { "path":[2], "val":8946 }, { "path":[3], "val":2675 }, { "path":[4], "val":1 }, { "path":[5], "val":4621 }, { "path":[6], "val":1486 }, { "path":[7], "val":181 }, { "path":[8], "val":2281 }, { "path":[9], "val":3 }, { "path":[10], "val":63 }, { "path":[11], "val":225 }, { "path":[12], "val":21 }, { "path":[13], "val":52 }, { "path":[14], "val":69 }, { "path":[15], "val":2080 }, { "path":[16], "val":5940 }, { "path":[17], "val":1 }, { "path":[18], "val":4 }, { "path":[19], "val":453 }, { "path":[20], "val":2 }, { "path":[21], "val":3295 }, { "path":[22], "val":1 }, { "path":[23], "val":216 }, { "path":[24], "val":2 }, { "path":[25], "val":446 }, { "path":[26], "val":1896 }, { "path":[27], "val":125 }, { "path":[28], "val":22 }, { "path":[29], "val":10837 }, { "path":[30], "val":458 } ] } }

    ## set target 
    http://localhost:29510/count.r("crime",set([1],[3]))
    { "layers":[  ], "root":{ "val":5393 } }

    ## ... or a shortcut for addresses in the first level of a binning structure
    http://localhost:29510/count.r("crime",set(1,3))
    { "layers":[  ], "root":{ "val":5393 } }

    ## image restricted to a time interval [a,b]
    http://localhost:29510/count.r("time",interval(484,500)).a("location",dive(tile2d(262,643,10),1),"img")
    { "layers":[ "anchor:location" ], "root":{ "children":[ { "x":1, "y":1, "val":66 }, { "x":0, "y":1, "val":65 }, { "x":1, "y":0, "val":188 }, { "x":0, "y":0, "val":73 } ] } }

    ## a time series of images (time series for each pixel)
    http://localhost:29510/count.r("time",mt_interval_sequence(484,1,5)).a("location",dive(tile2d(262,643,10),1),"img")
    { "layers":[ "anchor:location", "multi-target:time" ], "root":{ "children":[ { "x":1, "y":1, "children":[ { "path":[0], "val":4 }, { "path":[1], "val":2 }, { "path":[2], "val":1 }, { "path":[4], "val":1 } ] }, { "x":0, "y":1, "children":[ { "path":[0], "val":1 }, { "path":[1], "val":3 }, { "path":[2], "val":1 }, { "path":[3], "val":3 }, { "path":[4], "val":1 } ] }, { "x":1, "y":0, "children":[ { "path":[0], "val":1 }, { "path":[1], "val":3 }, { "path":[2], "val":6 }, { "path":[3], "val":4 }, { "path":[4], "val":11 } ] }, { "x":0, "y":0, "children":[ { "path":[0], "val":2 }, { "path":[1], "val":1 }, { "path":[3], "val":3 }, { "path":[4], "val":2 } ] } ] } }

    ## degrees_mask (longitude,latitude) single contour
    http://localhost:29510/count.r("location",degrees_mask("-87.6512,41.8637,-87.6512,41.9009,-87.6026,41.9009,-87.6026,41.8637",25))
    { "layers":[  ], "root":{ "val":3259 } }

    ## unsorted
    http://localhost:29510/count.a("location",mask("012<<12<<<",10))
    http://localhost:29510/count.a("location",region("us_states/newyork",10))   // search directory
    http://localhost:29510/count.a("location",mercator_mask("x0,y0,x1,y1,...,xn,yn;x0,y0,x1,y1,x2,y2",10))
    http://localhost:29510/count.a("location",degrees_mask("x0,y0,x1,y1,...,xn,yn;x0,y0,x1,y1,x2,y2",10))

### Range example illustration:

![image](https://github.com/laurolins/nanocube/blob/api3/img/range-example.png?raw=true)

## `.timing`

Timing

### Output Encoding

There are three kinds of encodings: json (default), text, and
binary. To activate these methods (the last activated will be used)
three functions are avaiable on the queries: `.json()`, `.text()`,
`.bin()`.

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

