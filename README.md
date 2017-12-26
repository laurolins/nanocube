# Nanocubes: an in-memory data structure for spatiotemporal data cubes

Nanocubes are a fast data structure for in-memory data cubes developed at the [Information Visualization department](http://www.research.att.com/infovis) at [AT&T Labs Research](http://www.research.att.com). Visualizations powered by nanocubes can be used to explore datasets with billions of elements at interactive rates in a web browser, and in some cases nanocubes uses sufficiently little memory that you can run a nanocube in a modern-day laptop.

# About this branch

This branch (`v4`) contains a new implementation of Nanocubes in the C programming language. The goal with this new implementation was to get a much finer control in all aspects of the data structure and specially on its memory aspects (allocation, layout). In our original C++ template-based implementation of Nanocubes (up to version 3.3), we implemented the Nanocube data structure on top of C++ STL (standard library) and while this was a reasonable solution at the time, it had some important downsides: (1) complex serialization which made it hard to save/load Nanocube into files; (2) variations in the internal memory layout of a Nanocube based on the specific STL implementation we used.

# Compiling on Linux or Mac

```shell
# clone the v4 branch
git clone -b v4 git@github.com:laurolins/nanocube
cd nanocube
autoreconf -vi
mkdir build
cd build
../configure --with-polycover --prefix=../local
make -j
./nanocube
```

# Paper Example

```shell
# create nanocube
nanocube create paper-example.csv paper-example.map paper-example.nanocube

# generate graphviz `.dot` file
nanocube draw paper-example.nanocube paper-example.dot

# generate `.pdf`
dot -Tpdf -opaper-example.pdf paper-example.dot
```

In the next figure we present the compressed-path version of the nanocube
example in Figure 2 of the [original Nanocubes paper](http://nanocubes.net/assets/pdf/nanocubes_paper.pdf).
Path that don't branch collapse into a single node.

![paper-example](/doc/paper-example.png)

## Memory Usage

To check details of how the memory is being used inside a nanocube one can run
the `memory` command.

```shell
# check the memory consumption
$ nanocube memory paper-example.nanocube
cache: Cache        cklen:   104  ckuse:  32  ckcap:    39  mem:  0MB  usage:    82% pages:       1
cache: Slab         cklen:    40  ckuse:   9  ckcap:   102  mem:  0MB  usage:     8% pages:       1
cache: nv_Nanocube  cklen:  4728  ckuse:   1  ckcap:     1  mem:  0MB  usage:   100% pages:       2
cache: bt_Node_0    cklen:  4080  ckuse:   1  ckcap:     1  mem:  0MB  usage:   100% pages:       1
cache: bt_BlockKV_0 cklen:    32  ckuse:   4  ckcap:   128  mem:  0MB  usage:     3% pages:       1
cache: detail1      cklen:     8  ckuse:   0  ckcap:     0  mem:  0MB  usage:     0% pages:       0
...
cache: INode        cklen:    36  ckuse:   6  ckcap:   113  mem:  0MB  usage:     5% pages:       1
cache: PNode        cklen:    36  ckuse:  10  ckcap:   113  mem:  0MB  usage:     8% pages:       1
cache: nv_payload   cklen:     8  ckuse:  10  ckcap:   512  mem:  0MB  usage:     1% pages:       1
INodes by degree...
INodes with degree   0 ->             4
INodes with degree   2 ->             1
INodes with degree   3 ->             1
PNodes by degree...
PNodes with degree   0 ->             8
PNodes with degree   2 ->             2
Number of records: 5
Allocator total memory: 1MB
```

Note the row starting with `cache: nv_payload` and the column `ckuse`. It says
that there are 10 payloads being stored in this nanocube, but what is more
interesting about this number is that it also corresponds to the number of
unique sets of records induced by the set of all `product bins` (*ie* choose
one bin from each dimension hierarchy) possible in the hierarchies of the
nanocube. If not for anything else, one can say that nanocube is an algorithm
to count the number of unique record sets yielded by all possible
multi-dimensional binnings provided as input. The right nodes on the figure
above is a visual representation for the meaning of the number 10 in this
example.

### Order makes a difference in terms memory usage

Depending on the order in which we set the index dimensions of a nanocube, the
internal representation changes and can be more or less expensive in terms of
memory. In the paper example, if we use the `device` dimension first and
the `location` dimension second we end up with a smaller nanocube:

![paper-example-permuted](/doc/paper-example-permuted.png)

While the first layer in the paper example used 6 nodes, the same layer
in the permuted version used only 3 nodes. It is also interesting to note
that each node in the last layer of a path compressed nanocube corresponds
to a unique payload. In other words, a unique set of records yielded by
the possible `product bins` is also in one-to-one correspondence with
the nodes in the last layer (last index dimension) of a path compressed
nanocube. In this example 10!

# Chicago Crime Example

Here is example on how to create a path compressed nanocube index from a
`.csv` file with the following characteristics:
* index dimensions:
   - `location` - a quadtree of height (25+1) based on latitude and longitude in degrees.
   - `type` - a tree of height (1+1) whose root has degree at most 256 (8 bits resolution).
   - `time` - a binary tree of height (16+1).
* measure dimensions:
   - `count` - unsigned 32-bit integer.

The `.csv` test file for this example has a sample of 10k Chicago crime records
published by the City of Chicago Data Portal (data.cityofchicago.org).

The `nanocube csv mapping` file that specifies how to map the `.csv` fields
in this dataset into the nested hierarchy specification we described above
is shown below.

```python
# I1. quadtree index dimension for the pickup location
index_dimension('location',input('Latitude','Longitude'),latlon(25));

# I2. categorical dimension
index_dimension('type',input('Primary Type'),categorical(8,1));

# I3. quadtree index dimension for the dropoff location
index_dimension('time',
                input('Date'),
                time(16,                          # binary tree with 16 levels (slots 0 to 65535)
                     '2000-01-01T00:00:00-06:00', # *base* date time
                     3600,                        # *width* in secs. interval [base, base + width)
                     6*60                         # add 360 minutes to all input values since input
                                                  # comes as if it is UTC and is actually -06:00)
                    ));

# M1. measure dimension for counting  (u32 means integral non-negative and 2^32 - 1 max value)
# if no input .csv column is used in a measure dimention, it functions as a count
# (ie. one for each input record)
measure_dimension('count',input(),u32);
```

Here is a sequence of commnads to create a nanocube file,
serve it through HTTP and query it:

```shell
# create nanocube
nanocube create chicago-crimes-10k-sample.csv chicago-crimes.map chicago-crimes-10k-sample.nanocube

# serve it on port 8000
nanocube serve 8000 crimes=chicago-crimes-10k-sample.nanocube

# query all its measures (count only) aggregate for the whole dataset
wget -qO- 'http://localhost:8000/format('text');q(crimes);'
#    #         count
#    1  1.000000e+04

# query group by type
wget -qO- "http://localhost:8000/format('text');q(crimes.b('type',dive(1),'name'));"
#    #                                              type         count
#    1                                                 0  7.900000e+01
#    2                                                 1  1.837000e+03
#    3                                                 2  6.390000e+02
#    4                                                 3  2.096000e+03
#    5                                                 4  4.680000e+02
#    6                                                 5  1.074000e+03
#    7                                                 6  5.950000e+02
#    8                                                 7  3.680000e+02
#    9                                                 8  5.980000e+02
#   10                                                 9  1.117000e+03
#   11                                                10  3.020000e+02
#   12                                                11  3.710000e+02
#   13                                                12  1.500000e+01
#   14                                                13  9.900000e+01
#   15                                                14  7.700000e+01
#   16                                                15  8.300000e+01
#   17                                                16  2.900000e+01
#   18                                                17  2.800000e+01
#   19                                                18  1.500000e+01
#   20                                                19  1.000000e+00
#   21                                                20  4.200000e+01
#   22                                                21  2.500000e+01
#   23                                                22  1.800000e+01
#   24                                                23  7.000000e+00
#   25                                                24  1.200000e+01
#   26                                                25  5.000000e+00
```

# Taxi Example

Example on how to create a nanocube compressed index, perform a command line
query and draw the guts of the index.

```python
#
# Create Compressed Nanocube Index (.cnc) from .csv file with initial 10 records on csv file
# taxi.map content is
#
# index_dimension('pickup_location',input('pickup_latitude','pickup_longitude'),latlon(25));
# index_dimension('dropoff_location',input('dropoff_latitude','dropoff_longitude'),latlon(25));
# index_dimension('pickup_time',input('tpep_pickup_datetime'),time(16,'2016-01-01T00:00:00-05:00',3600));
# measure_dimension('count',input(),u32);
# measure_dimension('tip',input('tip_amount'),f32);
# measure_dimension('fare',input('fare_amount'),f32);
# measure_dimension('distance',input('trip_distance'),f32);
#
nanocube create taxi.csv taxi.map taxi.nanocube -filter=0,10

# Query coarsest product-bin
nanocube query taxi.nanocube taxi;
#    #         count           tip          fare      distance
#    1  1.000000e+01  1.716000e+01  1.345000e+02  3.045000e+01

# Generate graphviz `.dot` file with Compressed Nanocube Index drawing
nanocube draw taxi.nanocube taxi.dot
# [draw:msg] To generate .pdf of graph run:
# dot -Tpdf -odrawing.pdf taxi.dot

```

# Query API

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
p(path:[<number>[,<number>]*])
```

Focus only on node described by the input *path*. No branching on the report
occurs.

### tile2d or img2d

```
tile2d(level, x, y)
img2d(level, x, y)    # this one is the slippy tile coords
```

Returns a path.

### Dive

```
dive(levels:<number>)
dive(base:<path>, depth:<number>)
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
```

### Interval

```
interval(a:<number>, b:<number>)
```

Focus only on records in [a,b) interval. No branching on the report occurs.
Valid on binary hierarchies.


### Interval Sequence

```
intseq(base:<number>, width:<number>, count:<number>)
```

Generate a sequence of results. Coarsest hierarchy nodes that fall in the
interval `[base, base + width)` will be reported with label 0. Coarsed
hierarchy nodes that fall in the interval `[base + width, base + 2*width)` will
be reported with label 1, and so on up to `[base + (count-1)*width, base +
count*width)`.  This target branches the result report.

### Time Series and Time Series Aggregate

```
timeseries(base:<string>, width_secs:<number>, count:<number>, stride_secs:<number>)
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

# first hour of 24 days starting in June 1st, 2016
timeseries('2016-06-01 00:00:00 -05:00', 3600, 24, 24*3600)

# same weekday for 4 weeks starting on June 1st, 2016
timeseries('2016-06-01 00:00:00 -05:00', 24*3600, 4, 7*24*3600)
```

# Commands

## `create`

Create index based on a csv file.

```
nanocube create <csv-filename> <mapping-filename> <ouput-filename> [-filter=<offset>,<num-records>]
```

## `draw`

```
nanocube draw <nanocube-filename> <dot-filename>
```

## `serve`

```
nanocube serve <port-number> ([<alias>=]<nanocube-filename>)+
```

## `memory`

```
nanocube memory <nanocube-filename> <dot-filename>
```

Pring memory map of a *nanocube index*.

## `query`

```
nanocube query <nanocube-filename> <query>
```

## `ast`

```
nanocube ast <query>
```

Print parsing result structure of a *query*.

# CSV import API

## `index_dimension`

```
index_dimension(<name>,<input>,<mapping>)
```

## `measure_dimension`

```
measure_dimension(<name>,<input>,<mapping>)
measure_dimension(<name>,<input>,<number-storage>)
```

## `duration` and `duration2`

```
duration(<number-storage>,<time_unit_in_secs>>)
duration2(<number-storage>,<time_unit_in_secs>>)
```

Measure the duration in some units based on two calendar date-time in the original csv.


# Implementation

Placeholder for implementation notes.

## Modules

```
al_   allocation
bt_   btree
nt_   tokenizer
np_   query API
nu_   util
nx_   index
pt_   platform
cm_   csv mapping API
nv_   nanocube vector
tm_   date and time
ntp_  date and time parser
```

