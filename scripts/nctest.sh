#!/bin/bash

/bin/rm -f out.txt
touch out.txt

wget -q -O - 'http://localhost:29512/count' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.a("location",dive([2,1,2],8))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("location",range2d(tile2d(1049,2571,12),tile2d(1050,2572,12)))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("time",mt_interval_sequence(480,24,10))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.a("location",dive([2,1,2],8)).r("time",mt_interval_sequence(480,24,10))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.a("location",dive(tile2d(1,2,2),8))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.a("location",dive(tile2d(1,2,2),8),"img")' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.a("crime",dive([],1))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("crime",set([1],[3]))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("crime",set(1,3))' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("time",interval(484,500)).a("location",dive(tile2d(262,643,10),1),"img")' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("time",mt_interval_sequence(484,1,5)).a("location",dive(tile2d(262,643,10),1),"img")' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/count.r("location",degrees_mask("-87.6512,41.8637,-87.6512,41.9009,-87.6026,41.9009,-87.6026,41.8637",25))' >> out.txt
echo >> out.txt


diff out.txt nctest_output_expected.txt

tmp=$?
if [ $tmp -ne 0 ]; then
echo "**********"
echo "FAILURE: Test output does not match expected results.  Something is wrong with the setup."
echo "**********"
else
echo "SUCCESS"
/bin/rm -f out.txt
fi
