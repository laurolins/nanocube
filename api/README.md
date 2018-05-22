# API version 4 (or API_v4)

We will show the query API_v4 for Nanocubes using the crime50k example.
In order to create such a nanocube and serve it via http, we
run the following script

```shell
#
# (0) go the the $NANOCUBE/data folder
#
# (1) create the nanocube index file 'crime50k.nanocube'
#
nanocube create <(gunzip -c crime50k.csv.gz) crime50k.map crime50k.nanocube
#
# (2) serve the index via http on port 51234 using the alias 'crimes'
#
nanocube serve 51234 crimes=crime50k.nanocube
```

## schema

To serve the schemas of all the Nanocube indices being served in port 51234 of
localhost we send a GET request to:
```url
http://localhost:51234/schema()
```
Note that in our case we only have one Nanocube index being served for the
crime50k example. The result of the request above is a `json` array with 
one entry: the schema of the crimes index:
```json
[
	{
		"type":"schema",
		"name":"crimes",
		"index_dimensions":[
			{
				"index":0,
				"name":"location",
				"bits_per_level":2,
				"num_levels":25,
				"hint":"spatial",
				"aliases":{
				}
			},
			{
				"index":1,
				"name":"type",
				"bits_per_level":8,
				"num_levels":1,
				"hint":"categorical",
				"aliases":{
					"11":"CRIMINAL TRESPASS",
					"8":"ROBBERY",
					"24":"NON-CRIMINAL",
					"27":"OBSCENITY",
					"30":"NON-CRIMINAL (SUBJECT SPECIFIED)",
					"3":"BATTERY",
					"26":"GAMBLING",
					"23":"INTIMIDATION",
					"2":"THEFT",
					"4":"BURGLARY",
					"17":"LIQUOR LAW VIOLATION",
					"29":"CONCEALED CARRY LICENSE VIOLATION",
					"21":"PROSTITUTION",
					"16":"CRIM SEXUAL ASSAULT",
					"12":"WEAPONS VIOLATION",
					"20":"SEX OFFENSE",
					"7":"DECEPTIVE PRACTICE",
					"10":"OTHER OFFENSE",
					"25":"PUBLIC INDECENCY",
					"14":"PUBLIC PEACE VIOLATION",
					"15":"INTERFERENCE WITH PUBLIC OFFICER",
					"22":"ARSON",
					"1":"CRIMINAL DAMAGE",
					"13":"KIDNAPPING",
					"5":"MOTOR VEHICLE THEFT",
					"0":"NARCOTICS",
					"18":"HOMICIDE",
					"9":"ASSAULT",
					"28":"OTHER NARCOTIC VIOLATION",
					"6":"OFFENSE INVOLVING CHILDREN",
					"19":"STALKING"
				}
			},
			{
				"index":2,
				"name":"time",
				"bits_per_level":1,
				"num_levels":16,
				"hint":"temporal|2013-12-01T06:00:00Z_3600s",
				"aliases":{
				}
			}
		],
		"measure_dimensions":[
			{
				"order":1,
				"name":"count",
				"type":"u32"
			}
		]
	}
]
```

## total

In order to get the total number of crimes indexed in `crimes` we
send the query `q(crimes)`:
```url
http://localhost:51234/q(crimes)
```
The total is 50k, since we indexed 50k records and the `value` measure
we defined in `crime50k.map` is simply a count of records.
```json
[
	{
		"type":"table",
		"numrows":1,
		"index_columns":[
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					50000.000000
				]
			}
		]
	}
]
```

## location

The quadtree convention we use is the following:
```
0 bottom-left  quadrant (in the first split of the mercator projection contains South-America)
1 bottom-right quadrant (in the first split of the mercator projection contains Africa and South Asia)
2 top-left     quadrant (in the first split of the mercator projection contains North America)
3 top-right    quadrant (in the first split of the mercator projection contains Europe and North Asia)
```
If we want to count the number of crimes in a 256x256 quadtree-aligned grid
of the sub-cell (top-left, bottom-right, top-left) or (2,1,2), we run the query
below. Note that the cell (2,1,2) contains Chicago.
```
http://localhost:29512/q(crimes.b('location',dive(p(2,1,2),8)))
```
The result is 
```json
[
	{
		"type":"table",
		"numrows":8,
		"index_columns":[
			{
				"name":"location",
				"hint":"none",
				"values_per_row":11,
				"values":[
					2,1,2,0,0,0,0,1,2,3,3,2,1,2,0,0,0,0,1,3,0,2,2,1,2,0,0,0,0,1,3,0,3,2,1,2,0,0,0,0,1,3,1,2,2,1,2,0,0,0,0,1,3,2,0,2,1,2,0,0,0,0,1,3,2,1,2,1,2,0,0,0,0,1,3,2,2,2,1,2,0,0,0,0,1,3,2,3
				]
			}
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					272.000000,416.000000,12268.000000,224.000000,6838.000000,16913.000000,5460.000000,7609.000000
				]
			}
		]
	}
]
```

