#!env python
import json
import sys
import time
import struct

# print output stream

schema = '''name: sliding-dmp-example
encoding: binary
metadata: src__origin degrees_mercator_quadtree25
metadata: tbin 2014-01-01_00:00:00_3600s
field: src nc_dim_quadtree_25
field: kind nc_dim_cat_1
field: time nc_dim_time_2
field: count nc_var_uint_4

'''

sys.stdout.write(schema)
for i in xrange(2000):
    # print latitude, longitude, checkin_time, checkin_device
    # little endian: 4 bytes float, 4 bytes float, 8 bytes unix time, 1 byte device
    data = struct.pack("<IIBHI", 0x44332211, 0x88776655, 3, i, 1)
    sys.stdout.write(data)
