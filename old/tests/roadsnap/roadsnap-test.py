#!/usr/bin/python
import time
import urllib2
import random

def Kilobytes(n):
    return(n * 1024)
def Megabytes(n):
    return(Kilobytes(n)*1024)

# prepare large url within budget
time_prepare = time.time()
budget = Megabytes(2)
url = ["roadsnap.k(5).r(100).loc(38,-78"]
length = len(url[0]) + 2 # final two characters );
count = 0
while 1:
    lat = random.uniform(31.1, 46.1)
    lon = random.uniform(-121.6, -71.7)
    loc = ",%.5f,%.5f" % (lat, lon)
    lenloc = len(loc)
    if length + lenloc <= budget:
        url.append(loc)
        length = length + lenloc
        count = count + 1
    else:
        break
url.append(");")
url = "".join(url)
open("/tmp/url.txt","w").write(url)
time_prepare = time.time() - time_prepare
print "time to prepare %.2fs num locates %d" % (time_prepare, count)
# print url

server = "http://nano4:5555/"
time_query = time.time()
f = open("/tmp/result.txt","w") 
f.write(urllib2.urlopen(server + url).read())
f.close()
time_query = time.time() - time_query
print "time to query %.2fs" % time_query

