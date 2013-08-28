#!/usr/bin/python
dimensions       = [1,2,3,4,5,6]
#levels           = [1,2,4,8,16,32]
levels           = [1,6,11,16,18,21,26]
for d in dimensions:
    for l in levels:
        extra_dimensions   = d-1
        max_quadtree_level = l-1 # starts from 0
        print \
'''   else if (num_extra_fields==%d && max_quadtree_level==%d) {
      index = buildIndex<%d,%d>(specialized_schema, server, max_points, time_bin_function, report_frequency, regions);
   }''' % (extra_dimensions, max_quadtree_level, extra_dimensions, max_quadtree_level)
