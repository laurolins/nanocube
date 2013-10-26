#!/usr/bin/env python
import sys
import os
import tempfile
import dateutil.parser
import struct
import time

def readline():
    l = sys.stdin.readline()
    if l == '': return None
    return [i.strip() for i in l.strip().split(',')]

def read_all_lines():
    while True:
        l = readline()
        if l is None:
            break
        yield l

class ColumnDict:
    def __init__(self, header):
        self.header = header
        self.columns = dict([(i, {}) for i in range(len(header))])
    def register(self, column, value):
        d = self.columns[column]
        d.setdefault(value, len(d))
        if len(d) > 255:
            sys.stderr.write("Column %s has too many distinct elements (max 256)\n" % self.header[i])
            exit(1)
    def convert(self, line):
        lat = float(line[0])
        lon = float(line[1])
        dt = dateutil.parser.parse(line[2])
        unix_time = time.mktime(dt.timetuple())
        fmt = 'ffq' + ('b' * (len(self.header) - 3))
        content = [self.columns[i+3][v] for (i,v) in enumerate(line[3:])]
        content = tuple([lat, lon, unix_time] + content)
        return struct.pack(fmt, *content)
        
def check_header(header):
    if len(header) < 3:
        sys.stderr.write("Need at least latitude, longitude and time\n")
        exit(1)
    if len(header) > 8:
        sys.stderr.write("This version supports at most 5 categorical columns\n")
        exit(1)
    if header[0] <> 'latitude':
        sys.stderr.write("First column must be latitude, in degrees\n")
        exit(1)
    if header[1] <> 'longitude':
        sys.stderr.write("Second column must be longitude, in degrees\n")
        exit(1)
    if header[2] <> 'datetime':
        sys.stderr.write("Third column must be datetime, in ISO 8601\n")
        exit(1)

################################################################################


fd, name = tempfile.mkstemp()
os.close(fd)
contents = file(name, 'w')

header = readline()
check_header(header)

c = ColumnDict(header)

for l in read_all_lines():
    for i in xrange(3, len(header)):
        c.register(i, l[i])
    contents.write(c.convert(l))
contents.close()

print "name: export"
print "field: latitude, float"
print "field: longitude, float"
print "field: time, uint64"
for i in header[3:]:
    print "field: %s, uint8" % i
for i in xrange(3, len(header)):
    d = sorted(c.columns[i].items(), key=lambda (v,k): (k, v))
    for (v, k) in d:
        print "valname: %s, %d, %s" % (header[i], k+1, v)
print
content = file(name, 'r')

chunk = content.read(1024)
while len(chunk) > 0:
    sys.stdout.write(chunk)
    chunk = content.read(1024)
content.close()
os.unlink(name)