Note that the way the resulting grid cells are described are as paths. Since we
subdivided the cell (2,1,2) at depth 3 more 8 times (the depth of the dive), we
get that each grid cell is described by a path of 11 subdivisions. So the way
to read the result above is to split location 'values' array at every 11-th entry.
The final correspondence can also be better seen by generating the above resul in
text format.
```
http://localhost:29512/format('text');q(crimes.b('location',dive(p(2,1,2),8)))
```
which yields the following text
```
                                          location                           count
                             2,1,2,0,0,0,0,1,2,3,3                      272.000000
                             2,1,2,0,0,0,0,1,3,0,2                      416.000000
                             2,1,2,0,0,0,0,1,3,0,3                    12268.000000
                             2,1,2,0,0,0,0,1,3,1,2                      224.000000
                             2,1,2,0,0,0,0,1,3,2,0                     6838.000000
                             2,1,2,0,0,0,0,1,3,2,1                    16913.000000
                             2,1,2,0,0,0,0,1,3,2,2                     5460.000000
                             2,1,2,0,0,0,0,1,3,2,3                     7609.000000
```
If instead of the path we just want the local (x,y) coordinate of the 256x256 grid,
we can use the hint `'img8'` as the last parameter of the binding `.b(...)`.
```
http://localhost:29512/format('text');q(crimes.b('location',dive(p(2,1,2),8),'img8'))
```
We now get coordinates `x` and `y` both in {0,1,...,255}
```
                                          location                           count
                                              11,7                      272.000000
                                              12,5                      416.000000
                                              13,5                    12268.000000
                                              14,5                      224.000000
                                              12,6                     6838.000000
                                              13,6                    16913.000000
                                              12,7                     5460.000000
                                              13,7                     7609.000000
```
if we want the global coord of the grid we generated we can use the hint `'img11'`:
```
http://localhost:29512/format('text');q(crimes.b('location',dive(p(2,1,2),8),'img11'))
```
We now get coordinates `x` and `y` both in {0,1,...,2047}. Note that the `y`
coordinate of the cells grow bottom-up, Chicago is above the Equator and
{1285,1286,1287} are in the upper half of 2048 or 2^11.

```
                                          location                           count
                                          523,1287                      272.000000
                                          524,1285                      416.000000
                                          525,1285                    12268.000000
                                          526,1285                      224.000000
                                          524,1286                     6838.000000
                                          525,1286                    16913.000000
                                          524,1287                     5460.000000
                                          525,1287                     7609.000000
```

## type

