# RoadSnap

## Build *RoadSnap* database based on Open Street Maps `.osm` files [DONE]

```
nanocube roadmap <output-roadsnap-file> (<input-osm-file>)+ [-size0=8g] [-min-segment-in-meters=10]
```

# query through http

```
http://nano4:4444/roadsnap.k(1).r(100.0).loc(38,-78);
```

# roadmap project (start time: 2016-10-23T13:14)

Read raw .osm files found in [geofabrik.de](http://download.geofabrik.de/north-america.html).
and store the road graph in a convenient way. The first goal is to make all nodes
of the `ways` as snapping points to reduce the size of the taxi dataset.

# Raw `.xml` data from `geofabrik.de` has bugs

There is an escaped double qutoes symbol in some states that messes
up the osm data on some states: florida, california, colorado, west-virginia, virgina

# create an easy to use API accessible through HTTP 2017-02-15T16:48:56

1. enable creation from multiple sources and add a source string to it.

```
nanocube roadmap output-file (<input-file> <source-text>)+ [-fill-segments=<meters>] [-types=motorway,trunk,residential,tertiary,service,
```

# Typed of highway on OSM file 2017-02-15T21:42

```
highway    motorway
highway    trunk
highway    primary
highway    secondary
highway    tertiary
highway    unclassified
highway    residential
highway    service
highway    motorway_link
highway    trunk_link
highway    primary_link
highway    secondary_link
highway    tertiary_link
highway    living_street
highway    pedestrian
highway    track
highway    bus_guideway
highway    escape
highway    raceway
highway    road
highway    footway
highway    bridleway
highway    steps
highway    path
```

## Finished a good-enough .xml scanner 2016-10-23T17:51

## Finished a .roadmap format 2016-10-26T16:01


## Closest points 2016-10-26T20:34

```
# this is near Church St. and Thomas St.
roadmap closest ny.roadmap 10 40.716507 -74.006607

# this is near Church St. and Thomas St.
roadmap snap ny.roadmap 3 40.716507 -74.006607 0.00001

```

```python
import math
6371000 * (0.001/180)*math.pi
# 111.19492664455873 meters
6371000 * (0.0001/180)*math.pi
# 11.19492664455873 meters
```



## Snap using vantage tree 2016-10-28T13:01


## modules

/* u64 -> al_Ptr_char btree */

module | description
-------|--------------------------------------------------------------------
`rbt_` | roadmap btree (maps a `u64` -> `al_Ptr_char`)
`rp_`  | roadmap parser (parses the .xml from OSM raw files)
`rg_`  | roadmap graph (parses the .xml from OSM raw files)








