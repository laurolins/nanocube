```
src/build.bat
src/app.c
c:/PROGRA~0/R/R-3.3.1/include/R.h
c:/PROGRA~1/R/R-3.3.1/include/Rinternals.h
polycover/R/polycover/src/glue.cc
R/nanocube/src/nanocube_r.c
R/nanocube/R/nanocube.R
R/example4.R
R/nanocube/R/example4.R
README.md
src/nanocube_vector.c
src/nanocube_measure.c
```

# ToDo

## Hint to bring in time as a formatted data-time-stamp

Nivan requestes this one. It should be simple.

## Releasing the port after Ctrl+C takes one minute or so 2017-08-15T09:11:47

How to improve the release time of the port

## Command I am using to support build with POLYCOVER support

POLYCOVER_PATH=$HOME/local/bin bash build.linux release POLYCOVER

## short name to run all nanocube files in a filter in a server

```
X=$(ls -1 data/nanocube/*.nanocube | grep -v 'awsd' | tr '\n' ','); X=${X::-1}; CMD="nanocube serve 11111 x={$X} -threads=20"; bash -c "$CMD"
```

## Trying to make the 'name' work

In lldb we need comma and semi-colon
lldb -- nanocube query "{format('json')\;q(x.b('type'\,dive(p()\,1)\,'name'))\;}" "x=chicago-crimes-10k-sample.nanocube"
lldb -- nanocube csv chicago-crimes-10k-sample.csv chicago-crimes.map chicago-crimes-10k-sample.nanocube

## Annotate the binding of some dimensions for output formatting purposes

When querying a quadtree dimension we might want to translate
paths into their x,y coordinates (global or local? dive).

Datetime on binary trees might also be translated into date time
on the appropriate format

Categorical dimensions might be translated into their names

b('location',dive(p(),8),'img:8') # eight levels
b('location',dive(p(),8),'img:25')
b('location',dive(p(),8),'tile2d')
b('location',dive(p(),8),'latlon')

b('crime',dive(p(),8),'name')
b('time',dive(p(),8),'time')

The algebra transforms the query results into tables, the string
encoding the format hint must be preserved throughout the algebraic
manipulations and used at result rendering. Hint compatibility issues
should be treated.

format('text');q(taxi.b('pickup_location',dive(p(),8),'img'))
format('json');schema()

### JSON format