If we want to count the number of crimes by `type` of crime, we run the query
below and retrieve the categories either by number (category id if you will) or
by category name.
```url
# by number
http://localhost:51234/q(crimes.b('type',dive(1)))
```
yields the following json
```json
[
	{
		"type":"table",
		"numrows":31,
		"index_columns":[
			{
				"name":"type",
				"hint":"none",
				"values_per_row":1,
				"values":[
					0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30
				]
			}
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					5742.000000,4660.000000,11367.000000,8990.000000,2933.000000,2226.000000,456.000000,2190.000000,2132.000000,2629.000000,3278.000000,1429.000000,489.000000,46.000000,441.000000,229.000000,181.000000,69.000000,69.000000,20.000000,119.000000,211.000000,63.000000,21.000000,1.000000,2.000000,2.000000,1.000000,2.000000,1.000000,1.000000
				]
			}
		]
	}
]
```
using the hint `'name'` we get path aliases (*ie* the category names)
```url
# by name
http://localhost:51234/q(crimes.b('type',dive(1)),'name')
```
results in
```json
[
	{
		"type":"table",
		"numrows":31,
		"index_columns":[
			{
				"name":"type",
				"hint":"name",
				"values_per_row":1,
				"values":[
					"NARCOTICS","CRIMINAL DAMAGE","THEFT","BATTERY","BURGLARY","MOTOR VEHICLE THEFT","OFFENSE INVOLVING CHILDREN","DECEPTIVE PRACTICE","ROBBERY","ASSAULT","OTHER OFFENSE","CRIMINAL TRESPASS","WEAPONS VIOLATION","KIDNAPPING","PUBLIC PEACE VIOLATION","INTERFERENCE WITH PUBLIC OFFICER","CRIM SEXUAL ASSAULT","LIQUOR LAW VIOLATION","HOMICIDE","STALKING","SEX OFFENSE","PROSTITUTION","ARSON","INTIMIDATION","NON-CRIMINAL","PUBLIC INDECENCY","GAMBLING","OBSCENITY","OTHER NARCOTIC VIOLATION","CONCEALED CARRY LICENSE VIOLATION","NON-CRIMINAL (SUBJECT SPECIFIED)"
				]
			}
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					5742.000000,4660.000000,11367.000000,8990.000000,2933.000000,2226.000000,456.000000,2190.000000,2132.000000,2629.000000,3278.000000,1429.000000,489.000000,46.000000,441.000000,229.000000,181.000000,69.000000,69.000000,20.000000,119.000000,211.000000,63.000000,21.000000,1.000000,2.000000,2.000000,1.000000,2.000000,1.000000,1.000000
				]
			}
		]
	}
]
```
In text form:
```url
# numerical
http://localhost:51234/format('text');q(crimes.b('type',dive(1)),'name')
```
results in
```txt
                                              type                           count
                                         NARCOTICS                     5742.000000
                                   CRIMINAL DAMAGE                     4660.000000
                                             THEFT                    11367.000000
                                           BATTERY                     8990.000000
                                          BURGLARY                     2933.000000
                               MOTOR VEHICLE THEFT                     2226.000000
                        OFFENSE INVOLVING CHILDREN                      456.000000
                                DECEPTIVE PRACTICE                     2190.000000
                                           ROBBERY                     2132.000000
                                           ASSAULT                     2629.000000
                                     OTHER OFFENSE                     3278.000000
                                 CRIMINAL TRESPASS                     1429.000000
                                 WEAPONS VIOLATION                      489.000000
                                        KIDNAPPING                       46.000000
                            PUBLIC PEACE VIOLATION                      441.000000
                  INTERFERENCE WITH PUBLIC OFFICER                      229.000000
                               CRIM SEXUAL ASSAULT                      181.000000
                              LIQUOR LAW VIOLATION                       69.000000
                                          HOMICIDE                       69.000000
                                          STALKING                       20.000000
                                       SEX OFFENSE                      119.000000
                                      PROSTITUTION                      211.000000
                                             ARSON                       63.000000
                                      INTIMIDATION                       21.000000
                                      NON-CRIMINAL                        1.000000
                                  PUBLIC INDECENCY                        2.000000
                                          GAMBLING                        2.000000
                                         OBSCENITY                        1.000000
                          OTHER NARCOTIC VIOLATION                        2.000000
                 CONCEALED CARRY LICENSE VIOLATION                        1.000000
                  NON-CRIMINAL (SUBJECT SPECIFIED)                        1.000000
```
If we want to simply get the number of crimes of a particular type, let's say
`THEFT` we can can request it either by the theft corresponding number (a path
of length 1 described in the [schema](README.md#schema) or by name
```url
#
# by number, since by the schema query above THEFT is an alias for the number 2
# (note that there can be aliases to any path in a hierarchy, in case of
# categorical dimension which are trees of height 2, a path is simply a number).
#
#        ...
#        "23":"INTIMIDATION",
#        "2":"THEFT",
#        "4":"BURGLARY",
#        ...
#
http://localhost:51234/q(crimes.b('type',p(2)))
#
# by name, when we find a string in the TARGET parameter of a binding we assume it
# is an alias and we process as if it was the path of that alias as in the query
# above.
#
http://localhost:51234/q(crimes.b('type','THEFT'))
```
both queries result in
```json
[
	{
		"type":"table",
		"numrows":1,
		"index_columns":[
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					11367.000000
				]
			}
		]
	}
]
```
In case we want to aggregate multiple types, we use `'pathagg'`
```url
#
# Usage pathagg: pathagg(PATH (,PATH)*)
#                pathagg(ALIAS(,ALIAS)*)
#
# by number, let's say we want THEFT and BURGLARY added up in a single value
#
#        ...
#        "23":"INTIMIDATION",
#        "2":"THEFT",
#        "4":"BURGLARY",
#        ...
#
http://localhost:51234/q(crimes.b('type',pathagg(p(2),p(4))))
#
# by name
#
http://localhost:51234/q(crimes.b('type',pathagg('THEFT','BURGLARY')))
```
both result in adding up what we saw in the drilldown query of crime `type`:
```
   THEFT                    11367.000000
BURGLARY                     2933.000000
----------------------------------------
   TOTAL                    14300.000000
```
Here is the `.json` result
```json
[
	{
		"type":"table",
		"numrows":1,
		"index_columns":[
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					14300.000000
				]
			}
		]
	}
]
```

## time

Assuming a dimension of a nanocube is a binary tree, we can get aggregate values
for a sequence of fixed width (in the finest resolution of the tree) using the
`'intseq'` *target*.

```
#
# Usage intseq: intseq(OFFSET,WIDTH,COUNT[,STRIDE])
#
# In the example of crime50k the 'time' dimension was specified in
# the .map file as
#
# index_dimension('time',                         # dimension name
#                 input('Date'),                  # .csv column wher the input date of the record is specified
#                 time(16,                        # binary tree with 16 levels (slots 0 to 65535)
#                    '2013-12-01T00:00:00-06:00', # offset 0 corresponds to 0 hour of Dec 1, 2013 at CST (Central Standard Time)
#                    3600,                        # finest time bin corresponds to 1 hour (or 3600 secs)
#                    6*60                         # add 360 minutes or 6 hoursto to all input values since input comes as
#                                                 # if it is UTC while it is actually CST Central Standard Time
#                   ));
#
# So the time bin semantic is the following: there are 16 levels in the binary
# tree which yields a correspondence between its leaves and the numbers {0,1,...,65535}.
# By the `index_dimension` spec above, each of these numbers correspond to one hour
# aggregate of date (ie. 3600 secs). Tying back these (fine time resolution)
# numbers to the calendar, we have the following correspondence:
#
#     time interval                                             finest bin
#                                                               or leaf number
#     [2013-12-01T00:00:00-06:00,2013-12-01T00:00:00-06:00)     0
#     [2013-12-01T00:00:01-06:00,2013-12-01T01:00:00-06:00)     1
#     [2013-12-01T00:00:02-06:00,2013-12-01T02:00:00-06:00)     2
#     [2013-12-01T00:00:03-06:00,2013-12-01T03:00:00-06:00)     3
#     ...
#     [2013-12-21T00:00:03-06:00,2013-12-01T00:00:00-06:00)     480 (is 20 days later)
#     ...
#
# if we want to query 10 days of daily aggregates starting at the 20th day from
# the 0 time bin, we would like to aggregate
#
#     [480 + 00 * 24, 480 + 01 * 24)
#     [480 + 01 * 24, 480 + 02 * 24)
#     ...
#     [480 + 09 * 24, 480 + 10 * 24)
#
# we achieve that by setting
#
#     OFFSET = 480
#     WIDTH  = 24
#     COUNT  = 10
#     STRIDE = 24  (we can ommit STRIDE when it is the same as the WIDTH)
#
# so the query we wan here can be one of the two below
#
http://localhost:51234/q(crimes.b('time',intseq(480,24,10,24)))
http://localhost:51234/q(crimes.b('time',intseq(480,24,10)))
```
The result of the `'intseq'` query above is
```json
[
	{
		"type":"table",
		"numrows":10,
		"index_columns":[
			{
				"name":"time",
				"hint":"none",
				"values_per_row":1,
				"values":[
					0,1,2,3,4,5,6,7,8,9
				]
			}
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					762.000000,724.000000,660.000000,515.000000,410.000000,584.000000,713.000000,712.000000,704.000000,617.000000
				]
			}
		]
	}
]
```

While we can get to query calendar time series using `'intseq'`, there is a
more convenient way to do it using `'timeseries'`:
```
#
# Usage timeseries: timeseries(BASE_DATE,WIDTH_IN_SECONDS,COUNT[,STRIDE_IN_SECONDS])
#
http://localhost:51234/q(crimes.b('time',timeseries('2013-12-21T00:00-06',24*3600,10,24*3600)))
http://localhost:51234/q(crimes.b('time',timeseries('2013-12-21T00:00-06',24*3600,10)))
```
The result is the same as the previous `'intseq'` version:
```json
[
	{
		"type":"table",
		"numrows":10,
		"index_columns":[
			{
				"name":"time",
				"hint":"none",
				"values_per_row":1,
				"values":[
					0,1,2,3,4,5,6,7,8,9
				]
			}
		],
		"measure_columns":[
			{
				"name":"count",
				"values":[
					762.000000,724.000000,660.000000,515.000000,410.000000,584.000000,713.000000,712.000000,704.000000,617.000000
				]
			}
		]
	}
]
```

# Polygons

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
```