[
{ "type":"schema", },
{ "type":"table", "index_dimensions":[], "},
]

### TEXT format
### BINARY format





## Make the .csv mapping spec support a pre defined table of valid values.

Instead of dynamically defining IDs for categorical values, use an
input table to indicate mapping. Give user the ability to set new
labels for the incoming raw labels.

## Categorical query that output labels instead of numbers

How to bring in the categorical values?

## Fix the fact that the max outdegree of a node is 255. It should be 256.

0 is a leaf node, from there the numbers the number stored in
degree should actually represent degree-1.

## Check if file is trivially a non-nanocube one when one is expected

Check if the arena version text is there.

## Count nodes by degree in each hierarchy of a Nanocube

Leaves and others.

## Check nanocube_parser syntax error if it starts with '/' (binary operator)

It seems there is an assertion breaking instead of just
triggering a parsing error.

## HTTP client architecture to be used to test server and new clients

. We might need this before the HTTP parsing to make testing easier
. Also useful to stress test multiple connections to the server implementation

## HTTP parsing for long requests

. Try to recognize the boundaries of a valid HTTP request.
. Method GET without payload will be the only one to be processed.
.. Maybe very long requests will need payload (try without first)
. Mechanism should be aware of payload boundaries
. Mechanism should be aware of chunked requests
.. Without payload boundaries and chunked requests server gets our of sync
.. and doesn't recognize requests.

## Better printing mechanism (learn from stb)

## Profile in multiple compilation units [DONE]

Enable pf_BEGIN_BLOCK and pf_END_BLOCK in multiple compilation units.
See how Casey did it, my intuition is that we intialize the profile
memory on the heap instead of the stack.

## Serve multi-part nanocube [DONE]

```
nanocube serve <port-number> ([<alias>=]<nanocube-filename>,<nanocube-filename>)+
```
Make sure an alias can map into multiple nanocubes with the same schema.

## Clean cache in linux

```
echo "echo 1 > /proc/sys/vm/drop_caches" | sudo sh
```

## Streaming build of a Nanocube

## Multi-threaded Query Engine

## Grow index file as it reaches a certain threshold [DONE]

## http requests

Use mongoose http infra-structure. The current TCP req-respond is breaking
the request into two parts.

## Check virtual memory stats

while true ; do vmstat 2 1 | grep -v procs | column -t; sleep 1; done
while true ; do free; sleep 1; done
sudo perf top
pmap <pid>

## Tile-based partition

Using the test.tpart2

  92229632 p1
 137883648 p2
 198725632 p3
 209637376 p4
 302940160 p5
 458657792 p6

1504968704 pall

## Filter by the quadtree partition 2016-10-29T19:06

```
#
# result of qpart
# time nanocube qpart yellow_tripdata_2016-06.csv \",\" pickup_latitude pickup_longitude 100000 3
#
filter <cut-points> <lat-column> <lon-column>
```

## haversine distance example

nanocube snap ny.roadmap 3 40.686636 -73.982005 1

## Snapping filtering effect by radius

First 100k points in june recuces to

98,064 500m
97,710 250m
92,249 100m

nanocube snap ny.roadmap 3 40.686636 -73.982005 1

## qpart (finding out if Lukas effect happens here)

First 100k points

### no time, snap 300m
```
nanocube csv -filter=0,100000 -snap=roadmap/ny.roadmap,300 yellow_tripdata_2016-06.csv taxi_notime.map b
[csv] progress     100000  time          6  memory(MB)         35  memuse(%)          0%
[csv] records inserted 97913  97%
cache: INode               cklen:    36  ckuse:    13565  ckcap:    15923  mem:     0MB  usage:    85% pages:     140
cache: PNode               cklen:    36  ckuse:   484333  ckcap:   613707  mem:    21MB  usage:    78% pages:    5394
cache: nv_payload          cklen:    16  ckuse:   484333  ckcap:   613632  mem:     9MB  usage:    78% pages:    2397
INodes by degree...
INodes with degree 0 ->          8012
INodes with degree 2 ->          3775
INodes with degree 3 ->          1098
INodes with degree 4 ->           680
PNodes by degree...
PNodes with degree 0 ->        158152
PNodes with degree 2 ->        203513
PNodes with degree 3 ->         75408
PNodes with degree 4 ->         47260
Allocator total memory: 35MB
[csv] records inserted 97914  insert time          6  save time          0  memory(MB)         35  memuse(%)          0
```

### no time, no snap
```
nanocube csv -filter=0,100000 yellow_tripdata_2016-06.csv taxi_notime.map b
[csv] progress     100000  time          4  memory(MB)         59  memuse(%)          0%
[csv] records inserted 99999  100%
cache: INode               cklen:    36  ckuse:   158780  ckcap:   181808  mem:     6MB  usage:    87% pages:    1598
cache: PNode               cklen:    36  ckuse:   692944  ckcap:   920565  mem:    31MB  usage:    75% pages:    8091
cache: nv_payload          cklen:    16  ckuse:   692944  ckcap:   920576  mem:    14MB  usage:    75% pages:    3596
INodes by degree...
INodes with degree 0 ->         96399
INodes with degree 2 ->         38034
INodes with degree 3 ->         14677
INodes with degree 4 ->          9670
PNodes by degree...
PNodes with degree 0 ->         99260
PNodes with degree 2 ->        382132
PNodes with degree 3 ->        131600
PNodes with degree 4 ->         79952
Allocator total memory: 59MB
[csv] records inserted 100000  insert time          4  save time          0  memory(MB)         59  memuse(%)          0
```

### time, no snap

```
nanocube csv -filter=0,100000 yellow_tripdata_2016-06.csv taxi.map a
[csv] progress     100000  time         14  memory(MB)        142  memuse(%)          0%
[csv] records inserted 99999  100%
cache: INode               cklen:    36  ckuse:   851724  ckcap:   920565  mem:    31MB  usage:    92% pages:    8091
cache: PNode               cklen:    36  ckuse:  1699022  ckcap:  2071426  mem:    71MB  usage:    82% pages:   18206
cache: nv_payload          cklen:    16  ckuse:  1699022  ckcap:  2071296  mem:    31MB  usage:    82% pages:    8091
INodes by degree...
INodes with degree 0 ->        195659
INodes with degree 2 ->        420166
INodes with degree 3 ->        146277
INodes with degree 4 ->         89622
PNodes by degree...
PNodes with degree 0 ->        624357
PNodes with degree 2 ->       1074665
Allocator total memory: 142MB
[csv] records inserted 100000  insert time         14  save time          0  memory(MB)        142  memuse(%)          0
```

### time, 300m
```
nanocube csv -filter=0,100000 -snap=roadmap/ny.roadmap,300 yellow_tripdata_2016-06.csv taxi.map b
[csv] progress     100000  time         14  memory(MB)         94  memuse(%)          0%
[csv] records inserted 97913  97%
cache: INode               cklen:    36  ckuse:   497898  ckcap:   613707  mem:    21MB  usage:    81% pages:    5394
cache: PNode               cklen:    36  ckuse:  1352789  ckcap:  1380909  mem:    47MB  usage:    97% pages:   12137
cache: nv_payload          cklen:    16  ckuse:  1352789  ckcap:  1380864  mem:    21MB  usage:    97% pages:    5394
INodes by degree...
INodes with degree 0 ->        166164
INodes with degree 2 ->        207288
INodes with degree 3 ->         76506
INodes with degree 4 ->         47940
PNodes by degree...
PNodes with degree 0 ->        536953
PNodes with degree 2 ->        815836
Allocator total memory: 94MB
[csv] records inserted 97914  insert time         14  save time          0  memory(MB)         94  memuse(%)          0
```
### time, 300m, snap to roads with no service roads
```
nanocube csv -filter=0,100000 -snap=roadmap/ny.roadmap,300 yellow_tripdata_2016-06.csv taxi.map c
[csv] progress     100000  time         13  memory(MB)         94  memuse(%)          0%
[csv] records inserted 95839  95%
cache: INode               cklen:    36  ckuse:   479556  ckcap:   613707  mem:    21MB  usage:    78% pages:    5394
cache: PNode               cklen:    36  ckuse:  1302756  ckcap:  1380909  mem:    47MB  usage:    94% pages:   12137
cache: nv_payload          cklen:    16  ckuse:  1302756  ckcap:  1380864  mem:    21MB  usage:    94% pages:    5394
INodes by degree...
INodes with degree 0 ->        162067
INodes with degree 2 ->        199190
INodes with degree 3 ->         73010
INodes with degree 4 ->         45289
PNodes by degree...
PNodes with degree 0 ->        519419
PNodes with degree 2 ->        783337
Number of records: 95839
Allocator total memory: 94MB
[csv] records inserted 95840  insert time         13  save time          0  memory(MB)         94  memuse(%)          0
```

### time, snap road services and 100m
```
nanocube csv -filter=0,100000 -snap=roadmap/ny.roadmap,100 yellow_tripdata_2016-06.csv taxi.map x.nanocube
[csv] records inserted 92248  92%
cache: INode               cklen:    36  ckuse:   472320  ckcap:   613707  mem:    21MB  usage:    76% pages:    5394
cache: PNode               cklen:    36  ckuse:  1277604  ckcap:  1380909  mem:    47MB  usage:    92% pages:   12137
cache: nv_payload          cklen:    16  ckuse:  1277604  ckcap:  1380864  mem:    21MB  usage:    92% pages:    5394
INodes by degree...
INodes with degree 0 ->        156830
INodes with degree 2 ->        197351
INodes with degree 3 ->         72832
INodes with degree 4 ->         45307
PNodes by degree...
PNodes with degree 0 ->        506706
PNodes with degree 2 ->        770898
Allocator total memory: 94MB
[csv] records inserted 92249  insert time         11  save time          0  memory(MB)         94  memuse(%)          0
```

```
nanocube csv -qpart=y100k.qpart,8,pickup_latitude,pickup_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,100 yellow_tripdata_2016-06.csv taxi.map x4.nanocube
# [csv] progress     100000  time          2  memory(MB)         26  memuse(%)          0%
# [csv] records inserted 21520  21%
# cache: INode               cklen:    36  ckuse:   111382  ckcap:   121165  mem:     4MB  usage:    91% pages:    1065
# cache: PNode               cklen:    36  ckuse:   279565  ckcap:   409135  mem:    14MB  usage:    68% pages:    3596
# cache: nv_payload          cklen:    16  ckuse:   279565  ckcap:   409088  mem:     6MB  usage:    68% pages:    1598
# INodes by degree...
# INodes with degree 0 ->         35459
# INodes with degree 2 ->         46655
# INodes with degree 3 ->         18263
# INodes with degree 4 ->         11005
# PNodes by degree...
# PNodes with degree 0 ->        110774
# PNodes with degree 2 ->        168791
# Allocator total memory: 26MB
# [csv] records inserted 21520  insert time          2  save time          0  memory(MB)         26  memuse(%)          0
```

```
nanocube csv -qpart=y100k.qpart,4,pickup_latitude,pickup_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,100 yellow_tripdata_2016-06.csv taxi.map x3.nanocube
# [csv] progress     100000  time          2  memory(MB)         26  memuse(%)          0%
# [csv] records inserted 23719  23%
# cache: INode               cklen:    36  ckuse:   108547  ckcap:   121165  mem:     4MB  usage:    89% pages:    1065
# cache: PNode               cklen:    36  ckuse:   281406  ckcap:   409135  mem:    14MB  usage:    68% pages:    3596
# cache: nv_payload          cklen:    16  ckuse:   281406  ckcap:   409088  mem:     6MB  usage:    68% pages:    1598
# INodes by degree...
# INodes with degree 0 ->         36625
# INodes with degree 2 ->         44873
# INodes with degree 3 ->         16977
# INodes with degree 4 ->         10072
# PNodes by degree...
# PNodes with degree 0 ->        114117
# PNodes with degree 2 ->        167289
# Allocator total memory: 26MB
# [csv] records inserted 23719  insert time          2  save time          0  memory(MB)         26  memuse(%)          0
```

```
nanocube csv -qpart=y100k.qpart,2,pickup_latitude,pickup_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,100 yellow_tripdata_2016-06.csv taxi.map x2.nanocube
[csv] progress     100000  time          2  memory(MB)         26  memuse(%)          0%
[csv] records inserted 23523  23%
ache: INode               cklen:    36  ckuse:   110675  ckcap:   121165  mem:     4MB  usage:    91% pages:    1065
cache: PNode               cklen:    36  ckuse:   291927  ckcap:   409135  mem:    14MB  usage:    71% pages:    3596
cache: nv_payload          cklen:    16  ckuse:   291927  ckcap:   409088  mem:     6MB  usage:    71% pages:    1598
INodes by degree...
INodes with degree 0 ->         36958
INodes with degree 2 ->         46166
INodes with degree 3 ->         16835
INodes with degree 4 ->         10716
PNodes by degree...
PNodes with degree 0 ->        116039
PNodes with degree 2 ->        175888
Allocator total memory: 26MB
[csv] records inserted 23523  insert time          2  save time          0  memory(MB)         26  memuse(%)          0
```

```
nanocube csv -qpart=y100k.qpart,1,pickup_latitude,pickup_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,100 yellow_tripdata_2016-06.csv taxi.map x1.nanocube
[csv] progress     100000  time          2  memory(MB)         28  memuse(%)          0%
[csv] records inserted 23486  23%
cache: INode               cklen:    36  ckuse:   125496  ckcap:   181808  mem:     6MB  usage:    69% pages:    1598
cache: PNode               cklen:    36  ckuse:   324648  ckcap:   409135  mem:    14MB  usage:    79% pages:    3596
cache: nv_payload          cklen:    16  ckuse:   324648  ckcap:   409088  mem:     6MB  usage:    79% pages:    1598
INodes by degree...
INodes with degree 0 ->         39302
INodes with degree 2 ->         54703
INodes with degree 3 ->         19138
INodes with degree 4 ->         12353
PNodes by degree...
PNodes with degree 0 ->        125283
PNodes with degree 2 ->        199365
Allocator total memory: 28MB
[csv] records inserted 23487  insert time          2  save time          0  memory(MB)         28  memuse(%)          0```
```


p8 <- c(154537, 142023, 139712, 138934, 139195, 139898, 145417, 119768)


## Check effect of quadtree partitioning 2016-11-01T11:43

```
#
# [csv] progress   11100000  time        654  memory(MB)       4238  memuse(%)         14%
# [csv] records inserted 2388048  21%
#

# june.qpart
# 646232869450931
# 646232896932314
# 646232908592769

time nanocube csv yellow_tripdata_2016-06.csv taxi.map june_part0.nanocube -qpart=june.qpart,1,pickup_latitude,pickup_longitude -snap=roadmap/ny.roadmap,0.001

#
# took: 654.29s user 9.35s system 93% cpu 11:52.19 total
# INodes by degree...
# INodes with degree 0 ->       2699682
# INodes with degree 2 ->       2049797
# INodes with degree 3 ->        709092
# INodes with degree 4 ->        520314
# PNodes by degree...
# PNodes with degree 0 ->      13018799
# PNodes with degree 2 ->      59390733
# Allocator total memory: 4238MB
#
```

### four parts based on longitude


```
nanocube csv yellow_tripdata_2016-06.csv taxi.map x1 -qpart=dropoff_100k_4parts.qpart,1,dropoff_latitude,dropoff_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          3  memory(MB)         28  memuse(%)          0%
[csv] records inserted 24045  24%
cache: INode               cklen:    36  ckuse:   135686  ckcap:   181808  mem:     6MB  usage:    74% pages:    1598
cache: PNode               cklen:    36  ckuse:   345106  ckcap:   409135  mem:    14MB  usage:    84% pages:    3596
cache: nv_payload          cklen:    16  ckuse:   345106  ckcap:   409088  mem:     6MB  usage:    84% pages:    1598
INodes by degree...
INodes with degree 0 ->         43417
INodes with degree 2 ->         63584
INodes with degree 3 ->         17689
INodes with degree 4 ->         10996
PNodes by degree...
PNodes with degree 0 ->        132602
PNodes with degree 2 ->        212504
Number of records: 24045
Allocator total memory: 28MB
[csv] records inserted 24046  insert time          3  save time          0  memory(MB)         28  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi.map x2 -qpart=dropoff_100k_4parts.qpart,2,dropoff_latitude,dropoff_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          2  memory(MB)         26  memuse(%)          0%
[csv] records inserted 24602  24%
cache: INode               cklen:    36  ckuse:   117026  ckcap:   121165  mem:     4MB  usage:    96% pages:    1065
cache: PNode               cklen:    36  ckuse:   302066  ckcap:   409135  mem:    14MB  usage:    73% pages:    3596
cache: nv_payload          cklen:    16  ckuse:   302066  ckcap:   409088  mem:     6MB  usage:    73% pages:    1598
INodes by degree...
INodes with degree 0 ->         44020
INodes with degree 2 ->         46051
INodes with degree 3 ->         17263
INodes with degree 4 ->          9692
PNodes by degree...
PNodes with degree 0 ->        121303
PNodes with degree 2 ->        180763
Number of records: 24602
Allocator total memory: 26MB
[csv] records inserted 24602  insert time          2  save time          0  memory(MB)         26  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi.map x3 -qpart=dropoff_100k_4parts.qpart,4,dropoff_latitude,dropoff_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          2  memory(MB)         26  memuse(%)          0%
[csv] records inserted 24532  24%
cache: INode               cklen:    36  ckuse:   117070  ckcap:   121165  mem:     4MB  usage:    96% pages:    1065
cache: PNode               cklen:    36  ckuse:   296684  ckcap:   409135  mem:    14MB  usage:    72% pages:    3596
cache: nv_payload          cklen:    16  ckuse:   296684  ckcap:   409088  mem:     6MB  usage:    72% pages:    1598
INodes by degree...
INodes with degree 0 ->         44134
INodes with degree 2 ->         45872
INodes with degree 3 ->         16446
INodes with degree 4 ->         10618
PNodes by degree...
PNodes with degree 0 ->        120275
PNodes with degree 2 ->        176409
Number of records: 24532
Allocator total memory: 26MB
[csv] records inserted 24532  insert time          2  save time          0  memory(MB)         26  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi.map x4 -qpart=dropoff_100k_4parts.qpart,8,dropoff_latitude,dropoff_longitude -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          2  memory(MB)         28  memuse(%)          0%
[csv] records inserted 22660  22%
cache: INode               cklen:    36  ckuse:   121534  ckcap:   181808  mem:     6MB  usage:    66% pages:    1598
cache: PNode               cklen:    36  ckuse:   303909  ckcap:   409135  mem:    14MB  usage:    74% pages:    3596
cache: nv_payload          cklen:    16  ckuse:   303909  ckcap:   409088  mem:     6MB  usage:    74% pages:    1598
INodes by degree...
INodes with degree 0 ->         40293
INodes with degree 2 ->         52908
INodes with degree 3 ->         18981
INodes with degree 4 ->          9352
PNodes by degree...
PNodes with degree 0 ->        117862
PNodes with degree 2 ->        186047
Number of records: 22660
Allocator total memory: 28MB

[csv] records inserted 22660  insert time          2  save time          0  memory(MB)         28  memuse(%)          0
nanocube csv yellow_tripdata_2016-06.csv taxi.map x -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time         13  memory(MB)         94  memuse(%)          0%
[csv] records inserted 95839  95%
cache: INode               cklen:    36  ckuse:   479556  ckcap:   613707  mem:    21MB  usage:    78% pages:    5394
cache: PNode               cklen:    36  ckuse:  1302756  ckcap:  1380909  mem:    47MB  usage:    94% pages:   12137
cache: nv_payload          cklen:    16  ckuse:  1302756  ckcap:  1380864  mem:    21MB  usage:    94% pages:    5394
INodes by degree...
INodes with degree 0 ->        162067
INodes with degree 2 ->        199190
INodes with degree 3 ->         73010
INodes with degree 4 ->         45289
PNodes by degree...
PNodes with degree 0 ->        519419
PNodes with degree 2 ->        783337
Number of records: 95839
Allocator total memory: 94MB
[csv] records inserted 95840  insert time         13  save time          0  memory(MB)         94  memuse(%)          0
# let the query discover the unique sets: 303909 + 296684 + 302066 + 345106 = 1,247,765
# unique sets is: 1,302,756
```

Not very relevant the reduction in breaking the dropoff quadtree.



### Checking if qpart is doing the right thing

```
nanocube csv yellow_tripdata_2016-06.csv taxi_dropoff.map x -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          1  memory(MB)          2  memuse(%)          0%
[csv] records inserted 98116  98%
cache: INode               cklen:    36  ckuse:        0  ckcap:        0  mem:     0MB  usage:     0% pages:       0
cache: PNode               cklen:    36  ckuse:    24929  ckcap:    35833  mem:     1MB  usage:    69% pages:     315
cache: nv_payload          cklen:    16  ckuse:    24929  ckcap:    35840  mem:     0MB  usage:    69% pages:     140
INodes by degree...
PNodes by degree...
PNodes with degree 0 ->         14892
PNodes with degree 2 ->          6628
PNodes with degree 3 ->          1964
PNodes with degree 4 ->          1445
Number of records: 98116
Allocator total memory: 2MB
[csv] records inserted 98117  insert time          1  save time          0  memory(MB)          2  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi_dropoff.map x1 -filter=0,100000 -snap=roadmap/ny.roadmap,300 -qpart=dropoff_100k_4parts.qpart,1,dropoff_latitude,dropoff_longitude
[csv] progress     100000  time          0  memory(MB)          1  memuse(%)          0%
[csv] records inserted 24765  24%
cache: INode               cklen:    36  ckuse:        0  ckcap:        0  mem:     0MB  usage:     0% pages:       0
cache: PNode               cklen:    36  ckuse:     9327  ckcap:    10576  mem:     0MB  usage:    88% pages:      93
cache: nv_payload          cklen:    16  ckuse:     9327  ckcap:    10496  mem:     0MB  usage:    88% pages:      41
INodes by degree...
PNodes by degree...
PNodes with degree 0 ->          5591
PNodes with degree 2 ->          2464
PNodes with degree 3 ->           690
PNodes with degree 4 ->           582
Number of records: 24765
Allocator total memory: 1MB
[csv] records inserted 24766  insert time          0  save time          0  memory(MB)          1  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi_dropoff.map x2 -filter=0,100000 -snap=roadmap/ny.roadmap,300 -qpart=dropoff_100k_4parts.qpart,2,dropoff_latitude,dropoff_longitude
[csv] progress     100000  time          0  memory(MB)          1  memuse(%)          0%
[csv] records inserted 24995  24%
cache: INode               cklen:    36  ckuse:        0  ckcap:        0  mem:     0MB  usage:     0% pages:       0
cache: PNode               cklen:    36  ckuse:     3619  ckcap:     4660  mem:     0MB  usage:    77% pages:      41
cache: nv_payload          cklen:    16  ckuse:     3619  ckcap:     4608  mem:     0MB  usage:    78% pages:      18
INodes by degree...
PNodes by degree...
PNodes with degree 0 ->          2129
PNodes with degree 2 ->          1032
PNodes with degree 3 ->           278
PNodes with degree 4 ->           180
Number of records: 24995
Allocator total memory: 1MB
[csv] records inserted 24995  insert time          0  save time          0  memory(MB)          1  memuse(%)          0


nanocube csv yellow_tripdata_2016-06.csv taxi_dropoff.map x3 -filter=0,100000 -snap=roadmap/ny.roadmap,300 -qpart=dropoff_100k_4parts.qpart,4,dropoff_latitude,dropoff_longitude
[csv] progress     100000  time          0  memory(MB)          1  memuse(%)          0%
[csv] records inserted 24997  24%
cache: INode               cklen:    36  ckuse:        0  ckcap:        0  mem:     0MB  usage:     0% pages:       0
cache: PNode               cklen:    36  ckuse:     3280  ckcap:     4660  mem:     0MB  usage:    70% pages:      41
cache: nv_payload          cklen:    16  ckuse:     3280  ckcap:     4608  mem:     0MB  usage:    71% pages:      18
INodes by degree...
PNodes by degree...
PNodes with degree 0 ->          1928
PNodes with degree 2 ->           923
PNodes with degree 3 ->           283
PNodes with degree 4 ->           146
Number of records: 24997
Allocator total memory: 1MB
[csv] records inserted 24997  insert time          0  save time          0  memory(MB)          1  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi_dropoff.map x4 -filter=0,100000 -snap=roadmap/ny.roadmap,300 -qpart=dropoff_100k_4parts.qpart,8,dropoff_latitude,dropoff_longitude
[csv] progress     100000  time          0  memory(MB)          1  memuse(%)          0%
[csv] records inserted 23359  23%
cache: INode               cklen:    36  ckuse:        0  ckcap:        0  mem:     0MB  usage:     0% pages:       0
cache: PNode               cklen:    36  ckuse:     8996  ckcap:    10576  mem:     0MB  usage:    85% pages:      93
cache: nv_payload          cklen:    16  ckuse:     8996  ckcap:    10496  mem:     0MB  usage:    85% pages:      41
INodes by degree...
PNodes by degree...
PNodes with degree 0 ->          5401
PNodes with degree 2 ->          2327
PNodes with degree 3 ->           731
PNodes with degree 4 ->           537
Number of records: 23359
Allocator total memory: 1MB
[csv] records inserted 23359  insert time          0  save time          0  memory(MB)          1  memuse(%)          0

5401 + 1928 + 2129 + 5591

```




nanocube csv yellow_tripdata_2016-06.csv taxi.map taxi-100k-part1.nanocube -filter=0,100000 -qpart=snap_pickup_uniq.qpart,1,pickup_latitude,pickup_longitude -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          2  memory(MB)         13  memuse(%)          0%
[csv] records inserted 10972  10%
cache: INode               cklen:    36  ckuse:    59640  ckcap:    80774  mem:     2MB  usage:    73% pages:     710
cache: PNode               cklen:    36  ckuse:   144634  ckcap:   181808  mem:     6MB  usage:    79% pages:    1598
cache: nv_payload          cklen:    16  ckuse:   144634  ckcap:   181760  mem:     2MB  usage:    79% pages:     710
INodes by degree...
INodes with degree 0 ->         17833
INodes with degree 2 ->         26727
INodes with degree 3 ->          9205
INodes with degree 4 ->          5875
PNodes by degree...
PNodes with degree 0 ->         55693
PNodes with degree 2 ->         88941
Number of records: 10972
Allocator total memory: 13MB
[csv] records inserted 10973  insert time          2  save time          0  memory(MB)         13  memuse(%)          0


nanocube csv yellow_tripdata_2016-06.csv taxi.map taxi-100k-part2.nanocube -filter=0,100000 -qpart=snap_pickup_uniq.qpart,2,pickup_latitude,pickup_longitude -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          5  memory(MB)         38  memuse(%)          0%
[csv] records inserted 33468  33%
cache: INode               cklen:    36  ckuse:   158085  ckcap:   181808  mem:     6MB  usage:    86% pages:    1598
cache: PNode               cklen:    36  ckuse:   423261  ckcap:   613707  mem:    21MB  usage:    68% pages:    5394
cache: nv_payload          cklen:    16  ckuse:   423261  ckcap:   613632  mem:     9MB  usage:    68% pages:    2397
INodes by degree...
INodes with degree 0 ->         53061
INodes with degree 2 ->         66151
INodes with degree 3 ->         23808
INodes with degree 4 ->         15065
PNodes by degree...
PNodes with degree 0 ->        168463
PNodes with degree 2 ->        254798
Number of records: 33468
Allocator total memory: 38MB
[csv] records inserted 33468  insert time          5  save time          0  memory(MB)         38  memuse(%)          0


nanocube csv yellow_tripdata_2016-06.csv taxi.map taxi-100k-part3.nanocube -filter=0,100000 -qpart=snap_pickup_uniq.qpart,4,pickup_latitude,pickup_longitude -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          5  memory(MB)         38  memuse(%)          0%
[csv] records inserted 37220  37%
cache: INode               cklen:    36  ckuse:   168115  ckcap:   181808  mem:     6MB  usage:    92% pages:    1598
cache: PNode               cklen:    36  ckuse:   436674  ckcap:   613707  mem:    21MB  usage:    71% pages:    5394
cache: nv_payload          cklen:    16  ckuse:   436674  ckcap:   613632  mem:     9MB  usage:    71% pages:    2397
INodes by degree...
INodes with degree 0 ->         57695
INodes with degree 2 ->         69057
INodes with degree 3 ->         25942
INodes with degree 4 ->         15421
PNodes by degree...
PNodes with degree 0 ->        179354
PNodes with degree 2 ->        257320
Number of records: 37220
Allocator total memory: 38MB
[csv] records inserted 37220  insert time          5  save time          0  memory(MB)         38  memuse(%)          0

nanocube csv yellow_tripdata_2016-06.csv taxi.map taxi-100k-part4.nanocube -filter=0,100000 -qpart=snap_pickup_uniq.qpart,8,pickup_latitude,pickup_longitude -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time          2  memory(MB)         13  memuse(%)          0%
[csv] records inserted 14179  14%
cache: INode               cklen:    36  ckuse:    68542  ckcap:    80774  mem:     2MB  usage:    84% pages:     710
cache: PNode               cklen:    36  ckuse:   168004  ckcap:   181808  mem:     6MB  usage:    92% pages:    1598
cache: nv_payload          cklen:    16  ckuse:   168004  ckcap:   181760  mem:     2MB  usage:    92% pages:     710
INodes by degree...
INodes with degree 0 ->         22387
INodes with degree 2 ->         28583
INodes with degree 3 ->         11019
INodes with degree 4 ->          6553
PNodes by degree...
PNodes with degree 0 ->         66496
PNodes with degree 2 ->        101508
Number of records: 14179
Allocator total memory: 13MB
[csv] records inserted 14179  insert time          2  save time          0  memory(MB)         13  memuse(%)          0


nanocube csv yellow_tripdata_2016-06.csv taxi.map taxi-100k.nanocube -filter=0,100000 -snap=roadmap/ny.roadmap,300
[csv] progress     100000  time         13  memory(MB)         94  memuse(%)          0%
[csv] records inserted 95839  95%
cache: INode               cklen:    36  ckuse:   479556  ckcap:   613707  mem:    21MB  usage:    78% pages:    5394
cache: PNode               cklen:    36  ckuse:  1302756  ckcap:  1380909  mem:    47MB  usage:    94% pages:   12137
cache: nv_payload          cklen:    16  ckuse:  1302756  ckcap:  1380864  mem:    21MB  usage:    94% pages:    5394
INodes by degree...
INodes with degree 0 ->        162067
INodes with degree 2 ->        199190
INodes with degree 3 ->         73010
INodes with degree 4 ->         45289
PNodes by degree...
PNodes with degree 0 ->        519419
PNodes with degree 2 ->        783337
Number of records: 95839
Allocator total memory: 94MB
[csv] records inserted 95840  insert time         13  save time          0  memory(MB)         94  memuse(%)          0

144634 + 423261 + 436674 + 168004 = 1172573

1302756















## Failed saving large file

```c
// file was 16,415,375,360 bytes
PLATFORM_WRITE_TO_FILE(win32_write_to_file)
{
	Assert(pfh->write && pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	size_t size = (size_t) (end-begin);
	size_t written = fwrite(begin, 1, size, fp);

	/* @TODO(llins): replace with windows functions */
// 	DWORD BytesWritten;
// 	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);

	Assert(written == size);
}
```
## Alignment matters! `2016-10-21T11:54` [Working with R and C]

```c
&nv_Nanocube.index_dimensions.names[0]
offset is 135 when running in the R generated .dll
and 136 when in the app generated .dll
```

Although it is working for now, there should be a revision of all structs to
minimize chances compilers will mess with struct alignments (manual padding).

## Memory map files for serve command `2016-10-19T10:45`

Keep memory usage low (let the OS deal with the pages we hit).
Future (include option to load the whole file in memory?)

## Implement `< > <= >= !=` operations on relations `2016-10-18T15:59`

Would this solve the query `fare/count * count > 25`?

## Column names should combine original names `2016-10-14T14:12`

For example if we divide a column called `fare` with a column
called `count` we should obtains `fare/count`.

## Associate annotation to dimension column values `2016-10-14T10:58`

Instead of simple numbers (loop dimensions) or paths (drilldown dimensions) we
want the ability to respond tranformed labels to these values. For example, a
path on a quadtree for a *dive* target could get a global x,y coordinate or
even a lat,lon (centered at the given cell). A drilldown on a categorical
dimension could use the registered names for the paths.

In other words, when binding we could also have a format object:
```
.b(<dim-name>, <target>, <format>)
```

Question: where should the naming be incorporated? After the final analysis?
What if we evaluate two calendar based queries with incompatible
interpretations?  loop variables represent different calendar ranges?

## Import categorical dimension in `csv` command `2016-10-14T16:01`

Ideas:

1. Parse all the column of a csv file and check all different values (maybe with
some simple case-insensitive and trimming options), then, maybe use a sorted
list of these values to assign a number (everything will be alfabetically
sorted).
  - [pro ] well defined; simple; alphabetical order match the numerical order;
  - [cons] day of the week will have a weird ordering;
2. For a small set of values maybe it is easier to let a user define the list
of available values and their encoding text. If text fails to match diregard
record.

Maybe 1 and 2 should both be available.

* What about unicode?

* What about hierarchical values?

* Interesting example could be a nanocube of a hard disk? Size is interesting.
Should be an interesting sanity check case for the dictionary naming of files.

## Encode results as simple json objects (binary format web-compatible too?) `2016-10-17T16:11` `[Working Version]`

Http result objects are still mime-typed as text, but their content are an
actual valid json. If we use `txt=1` we get the previous text tabular
representation.

```js
{
"rows": 1000,
"index_columns": [
{"name": "location", "values_per_row": 1, "values": [ 1, 0, 1, 0, 2, 1]},
{"name": "location", "values_per_row": 1, "values": [ 1, 0, 1, 0, 2, 1]},
{"name": "location", "values_per_row": 1, "values": [ 1, 0, 1, 0, 2, 1]}
],
"value_columns": [
 {"name": "fare/count", values: [0.5, 0.3, 0.1, 0.8, 1.1 ] },
 {"name": "fare/count", values: [0.5, 0.3, 0.1, 0.8, 1.1 ] }
]
}
```

## Better printing service for floating point numbers `2016-10-14T16:00`

Casey Muratori has a video of a simple floating point number printing function.

## Revise memory management of measure evaluations

## Revise error flow on nm_Measure_solve_query (should report more precisely error eg. timeseq)

## Schema is being stored? (Value Names).

## Nanocube count unit test (needs a result format for the tables).

## Documentation.

## Revive the Parallel-Cross Filtered Viewer (applying the clean model).

## Store names of paths (btree?)

# Done

## Bug on dive resolution `2016-10-17T16:05` [DONE]

Dataset generated with
```
nanocube csv yellow_tripdata_2016-06.csv taxi.map small.nanocube 99 2
```
missing record counts with the simplest dive query
```
http://localhost:8000/taxi.b("pickup_location",dive(1)).select("count");
```
only happening on the json query


## Time Seq not merging `[DONE]` `2016-10-13T20:09`

Problem was when comparing *loop_columns* on `nm_TableKeys`.

```shell
nanocube serve 8000 taxi=taxi_june_1M.nanocube nypd=nypd_20161013.nanocube
wget 'http://localhost:8000/nypd.b("time",timeseq("2000T0-05:00",365*24*3600,10,365*24*3600));'
nanocube serve 8000 taxi=w:\compressed_nanocube\data\taxi_june_1M.nanocube nypd=w:\compressed_nanocube\data\nypd_20161013.nanocube
```
## Find a simple hash function for the btree module bt_ `[DONE]`

## Calendar-based time series query. `[DONE]`

## More general date parser `2016-10-13T16:20` `[DONE]`

AM and PM, and year as last number of date

## Include allocator of keyvalue pairs `[DONE]`

## Fix depth on interval sequence query. `[DONE]`

## Instantiate server on an HTTP port. `[DONE]`

## Single interval restriction. `[DONE]`

## Make nanocube count support multiple values `[DONE]`

## Make nanocube count language support multiple values `[DONE]`

## Incorporate module prefix convention on al_ `[DONE]`


# Taxi Example

```
#
# Create Compressed Nanocube Index (.cnc) from .csv file with initial 10 records on csv file
# taxi.map content is
#
# index_dimension("pickup_location",input("pickup_latitude","pickup_longitude"),latlon(25));
# index_dimension("dropoff_location",input("dropoff_latitude","dropoff_longitude"),latlon(25));
# index_dimension("pickup_time",input("tpep_pickup_datetime"),time(16,"2016-01-01T00:00:00-05:00",3600));
# measure_dimension("count",input(),u32);
# measure_dimension("tip",input("tip_amount"),f32);
# measure_dimension("fare",input("fare_amount"),f32);
# measure_dimension("distance",input("trip_distance"),f32);
#
cnc csv taxi.csv taxi.map taxi.cnc 0 10

# Query coarsest product-bin
cnc query taxi.cnc taxi;
#    #         count           tip          fare      distance
#    1  1.000000e+01  1.716000e+01  1.345000e+02  3.045000e+01

# Generate graphviz `.dot` file with Compressed Nanocube Index drawing
cnc draw taxi.cnc taxi.dot
# [draw:msg] To generate .pdf of graph run:
# dot -Tpdf -odrawing.pdf taxi.dot

```

# Time parsing

index_dimension("pickup_time",input("pickup_time"),time(16,"2016",HOUR));
<year> [ '/' <month> [ '/' <day> ] ] ['T' <hour> [ ':' <minute> [ ':' <second> ] ] ] [ 'Z' | ('+' | '-') [ <hour> ':' [ <minute> ] ] ]

Accept some formats:

2D-2D-2D 2D:2D:2D
4D-2D-2D'T'

2016-12-09T23:00

Infer the date format from example.
Have a spec language for a format.

No input. Have an auto way of detecting UTC datetime formats.
Most recently used.
Loop through the options.

# Query examples

```
taxirides.select("fare")/taxirides.select("count")
```

# Examples

```
ncc test
ncc create crimes.dmp crimes.ncc
ncc create crimes.dmp crimes-0-100k.ncc 0 100000
ncc create crimes.dmp crimes-50k-100k.ncc 50000 50000
ncc ast 'events.b("location",mask("020<<11<<<3<<")).b("time",dive(3));'
ncc ast 'events.b("location",mask("020<<11<<<3<<")).b("time",dive(p(0,1),3));'
ncc query crimes.ncc 'events.b("location",mask("020<<11<<<3<<")).b("time",dive(3));'
ncc query crimes.ncc 'events.b("location",mask("020<<11<<<3<<")).b("time",dive(p(0,1),3));'


csv w:\compressed_nanocube\data\taxi.csv w:\compressed_nanocube\data\taxi.map w:\compressed_nanocube\data\taxi.out 0 10

ncv csv c:\work\compressed_nanocube\data\nytaxi_sample_1k.csv c:\work\compressed_nanocube\data\nytaxi_1k.nc
index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));
measure_dimension(\"count\",input(),u64);

# on a mac
ncv csv /Users/llins/projects/compressed_nanocube/data/nytaxi_sample_1k.csv /Users/llins/projects/compressed_nanocube/data/taxi.nc "index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));measure_dimension(\"count\",input(),u32);"

ncv csv /Users/llins/projects/compressed_nanocube/data/nytaxi_sample_1k.csv /Users/llins/projects/compressed_nanocube/data/taxi.nc

#
# pickup -> count
#09/06/2016  11:51 PM         1,040,384 nytaxi_pickup_q25_count_u32.nc
#
ncv csv w:\compressed_nanocube\data\nytaxi_sample_1k.csv w:\compressed_nanocube\data\nytaxi_pickup_q25_count_u32.nc index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));measure_dimension(\"count\",input(),u32);

#
# pickup, dropoff -> count
# 09/06/2016  11:55 PM         1,114,112 nytaxi_pickup_q25_dropoff_q25_count_u32.nc
#
ncv ^
csv ^
w:\compressed_nanocube\data\nytaxi_sample_1k.csv ^
w:\compressed_nanocube\data\nytaxi_pickup_q25_dropoff_q25_count_u32.nc ^
index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));^
index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));^
measure_dimension(\"count\",input(),u32);

#
# pickup, dropoff -> count, fare
# 09/06/2016  11:55 PM         1,114,112 nytaxi_pickup_q25_dropoff_q25_count_u32_fare_f32.nc
#
ncv ^
csv ^
w:\compressed_nanocube\data\nytaxi_sample_1k.csv ^
w:\compressed_nanocube\data\nytaxi_pickup_q25_dropoff_q25_count_u32_fare_f32.nc ^
index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));^
index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));^
measure_dimension(\"count\",input(),u32);^
measure_dimension(\"fare\",input(\"fare_amount\"),f32);



ncv csv w:\compressed_nanocube\data\nytaxi_sample_1k.csv ^
w:\compressed_nanocube\data\taxi.nc ^
index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));^
index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));^
measure_dimension(\"count\",input(),u32);^
measure_dimension(\"tip\",input(\"tip_amount\"),f32);^
measure_dimension(\"fare\",input(\"fare_amount\"),f32);^
measure_dimension(\"distance\",input(\"trip_distance\"),f32);

# windows
ncv csv w:\compressed_nanocube\data\yellow_tripdata_2016-06.csv taxi.nc index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));measure_dimension(\"count\",input(),u32);measure_dimension(\"tip\",input(\"tip_amount\"),f32);measure_dimension(\"fare\",input(\"fare_amount\"),f32);measure_dimension(\"distance\",input(\"trip_distance\"),f32); 0 10000000

ncv csv w:\compressed_nanocube\data\nytaxi_sample_1k.csv w:\compressed_nanocube\data\taxi.nc index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));measure_dimension(\"count\",input(),u32);measure_dimension(\"tip\",input(\"tip_amount\"),f32);measure_dimension(\"fare\",input(\"fare_amount\"),f32);measure_dimension(\"distance\",input(\"trip_distance\"),f32); 0 10


# osx
ncv csv /Users/llins/projects/data/yellow_tripdata_2016-06.csv taxi.nc 'index_dimension("pickup_location",input("pickup_latitude","pickup_longitude"),latlon(25));index_dimension("dropoff_location",input("dropoff_latitude","dropoff_longitude"),latlon(25));measure_dimension("count",input(),u32);measure_dimension("tip",input("tip_amount"),f32);measure_dimension("fare",input("fare_amount"),f32);measure_dimension("distance",input("trip_distance"),f32);' 0 40000


ncv csv w:\compressed_nanocube\data\yellow_tripdata_2016-06.csv w:\compressed_nanocube\data\taxi2.nc index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));measure_dimension(\"count\",input(),u32);measure_dimension(\"tip\",input(\"tip_amount\"),f32);measure_dimension(\"fare\",input(\"fare_amount\"),f32);measure_dimension(\"distance\",input(\"trip_distance\"),f32); 0 1000000

# bug

# bugged queries
region=b("pickup_location",dive(11));fare=select("fare");count=select("count");taxi.region.fare/taxi.region.count;
serve 8000 w:\compressed_nanocube\data\taxi.nc

ncv csv w:\compressed_nanocube\data\yellow_tripdata_2016-06.csv w:\compressed_nanocube\data\taxi2.nc index_dimension(\"pickup_location\",input(\"pickup_latitude\",\"pickup_longitude\"),latlon(25));index_dimension(\"dropoff_location\",input(\"dropoff_latitude\",\"dropoff_longitude\"),latlon(25));measure_dimension(\"count\",input(),u32);measure_dimension(\"tip\",input(\"tip_amount\"),f32);measure_dimension(\"fare\",input(\"fare_amount\"),f32);measure_dimension(\"distance\",input(\"trip_distance\"),f32); 0 1000000

# linux
LD_LIBRARY_PATH=. ./ncv csv /home/llins/projects/compressed_nanocube/data/yellow_tripdata_2016-05.csv taxi2.nc 'index_dimension("pickup_location",input("pickup_latitude","pickup_longitude"),latlon(25));index_dimension("dropoff_location",input("dropoff_latitude","dropoff_longitude"),latlon(25));measure_dimension("count",input(),u32);measure_dimension("tip",input("tip_amount"),f32);measure_dimension("fare",input("fare_amount"),f32);measure_dimension("distance",input("trip_distance"),f32);' 0 6000000


sudo perf record --call-graph dwarf ./ncv csv /home/llins/projects/compressed_nanocube/data/yellow_tripdata_2016-05.csv taxi2.nc 'index_dimension("pickup_location",input("pickup_latitude","pickup_longitude"),latlon(25));index_dimension("dropoff_location",input("dropoff_latitude","dropoff_longitude"),latlon(25));measure_dimension("count",input(),u32);measure_dimension("tip",input("tip_amount"),f32);measure_dimension("fare",input("fare_amount"),f32);measure_dimension("distance",input("trip_distance"),f32);' 0 100000



```

# Chasing Bug X

```
nx_INSERT_EXACT				 2,086,841   1.675470
nx_INSERT_SPLIT				11,296,715   9.069838
nx_INSERT_BRANCH			17,482,489  14.036235
nx_INSERT_SHARED_SUFFIX_WAS_SPLIT	 1,250,905   1.004319
nx_INSERT_SHARED_SPLIT			27,695,762  22.236206
nx_INSERT_SHARED_NO_SPLIT		64,739,840  51.977931
```

# Linux Perf

```
  780  man perf
  781  perf timechart
  782  perf trace record ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 1000000
  783  perf record ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 1000000
  784  man perf reco
  785  perf record --help
  786  perf record -ag ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 1000000
  787  perf record -g ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 1000000
  788  perf report
  789  perf report -V
  790  perf report -G
  791  perf report -G -g
  792  perf report -g
  793  perf report --branch-history
  794  perf record -b -g ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 1000000
  795  perf record -b -g ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  796  perf report --branch-history
  797  perf report -b
  798  perf start ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  799  perf stat ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  800  perf top -a
  801  perf top
  802  perf record -bag ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  803  sudo perf record -bag ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  804  perf record -bg ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  805  perf report
  806  perf record --call-graph dwarf -- ncc create /home/llins/data/mts/mts_nc_bin.dmp mts-100k.ncc 0 100000
  807  perf report -g graph --no-children
 1158  history | grep perf
```

# Import .csv, .psv directly and avoid intermediate format

* Rules on mapping .csv into NC index and measure dimensions
should be passable from the command line.

** should be easy to write and parse

# based only on column names

<dimension>:<inputformat>,<col1>,...,<colN>:<bits>:<

with/without header (header can be passed externally)

# parse a program with the same parser we already have.
index_dimension("pickup_location",input("pickup_latitude","pickup_longitude"),latlon_mercator_quadtree(25));
index_dimension("dropoff_location",input("dropoff_latitude","dropoff_longitude"),latlon_mercator_quadtree(25));
index_dimension("pickup_time",input("tpep_pickup_datetime"),datetime_bintree(16,"2016-12-02T20:00","1h"));
index_dimension("dropoff_time",input("tpep_dropoff_datetime"),datetime_bintree(16,"2016-12-02T20:00","1h"));
index_dimension("payment_type",input("payment_type"),categorical(1,8));
measure_dimension("count",input(),"u32");
measure_dimension("fare_amount",input("fare_amount"),"f32");
measure_dimension("tip_amount",input("tip_amount"),"f32");


index_dimension("pickup_location",input("pickup_latitude","pickup_longitude"),latlon_mercator_quadtree(25));

measure_dimension(
dimension("pickup_location").

@23, @4 =>
pickup_latitude, pickup_longitude => LATLONGDEGREE_MERCATOR_QUADTREE_CELL(25) => pickup_location
tpep_pickup_datetime => DATETIME_BINTREE_CELL(16,2016-12-30T00:00,1h) => pickup_time


pickup:INPUT_FORMAT_LATLON,pickup_latitude,pickup_longitude:OUTPUT_RULE_DEGMER_QUADTREE,25|
dropup:INPUT_FORMAT_LATLON,dropup_latitude,dropup_longitude:OUTPUT_RULE_DEGMER_QUADTREE,25|
picktime:INPUT_FORMAT_DATE,tpep_pickup_datetime:OUTPUT_RULE_LOCALTIME_BINTREE,16,2016-12-30,1h|
droptime:INPUT_FORMAT_DATE,tpep_dropup_datetime:OUTPUT_RULE_LOCALTIME_BINTREE,16,2016-12-30,1h|
droptime:fmt_date,tpep_dropoff_datetime:1:16|
,binary,16,

# Json Representation of a Table

Thinking...

```
{
    "type": "error",
    "message":"104 Syntax Error"
}
{
    "type": "table";
    "rows":3,
    "columns": {
        "location": {
            "type":"path",
            "path_length": 3,
            "data": [2,1,0,2,1,1,2,1,3],
        }
        "time": {
            "type":"index",
            "data": [0,1,4]
        }
    },
    "values": [1.232, 1.2311, 0.322 ]
}
{
    "type": "table";
    "rows":3,
    "columns": {
        "location": {
            "type":"path",
            "path_length": 3,
            "data": [2,1,0,2,1,1,2,1,3],
        }
        "time": {
            "type":"index",
            "data": [0,1,4]
        }
    },
    "values": [1.232, 1.2311, 0.322 ]
}
```

# Typed table

(gdb) p *(self->type_table.types.begin)
$12 = {id = 0, name = {begin = 0x7fffed1d7190 "Undefined", end = 0x7fffed1d7199 ""}}
(gdb) p *(self->type_table.types.begin+1)
$13 = {id = 1, name = {begin = 0x7fffed1d9599 "NumberString+\001", end = 0x7fffed1d959f "String+\001"}}
(gdb) p *(self->type_table.types.begin+2)
$16 = {id = 2, name = {begin = 0x7fffed1d959f "String+\001", end = 0x7fffed1d95a5 "+\001"}}
(gdb) p *(self->type_table.types.begin+3)
$17 = {id = 3, name = {begin = 0x7fffed1d95c9 "PathMeasureTargetBindingpintseq\001", end = 0x7fffed1d95cd "MeasureTargetBindingpintseq\001"}}
(gdb) p *(self->type_table.types.begin+4)
$18 = {id = 4, name = {begin = 0x7fffed1d95cd "MeasureTargetBindingpintseq\001", end = 0x7fffed1d95d4 "TargetBindingpintseq\001"}}
(gdb) p *(self->type_table.types.begin+5)
$19 = {id = 5, name = {begin = 0x7fffed1d95d4 "TargetBindingpintseq\001", end = 0x7fffed1d95da "Bindingpintseq\001"}}
(gdb) p *(self->type_table.types.begin+6)
$21 = {id = 6, name = {begin = 0x7fffed1d95da "Bindingpintseq\001", end = 0x7fffed1d95e1 "pintseq\001

# Queries for testing

- finer:

crimes.b(\"kind\",dive(1))/crimes;

- equal

crimes.b(\"kind\",p(2))/crimes;

- coarser:

crimes/crimes.b(\"kind\",dive(1));

# Shell

c:/Users/llins/etc/bin/shell.bat

# Networking

Start server passing the callback function that
handles calls of the type

handle(NetworkConnection, char *request_begin, char *request_end)
{
    /api/query/crimes.b("time",intseq(1,3,4))
    /api/sources
    /api/schema/<source-name>

    after parsing

    PlatformNetwork_respond(NetworkConnection self, char *begin, char *end)
}


# On little crimes_nc.dmp

Allocator usage after 4000000 records in #pages is 715695
./nanocube_count ~/att/data/crimes/crimes_nc.dmp x
240.76s user 2.55s system 99% cpu 4:04.41 total

/Users/llins/projects/compressed_nanocube/src/build.sh

# see the pre-loaded macros

clang -dM -E -x c /dev/null

# Build

c:/work/compressed_nanocube/src/build.bat

# Todo:

1. Fix code reorg execution on the small example.
   A lot of rewrite, but the problem happens in the first
   insertion which is trivial. Check that.

//
// erro on a branch case: assertion
//
000 000 000
000 010 100
001 000 110
000 000 100

+		buffer	0x0000001fc84ff7f0 "/tmp/test_68417_p4-4_100001000010010000000000000100000000_b1_d3_l3.dot"	char[128]
//
// error on shared split (mfthreads advance)
//
100 001 000
010 010 000
000 000 000
100 000 000

//
// there is still another error on mfthreads assertion
// not sure where is the example.
//

0x0000006e84effdb0 "/tmp/test_7111_p4-4_010010000000001011100011_b1_d2_l3.dot"

// b3
010 010
000 000
001 011
100 011

# Debug memory usage

Goal: I want to have a good way to understand the
memory usage of our allocator:

#
# # Caches
# # Slabs per Cache
# # Occupied Slab Slots per Slab
#
# # PageMap_Level1 Usage
#

#
# Semantically Loaded traverse
#
# In terms of our application, maybe I just want
# to iterate over all INodes and PNodes
# and detail Caches
#

#
# I want to change the detail scheme to be in place.
#


Want to get a screenshot of the memory state.


# After 1M points crimes_nc.dmp

Allocator usage after 1000000 records in #pages is 151293

cache: Cache          cklen:   112 ckuse:       27 ckcap:       36 mem:     0MB usage:  75.0% pages:       1
cache: Slab           cklen:    48 ckuse:      244 ckcap:      255 mem:     0MB usage:  95.7% pages:       3
cache: NanocubeCount  cklen:  4464 ckuse:        1 ckcap:        1 mem:     0MB usage: 100.0% pages:       2
cache: detail1        cklen:     8 ckuse:  2971522 ckcap:  4142592 mem:    31MB usage:  71.7% pages:    8091
cache: detail2        cklen:     8 ckuse:   901555 ckcap:  1227264 mem:     9MB usage:  73.5% pages:    2397
cache: detail3        cklen:     8 ckuse:      276 ckcap:     2560 mem:     0MB usage:  10.8% pages:       5
cache: detail4        cklen:     8 ckuse:        1 ckcap:      512 mem:     0MB usage:   0.2% pages:       1
cache: detail6        cklen:     8 ckuse:        9 ckcap:      512 mem:     0MB usage:   1.8% pages:       1
cache: detail8        cklen:     8 ckuse:        0 ckcap:      512 mem:     0MB usage:   0.0% pages:       1
cache: detail12       cklen:    12 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail16       cklen:    16 ckuse:   163646 ckcap:   181760 mem:     2MB usage:  90.0% pages:     710
cache: detail24       cklen:    24 ckuse:  8721973 ckcap: 10486774 mem:   240MB usage:  83.2% pages:   61446
cache: detail32       cklen:    32 ckuse:   103874 ckcap:   136320 mem:     4MB usage:  76.2% pages:    1065
cache: detail48       cklen:    48 ckuse:   102726 ckcap:   136357 mem:     6MB usage:  75.3% pages:    1598
cache: detail64       cklen:    64 ckuse:    33505 ckcap:    45440 mem:     2MB usage:  73.7% pages:     710
cache: detail96       cklen:    96 ckuse:    28493 ckcap:    30288 mem:     2MB usage:  94.1% pages:     710
cache: detail128      cklen:   128 ckuse:     8234 ckcap:    10080 mem:     1MB usage:  81.7% pages:     315
cache: detail192      cklen:   192 ckuse:     3268 ckcap:     4476 mem:     0MB usage:  73.0% pages:     210
cache: detail256      cklen:   256 ckuse:      331 ckcap:      432 mem:     0MB usage:  76.6% pages:      27
cache: detail384      cklen:   384 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail512      cklen:   512 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail768      cklen:   768 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail1024     cklen:  1024 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail1536     cklen:  1536 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail2048     cklen:  2048 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail3072     cklen:  3072 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail4096     cklen:  4096 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: INode          cklen:    20 ckuse:  2448160 ckcap:  2485647 mem:    47MB usage:  98.5% pages:   12137
cache: PNode          cklen:    20 ckuse: 10591253 ckcap: 12584129 mem:   240MB usage:  84.2% pages:   61446

Most of the nodes are in the last dimension: the binary tree of time.
There are 10.5M nodes in that layer. The Nanocube TSeries wouldn't
have this layer and would replace it with a contiguous array.


# After the whole dataset we get

Allocator usage after 4011601 records in #pages is 715695
cache: Cache          cklen:   112 ckuse:       27 ckcap:       36 mem:     0MB usage:  75.0% pages:       1
cache: Slab           cklen:    48 ckuse:      285 ckcap:      425 mem:     0MB usage:  67.1% pages:       5
cache: NanocubeCount  cklen:  4464 ckuse:        1 ckcap:        1 mem:     0MB usage: 100.0% pages:       2
cache: detail1        cklen:     8 ckuse: 10949611 ckcap: 13982208 mem:   106MB usage:  78.3% pages:   27309
cache: detail2        cklen:     8 ckuse:  3050635 ckcap:  4142592 mem:    31MB usage:  73.6% pages:    8091
cache: detail3        cklen:     8 ckuse:      231 ckcap:     2560 mem:     0MB usage:   9.0% pages:       5
cache: detail4        cklen:     8 ckuse:        4 ckcap:      512 mem:     0MB usage:   0.8% pages:       1
cache: detail6        cklen:     8 ckuse:       33 ckcap:      512 mem:     0MB usage:   6.4% pages:       1
cache: detail8        cklen:     8 ckuse:        0 ckcap:      512 mem:     0MB usage:   0.0% pages:       1
cache: detail12       cklen:    12 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail16       cklen:    16 ckuse:   348296 ckcap:   409088 mem:     6MB usage:  85.1% pages:    1598
cache: detail24       cklen:    24 ckuse: 40684131 ckcap: 53089610 mem:  1215MB usage:  76.6% pages:  311072
cache: detail32       cklen:    32 ckuse:   254065 ckcap:   306816 mem:     9MB usage:  82.8% pages:    2397
cache: detail48       cklen:    48 ckuse:   300436 ckcap:   306852 mem:    14MB usage:  97.9% pages:    3596
cache: detail64       cklen:    64 ckuse:   143414 ckcap:   153408 mem:     9MB usage:  93.5% pages:    2397
cache: detail96       cklen:    96 ckuse:   132472 ckcap:   153422 mem:    14MB usage:  86.3% pages:    3596
cache: detail128      cklen:   128 ckuse:    38286 ckcap:    51136 mem:     6MB usage:  74.9% pages:    1598
cache: detail192      cklen:   192 ckuse:    14075 ckcap:    15142 mem:     2MB usage:  93.0% pages:     710
cache: detail256      cklen:   256 ckuse:     1464 ckcap:     1488 mem:     0MB usage:  98.4% pages:      93
cache: detail384      cklen:   384 ckuse:       10 ckcap:       10 mem:     0MB usage: 100.0% pages:       1
cache: detail512      cklen:   512 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail768      cklen:   768 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail1024     cklen:  1024 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail1536     cklen:  1536 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail2048     cklen:  2048 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail3072     cklen:  3072 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail4096     cklen:  4096 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: INode          cklen:    20 ckuse:  7183745 ckcap:  8389416 mem:   160MB usage:  85.6% pages:   40964
cache: PNode          cklen:    20 ckuse: 48733418 ckcap: 63707532 mem:  1215MB usage:  76.5% pages:  311072

From the 48M nodes, how many are leaves?
I would expect most of them to be leaves. If that is
the case detail16 should have been the most populated
chunk. Maybe some bug is causing the problem?
Gather more data


# Checking again the 100k fist data points

Allocator usage after 100001 records in #pages is 12467
cache: Cache         cklen:   112 ckuse:       27 ckcap:       36 mem:     0MB usage:  75.0% pages:       1
cache: Slab          cklen:    48 ckuse:      171 ckcap:      255 mem:     0MB usage:  67.1% pages:       3
cache: NanocubeCount cklen:  4464 ckuse:        1 ckcap:        1 mem:     0MB usage: 100.0% pages:       2
cache: detail1       cklen:     8 ckuse:   288565 ckcap:   363520 mem:     2MB usage:  79.4% pages:     710
cache: detail2       cklen:     8 ckuse:   136893 ckcap:   161280 mem:     1MB usage:  84.9% pages:     315
cache: detail3       cklen:     8 ckuse:      988 ckcap:     2560 mem:     0MB usage:  38.6% pages:       5
cache: detail4       cklen:     8 ckuse:        2 ckcap:      512 mem:     0MB usage:   0.4% pages:       1
cache: detail6       cklen:     8 ckuse:        0 ckcap:      512 mem:     0MB usage:   0.0% pages:       1
cache: detail8       cklen:     8 ckuse:        1 ckcap:      512 mem:     0MB usage:   0.2% pages:       1
cache: detail12      cklen:    12 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail16      cklen:    16 ckuse:    23240 ckcap:    23808 mem:     0MB usage:  97.6% pages:      93
cache: detail24      cklen:    24 ckuse:   593541 ckcap:   613710 mem:    14MB usage:  96.7% pages:    3596
cache: detail32      cklen:    32 ckuse:    15123 ckcap:    17920 mem:     0MB usage:  84.4% pages:     140
cache: detail48      cklen:    48 ckuse:    13123 ckcap:    17916 mem:     0MB usage:  73.2% pages:     210
cache: detail64      cklen:    64 ckuse:     2992 ckcap:     3968 mem:     0MB usage:  75.4% pages:      62
cache: detail96      cklen:    96 ckuse:     2538 ckcap:     2642 mem:     0MB usage:  96.1% pages:      62
cache: detail128     cklen:   128 ckuse:      809 ckcap:      864 mem:     0MB usage:  93.6% pages:      27
cache: detail192     cklen:   192 ckuse:      360 ckcap:      382 mem:     0MB usage:  94.2% pages:      18
cache: detail256     cklen:   256 ckuse:       42 ckcap:       48 mem:     0MB usage:  87.5% pages:       3
cache: detail384     cklen:   384 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail512     cklen:   512 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail768     cklen:   768 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail1024    cklen:  1024 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail1536    cklen:  1536 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail2048    cklen:  2048 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail3072    cklen:  3072 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: detail4096    cklen:  4096 ckuse:        0 ckcap:        0 mem:     0MB usage:   0.0% pages:       0
cache: INode         cklen:    20 ckuse:   307662 ckcap:   327262 mem:     6MB usage:  94.0% pages:    1598
cache: PNode         cklen:    20 ckuse:   770555 ckcap:  1104682 mem:    21MB usage:  69.8% pages:    5394

Allocator total memory: 48MB
----------
Num PNodes with degree 0           in full slabs: 210346
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 526106

Num PNodes with degree 0 in full slabs: 210346

There are 210346 leaves in PNode and full slabs and the number of detail2 is less
than 136k. Is there is an allocation bug here!!! Or I am not understanding the
problem.

500k PNodes that are not leaves: -----> 16 bytes + 2 bytes


# General idea to improve the detail field

Add 18+ bytes of detail to each node:
(1) It can handle 16-levels binary trees totally in place
(2) It can handle degree 2 nodes and have 16 bits of path space (8 levels of a quadtree)

Could still preserve contiguous paths locally plus
a pointer to children. Or vice-versa if degree is 2
and we have an extra 6 bytes for a pointer for
path detail.

Proposal 1:

22 bytes properties

Children of degree-2 nodes are stored locally
independent of path length.

If path length needs more than 6 bytes then it
needs to be far away.

f(degree, length) {

    if (degree <= 2) {

    }


}

# The same 100k entries

Allocator usage after 100001 records in #pages is 12462
cache: Cache               cklen:   112  ckuse:       27  ckcap:       36  mem:     0MB  usage:  75.0%  pages:       1
cache: Slab                cklen:    48  ckuse:      116  ckcap:      170  mem:     0MB  usage:  68.2%  pages:       2
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:    21347  ckcap:    23889  mem:     0MB  usage:  89.4%  pages:     140
cache: detail32            cklen:    32  ckuse:    13147  ckcap:    17920  mem:     0MB  usage:  73.4%  pages:     140
cache: detail48            cklen:    48  ckuse:     6467  ckcap:     7933  mem:     0MB  usage:  81.5%  pages:      93
cache: detail64            cklen:    64  ckuse:     2988  ckcap:     3968  mem:     0MB  usage:  75.3%  pages:      62
cache: detail96            cklen:    96  ckuse:     2546  ckcap:     2642  mem:     0MB  usage:  96.4%  pages:      62
cache: detail128           cklen:   128  ckuse:      809  ckcap:      864  mem:     0MB  usage:  93.6%  pages:      27
cache: detail192           cklen:   192  ckuse:      360  ckcap:      382  mem:     0MB  usage:  94.2%  pages:      18
cache: detail256           cklen:   256  ckuse:       42  ckcap:       48  mem:     0MB  usage:  87.5%  pages:       3
cache: detail384           cklen:   384  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode               cklen:    36  ckuse:   325373  ckcap:   409135  mem:    14MB  usage:  79.5%  pages:    3596
cache: PNode               cklen:    36  ckuse:   769094  ckcap:   920565  mem:    31MB  usage:  83.5%  pages:    8091
Allocator total memory: 48MB

----------
Num PNodes with degree 0           in full slabs: 180007
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 433700

Weird, the same 48M :(

The number of INodes and PNodes are different than the last run.
This is pointing to a bug...

# The same 1M entries

Allocator usage after 100000 records in #pages is 12462
Allocator usage after 200000 records in #pages is 24956
Allocator usage after 300000 records in #pages is 37316
Allocator usage after 400000 records in #pages is 55362
Allocator usage after 500000 records in #pages is 76598
Allocator usage after 600000 records in #pages is 82980
Allocator usage after 700000 records in #pages is 114145
Allocator usage after 800000 records in #pages is 123840
Allocator usage after 900000 records in #pages is 124902
Allocator usage after 1000000 records in #pages is 171153
Allocator usage after 1000001 records in #pages is 171153
cache: Cache               cklen:   112  ckuse:       27  ckcap:       36  mem:     0MB  usage:  75.0%  pages:       1
cache: Slab                cklen:    48  ckuse:      173  ckcap:      255  mem:     0MB  usage:  67.8%  pages:       3
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:   162144  ckcap:   181754  mem:     4MB  usage:  89.2%  pages:    1065
cache: detail32            cklen:    32  ckuse:    99898  ckcap:   136320  mem:     4MB  usage:  73.3%  pages:    1065
cache: detail48            cklen:    48  ckuse:    69154  ckcap:    90875  mem:     4MB  usage:  76.1%  pages:    1065
cache: detail64            cklen:    64  ckuse:    33923  ckcap:    45440  mem:     2MB  usage:  74.7%  pages:     710
cache: detail96            cklen:    96  ckuse:    28737  ckcap:    30288  mem:     2MB  usage:  94.9%  pages:     710
cache: detail128           cklen:   128  ckuse:     8253  ckcap:    10080  mem:     1MB  usage:  81.9%  pages:     315
cache: detail192           cklen:   192  ckuse:     3273  ckcap:     4476  mem:     0MB  usage:  73.1%  pages:     210
cache: detail256           cklen:   256  ckuse:      331  ckcap:      432  mem:     0MB  usage:  76.6%  pages:      27
cache: detail384           cklen:   384  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode               cklen:    36  ckuse:  2642706  ckcap:  3107145  mem:   106MB  usage:  85.1%  pages:   27309
cache: PNode               cklen:    36  ckuse: 10602526  ckcap: 15730218  mem:   540MB  usage:  67.4%  pages:  138254
Allocator total memory: 668MB
----------
Num PNodes with degree 0           in full slabs: 2202966
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 8283804



# All crimes ~4M

Allocator usage after 100000 records in #pages is 12462
Allocator usage after 200000 records in #pages is 24956
Allocator usage after 300000 records in #pages is 37316
Allocator usage after 400000 records in #pages is 55362
Allocator usage after 500000 records in #pages is 76598
Allocator usage after 600000 records in #pages is 82980
Allocator usage after 700000 records in #pages is 114145
Allocator usage after 800000 records in #pages is 123840
Allocator usage after 900000 records in #pages is 124902
Allocator usage after 1000000 records in #pages is 171153
Allocator usage after 1100000 records in #pages is 171508
Allocator usage after 1200000 records in #pages is 185696
Allocator usage after 1300000 records in #pages is 185854
Allocator usage after 1400000 records in #pages is 187394
Allocator usage after 1500000 records in #pages is 187394
Allocator usage after 1600000 records in #pages is 257150
Allocator usage after 1700000 records in #pages is 257150
Allocator usage after 1800000 records in #pages is 278527
Allocator usage after 1900000 records in #pages is 278764
Allocator usage after 2000000 records in #pages is 280096
Allocator usage after 2100000 records in #pages is 280275
Allocator usage after 2200000 records in #pages is 384861
Allocator usage after 2300000 records in #pages is 384861
Allocator usage after 2400000 records in #pages is 385660
Allocator usage after 2500000 records in #pages is 385660
Allocator usage after 2600000 records in #pages is 385660
Allocator usage after 2700000 records in #pages is 386015
Allocator usage after 2800000 records in #pages is 386015
Allocator usage after 2900000 records in #pages is 386015
Allocator usage after 3000000 records in #pages is 387051
Allocator usage after 3100000 records in #pages is 419069
Allocator usage after 3200000 records in #pages is 574797 <----- interesting spot here (did we allocate a new level1 node?)
Allocator usage after 3300000 records in #pages is 576027        a level 1 node occupies 6 bytes * 2^16... It is only 96
Allocator usage after 3400000 records in #pages is 577226        pages per level 1 node. Does not explain this 155k pages
Allocator usage after 3500000 records in #pages is 577227
Allocator usage after 3600000 records in #pages is 577227
Allocator usage after 3700000 records in #pages is 577227
Allocator usage after 3800000 records in #pages is 577760
Allocator usage after 3900000 records in #pages is 578959
Allocator usage after 4000000 records in #pages is 578959
Allocator usage after 4011601 records in #pages is 578960
cache: Cache               cklen:   112  ckuse:       27  ckcap:       36  mem:     0MB  usage:  75.0%  pages:       1
cache: Slab                cklen:    48  ckuse:      207  ckcap:      255  mem:     0MB  usage:  81.2%  pages:       3
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:   489549  ckcap:   613710  mem:    14MB  usage:  79.8%  pages:    3596
cache: detail32            cklen:    32  ckuse:   312594  ckcap:   460288  mem:    14MB  usage:  67.9%  pages:    3596
cache: detail48            cklen:    48  ckuse:   264577  ckcap:   306852  mem:    14MB  usage:  86.2%  pages:    3596
cache: detail64            cklen:    64  ckuse:   146541  ckcap:   153408  mem:     9MB  usage:  95.5%  pages:    2397
cache: detail96            cklen:    96  ckuse:   128906  ckcap:   153422  mem:    14MB  usage:  84.0%  pages:    3596
cache: detail128           cklen:   128  ckuse:    37217  ckcap:    51136  mem:     6MB  usage:  72.8%  pages:    1598
cache: detail192           cklen:   192  ckuse:    13814  ckcap:    15142  mem:     2MB  usage:  91.2%  pages:     710
cache: detail256           cklen:   256  ckuse:     1393  ckcap:     1488  mem:     0MB  usage:  93.6%  pages:      93
cache: detail384           cklen:   384  ckuse:       11  ckcap:       20  mem:     0MB  usage:  55.0%  pages:       2
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode               cklen:    36  ckuse:  8895441  ckcap: 10486770  mem:   360MB  usage:  84.8%  pages:   92169
cache: PNode               cklen:    36  ckuse: 46791735  ckcap: 53089605  mem:  1822MB  usage:  88.1%  pages:  466608

Allocator total memory: 2261MB
----------
Num PNodes with degree 0           in full slabs: 7033519
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 28359546


# time to build!
llins@DESKTOP-FF5NFHV ~/work/build/compressed_nanocube
$ time ./win32_nanocube_count ../../compressed_nanocube/data/crimes_nc.dmp x 2> x
real    4m2.564s
user    0m0.000s
sys     0m0.000s


# timing of the crimes_nc.dmp on little with the new detail scheme

Allocation table is the same
Allocator total memory: 2261MB
nanocube_count crimes_nc.dmp x
172.76s user 1.10s system 97% cpu 2:58.06 total <---- time was much better (cache friendly)

# found another bug with this example

Will call this case b4 or bug4

[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 3 1 2 3 3 1 0 1 2 1 0 0
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 3 2 2 3 1 1 3 0 2 0 3 0 1
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 1 records in #pages is 231
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 0 0 2 1 2 2 3 0 1 0 3 1 1
[1] -> 2 1 0 2 3 2 3 3 2 3 0 1 1 3 1 3 1 0 3 2 2 0 2 1 1
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 2 records in #pages is 231
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 3 1 1 2 2 0 3 1 3 3 0 1
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 3 2 0 1 3 0 2 2 2 3 1 0
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 3 records in #pages is 231
[0] -> 2 1 0 2 3 2 3 3 2 1 2 3 3 3 3 0 3 2 0 1 1 2 2 1 1
[1] -> 2 1 0 2 3 2 3 3 2 1 2 3 3 3 3 0 2 3 2 0 0 2 1 2 1
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 4 records in #pages is 231
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 0 0 0 2 1 0 0 3 3 2 0 0 0
[1] -> 2 1 0 2 3 2 3 3 2 1 3 2 3 2 3 2 3 1 0 2 3 3 2 1 0
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 5 records in #pages is 231
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 1 1 3 2 2 2 1 2 0 1 0 0
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 1 2 2 3 0 1 2 3 3 1 3 0
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 6 records in #pages is 231
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 0 3 0 2 2 0 1 0 0 0 2 0
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 2 1 3 0 1 2 1 0 0 3 0 0
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 7 records in #pages is 232
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 0 2 3 2 3 3 0 2 2 2 0 0
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 1 3 0 3 2 1 3 3 3 3 2 0
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 8 records in #pages is 232
[0] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 0 0 0 2 3 0 3 3 0 1 3 0
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 2 0 0 2 3 1 2 3 3 0 2 0 1
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1
Allocator usage after 9 records in #pages is 233
[0] -> 2 1 0 2 3 2 3 3 2 1 3 2 3 2 3 1 1 2 1 0 3 1 1 1 0
[1] -> 2 1 0 2 3 2 3 3 2 3 1 0 0 0 3 2 1 1 0 0 2 3 2 1 0
[2] -> 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1 1

The path on the third dimension is equal for all entries.
Essentially a 2d problem. Happening on a shared split
case.


# generate .pdf from .dot
find . | grep dot$ | xargs -I {} dot -Tpdf -o{}.pdf {}


# nytaxi is now running. 100k trips ---> 110MB

This is still the expensive case where time is not a tseries, but
a binary tree.

Allocator usage after 10000 records in #pages is 1804
Allocator usage after 20000 records in #pages is 3775
Allocator usage after 30000 records in #pages is 6747
Allocator usage after 40000 records in #pages is 10007
Allocator usage after 50000 records in #pages is 12704
Allocator usage after 60000 records in #pages is 14897
Allocator usage after 70000 records in #pages is 18943
Allocator usage after 80000 records in #pages is 21877
Allocator usage after 90000 records in #pages is 28301
Allocator usage after 100000 records in #pages is 28301
Allocator usage after 100000 records in #pages is 28301
cache: Cache               cklen:   112  ckuse:       27  ckcap:       36  mem:     0MB  usage:  75.0%  pages:       1
cache: Slab                cklen:    48  ckuse:       80  ckcap:       85  mem:     0MB  usage:  94.1%  pages:       1
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:   145142  ckcap:   181754  mem:     4MB  usage:  79.9%  pages:    1065
cache: detail32            cklen:    32  ckuse:    84149  ckcap:    90880  mem:     2MB  usage:  92.6%  pages:     710
cache: detail48            cklen:    48  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail64            cklen:    64  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail96            cklen:    96  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail128           cklen:   128  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail192           cklen:   192  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail256           cklen:   256  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail384           cklen:   384  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode               cklen:    36  ckuse:   835264  ckcap:   920565  mem:    31MB  usage:  90.7%  pages:    8091
cache: PNode               cklen:    36  ckuse:  1905669  ckcap:  2071426  mem:    71MB  usage:  92.0%  pages:   18206
Allocator total memory: 110MB
----------
Num PNodes with degree 0           in full slabs: 475786
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 905123

# 1M Taxi Trips

Allocator usage after 10000 records in #pages is 1804
Allocator usage after 20000 records in #pages is 3775
Allocator usage after 30000 records in #pages is 6747
Allocator usage after 40000 records in #pages is 10007
Allocator usage after 50000 records in #pages is 12704
Allocator usage after 60000 records in #pages is 14897
Allocator usage after 70000 records in #pages is 18943
Allocator usage after 80000 records in #pages is 21877
Allocator usage after 90000 records in #pages is 28301
Allocator usage after 100000 records in #pages is 28301
Allocator usage after 110000 records in #pages is 41805
Allocator usage after 120000 records in #pages is 41805
Allocator usage after 130000 records in #pages is 42339
Allocator usage after 140000 records in #pages is 42339
Allocator usage after 150000 records in #pages is 42339
Allocator usage after 160000 records in #pages is 56527
Allocator usage after 170000 records in #pages is 62596
Allocator usage after 180000 records in #pages is 62596
Allocator usage after 190000 records in #pages is 63395
Allocator usage after 200000 records in #pages is 63395
Allocator usage after 210000 records in #pages is 83973
Allocator usage after 220000 records in #pages is 84772
Allocator usage after 230000 records in #pages is 84772
Allocator usage after 240000 records in #pages is 84772
Allocator usage after 250000 records in #pages is 93875
Allocator usage after 260000 records in #pages is 93875
Allocator usage after 270000 records in #pages is 93875
Allocator usage after 280000 records in #pages is 95074
Allocator usage after 290000 records in #pages is 125797
Allocator usage after 300000 records in #pages is 125797
Allocator usage after 310000 records in #pages is 125797
Allocator usage after 320000 records in #pages is 126996
Allocator usage after 330000 records in #pages is 126996
Allocator usage after 340000 records in #pages is 126996
Allocator usage after 350000 records in #pages is 126996
Allocator usage after 360000 records in #pages is 140747
Allocator usage after 370000 records in #pages is 140747
Allocator usage after 380000 records in #pages is 140747
Allocator usage after 390000 records in #pages is 140747
Allocator usage after 400000 records in #pages is 188630
Allocator usage after 410000 records in #pages is 188630
Allocator usage after 420000 records in #pages is 188630
Allocator usage after 430000 records in #pages is 188630
Allocator usage after 440000 records in #pages is 188630
Allocator usage after 450000 records in #pages is 188630
Allocator usage after 460000 records in #pages is 190428
Allocator usage after 470000 records in #pages is 190428
Allocator usage after 480000 records in #pages is 190428
Allocator usage after 490000 records in #pages is 190428
Allocator usage after 500000 records in #pages is 190428
Allocator usage after 510000 records in #pages is 190428
Allocator usage after 520000 records in #pages is 190428
Allocator usage after 530000 records in #pages is 190428
Allocator usage after 540000 records in #pages is 211006
Allocator usage after 550000 records in #pages is 211006
Allocator usage after 560000 records in #pages is 280229
Allocator usage after 570000 records in #pages is 280229
Allocator usage after 580000 records in #pages is 280229
Allocator usage after 590000 records in #pages is 280229
Allocator usage after 600000 records in #pages is 282926
Allocator usage after 610000 records in #pages is 282926
Allocator usage after 620000 records in #pages is 282926
Allocator usage after 630000 records in #pages is 282926
Allocator usage after 640000 records in #pages is 282926
Allocator usage after 650000 records in #pages is 282926
Allocator usage after 660000 records in #pages is 285623
Allocator usage after 670000 records in #pages is 285623
Allocator usage after 680000 records in #pages is 285623
Allocator usage after 690000 records in #pages is 285623
Allocator usage after 700000 records in #pages is 285623
Allocator usage after 710000 records in #pages is 285623
Allocator usage after 720000 records in #pages is 285623
Allocator usage after 730000 records in #pages is 285623
Allocator usage after 740000 records in #pages is 285623
Allocator usage after 750000 records in #pages is 285623
Allocator usage after 760000 records in #pages is 285623
Allocator usage after 770000 records in #pages is 285623
Allocator usage after 780000 records in #pages is 285623
Allocator usage after 790000 records in #pages is 420229
Allocator usage after 800000 records in #pages is 420229
Allocator usage after 810000 records in #pages is 420229
Allocator usage after 820000 records in #pages is 420229
Allocator usage after 830000 records in #pages is 420229
Allocator usage after 840000 records in #pages is 420229
Allocator usage after 850000 records in #pages is 420229
Allocator usage after 860000 records in #pages is 420229
Allocator usage after 870000 records in #pages is 420229
Allocator usage after 880000 records in #pages is 424275
Allocator usage after 890000 records in #pages is 424275
Allocator usage after 900000 records in #pages is 424275
Allocator usage after 910000 records in #pages is 424275
Allocator usage after 920000 records in #pages is 424275
Allocator usage after 930000 records in #pages is 424275
Allocator usage after 940000 records in #pages is 424275
Allocator usage after 950000 records in #pages is 424275
Allocator usage after 960000 records in #pages is 428321
Allocator usage after 970000 records in #pages is 428321
Allocator usage after 980000 records in #pages is 428321
Allocator usage after 990000 records in #pages is 428321
Allocator usage after 1000000 records in #pages is 428321
Allocator usage after 1000000 records in #pages is 428321
cache: Cache               cklen:   112  ckuse:       27  ckcap:       36  mem:     0MB  usage:  75.0%  pages:       1
cache: Slab                cklen:    48  ckuse:      106  ckcap:      170  mem:     0MB  usage:  62.4%  pages:       2
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:  1576143  ckcap:  2071373  mem:    47MB  usage:  76.1%  pages:   12137
cache: detail32            cklen:    32  ckuse:  1086058  ckcap:  1553536  mem:    47MB  usage:  69.9%  pages:   12137
cache: detail48            cklen:    48  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail64            cklen:    64  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail96            cklen:    96  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail128           cklen:   128  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail192           cklen:   192  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail256           cklen:   256  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail384           cklen:   384  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode               cklen:    36  ckuse:  8892953  ckcap: 10486770  mem:   360MB  usage:  84.8%  pages:   92169
cache: PNode               cklen:    36  ckuse: 32066644  ckcap: 35393065  mem:  1215MB  usage:  90.6%  pages:  311072
Allocator total memory: 1673MB

Num PNodes with degree 0           in full slabs: 4732363
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 18862971

nanocube_count nytaxi_sample_1M.dmp x  303.31s user 0.63s system 95% cpu 5:19.78 total

# Run again chicago crime on little (after fixes)

Allocator usage after 4011601 records in #pages is 576561
cache: Cache               cklen:   112  ckuse:       27  ckcap:       36  mem:     0MB  usage:  75.0%  pages:       1
cache: Slab                cklen:    48  ckuse:      204  ckcap:      255  mem:     0MB  usage:  80.0%  pages:       3
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:   317953  ckcap:   409081  mem:     9MB  usage:  77.7%  pages:    2397
cache: detail32            cklen:    32  ckuse:   238013  ckcap:   306816  mem:     9MB  usage:  77.6%  pages:    2397
cache: detail48            cklen:    48  ckuse:   236189  ckcap:   306852  mem:    14MB  usage:  77.0%  pages:    3596
cache: detail64            cklen:    64  ckuse:   143414  ckcap:   153408  mem:     9MB  usage:  93.5%  pages:    2397
cache: detail96            cklen:    96  ckuse:   132472  ckcap:   153422  mem:    14MB  usage:  86.3%  pages:    3596
cache: detail128           cklen:   128  ckuse:    38286  ckcap:    51136  mem:     6MB  usage:  74.9%  pages:    1598
cache: detail192           cklen:   192  ckuse:    14075  ckcap:    15142  mem:     2MB  usage:  93.0%  pages:     710
cache: detail256           cklen:   256  ckuse:     1464  ckcap:     1488  mem:     0MB  usage:  98.4%  pages:      93
cache: detail384           cklen:   384  ckuse:       10  ckcap:       10  mem:     0MB  usage: 100.0%  pages:       1
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode               cklen:    36  ckuse:  7183745  ckcap: 10486770  mem:   360MB  usage:  68.5%  pages:   92169
cache: PNode               cklen:    36  ckuse: 48733418  ckcap: 53089605  mem:  1822MB  usage:  91.8%  pages:  466608
Allocator total memory: 2252MB

Num PNodes with degree 0           in full slabs: 6755602
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 28637463

 sed s/Type/Node/g nanocube_ptr.template > nanocube_index_templates.c;\
 sed s/Type/Child/g nanocube_ptr.template >> nanocube_index_templates.c;\
 sed s/Type/Label/g nanocube_ptr.template >> nanocube_index_templates.c;\

 sed s/Type/NodeP/g nanocube_list.template > a;\
 sed s/Type/u8/g nanocube_list.template >> a;\


# C Implementation: first run on little

Allocator usage after 4011601 records in #pages is 576561
cache: Cache               cklen:   104  ckuse:       27  ckcap:       39  mem:     0MB  usage:  69.2%  pages:       1
cache: Slab                cklen:    40  ckuse:      204  ckcap:      306  mem:     0MB  usage:  66.7%  pages:       3
cache: NanocubeCount       cklen:  4464  ckuse:        1  ckcap:        1  mem:     0MB  usage: 100.0%  pages:       2
cache: detail1             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail6             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail8             cklen:     8  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail12            cklen:    12  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail16            cklen:    16  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail24            cklen:    24  ckuse:   317953  ckcap:   409081  mem:     9MB  usage:  77.7%  pages:    2397
cache: detail32            cklen:    32  ckuse:   238013  ckcap:   306816  mem:     9MB  usage:  77.6%  pages:    2397
cache: detail48            cklen:    48  ckuse:   236189  ckcap:   306852  mem:    14MB  usage:  77.0%  pages:    3596
cache: detail64            cklen:    64  ckuse:   143414  ckcap:   153408  mem:     9MB  usage:  93.5%  pages:    2397
cache: detail96            cklen:    96  ckuse:   132472  ckcap:   153422  mem:    14MB  usage:  86.3%  pages:    3596
cache: detail128           cklen:   128  ckuse:    38286  ckcap:    51136  mem:     6MB  usage:  74.9%  pages:    1598
cache: detail192           cklen:   192  ckuse:    14075  ckcap:    15142  mem:     2MB  usage:  93.0%  pages:     710
cache: detail256           cklen:   256  ckuse:     1464  ckcap:     1488  mem:     0MB  usage:  98.4%  pages:      93
cache: detail384           cklen:   384  ckuse:       10  ckcap:       10  mem:     0MB  usage: 100.0%  pages:       1
cache: detail512           cklen:   512  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail768           cklen:   768  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1024          cklen:  1024  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail1536          cklen:  1536  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail2048          cklen:  2048  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail3072          cklen:  3072  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: detail4096          cklen:  4096  ckuse:        0  ckcap:        0  mem:     0MB  usage:   0.0%  pages:       0
cache: INode              cklen:    36  ckuse:  7183745  ckcap: 10486770  mem:   360MB  usage:  68.5%  pages:   92169
cache: PNode              cklen:    36  ckuse: 48733418  ckcap: 53089605  mem:  1822MB  usage:  91.8%  pages:  466608
Allocator total memory: 2252MB

Num PNodes with degree 0           in full slabs: 6755602
Num PNodes with degree 2 path 0    in full slabs: 0
Num PNodes with degree 2 path >= 1 in full slabs: 28637463

nanocube_count crimes_nc.dmp x  183.06s user 0.92s system 98% cpu 3:07.46 total


Looks like the result is the same. Good. The time with O3 and not checjing assertions is 3.1min.

# Next Steps

1. Save nanocube count files.

```
# 1. in memory creation of a file
nanocube_count new crimes_nc.dmp crimes.nc

# 2. stats
nanocube_count memory crimes.nc

# 3. simple Q&A interface
nanocube_count shell crimes.nc

$ q count
$ q count.a("location",dive([],4))
$ q count.r("location",[2,1,0])

# 3. load as an http server
nanocube_count serve crimes.nc 12345
```

The platform would provide the following services:

```
b8          platform_dump_memory_to_file(MemoryBlock b, const char* filename);
MemoryBlock platform_alloc_page_aligned_memory(u64 size, u64 preferred_location);
MemoryBlock platform_mmap_file(const char* filename);
b8          platform_free(MemoryBlock mb);
```

Algebra please!!!

//
// measures
// relational calculus
//

measure and a way to combine them

count.constrain

all=filter(location,ANCHOR,{@012012130+8,@012012131+8}).filter(kind,RESTRICT,"THEFT")
theft=filter(location,ANCHOR,{@012012127+8,@012012131+8})
(theft>100)*theft/all*100

Allow for '.' operator. Semantic analysis will forbid 1.3 for example.








//
// define a measure of theft percentage, but crimes
// need to be 100 or more
//
// assume crime is a measure that is available
// maybe there is a default measure symbol: % or $
//

//
// precedence
//
// function calling
// .                      chain operator
// * /
// + -
// < > == != <= >=
// && ||
// =                      assignment
//

theft = crimes . r("kind",@2) . r("time",interval(0,100));

theftcoef = thefts/crimes * 100 * (crimes >= 100);

theftcoef . a("location",dive(@,8))

A measure has the following properties:
- dimensions
- each dimension in a measure is either unbound or bounded
- the free dimensions of a measure are its unbounded dimensions
- when a dimension is bound, this this bining can be anchored or not anchored
- a dimension binding is defined by either one (singleton) target or a (named) list of targets.
- a measure can be evaluated
- one can obtain a new measure by binding/unbinding dimensions of a measure differently
- logical and arithmetic expressions of measures are also considered measures

(order) named-dimension | anchor-dim | value

Alignment:

- Semantic mapping:
- register symbols with a type (bootstrap)
- in our case measures

.        : measure x binding -> measure
dive     : path    x int     -> target
interval : int     x int     -> target

1. compute AST and solve priority of binary operators
2. semantic mapping:

list of statements
assignments
expressions

a . b
a / b
a + b
a - b

find type of a
find type of b
is there a funcition . : typeof(a) x typeof(b)

Symbol

    type (e.g. int, float, string, measure, target, binding)

A measure can be

- constant
- simple
- compounded

thefts > 1.2 * kidnap

- Binary operators precedence


struct Type
{
    u32   id;
    u32   name_length;
    char *name;
} Type;

//
// primitive types: Function, Type, Int, Float, String, Measure
//



// 1. syntax is correct
// 2. types are compatible
// 3. evaluate (store in a evaluation compatible format)

typedef struct Measure
{


} Measure;

typedef struct Binding
{
} Binding;




// Type
typedef u32 Type;

TypesTable types;

SymbolsTable_insert(symbols_table, "crimes", measure_type);





