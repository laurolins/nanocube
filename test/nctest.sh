#!/bin/bash

# Test for wget (exit on error)
which wget >> /dev/null
if [ "$?" = "1" ]; then
	echo "********************"
	echo "The nctest script requires wget, but it was not found on your system."
	echo "Please install it or update your PATH environment variable to include wget."
	echo "********************"
	exit
fi

# Test for sed (exit on error)
which sed >> /dev/null
if [ "$?" = "1" ]; then
	echo "********************"
	echo "The nctest script requires sed, but it was not found on your system."
	echo "Please install it or update your PATH environment variable to include sed."
	echo "********************"
	exit
fi


# Now try to access the nanocube using wget (exit on error)
wget -q -O - 'http://localhost:29512/count' >> /dev/null
tmp=$?
if [ $tmp -ne 0 ]; then
	echo "********************"
	echo "Error querying nanocube server localhost:29512."
	echo "Please verify that the server is running on localhost on port 29512"
	echo "********************"
	exit
fi


# Remove any old output files.  Create empty out.txt
/bin/rm -f out.txt
touch out.txt


# Start calling the test queries
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


# Mangle the file a bit to make comparisons easier.

# Replace space and left brace with newline and left brace
sed -i $'s/ {/\\\n{/g' out.txt

# Now sort the file so that we can more easily compare.  The values should be identical across all OSes.
# Yes, we know this is not a perfect solution, but good enough to determine if your nanocube is setup correctly and able to answer queries.

sort out.txt > out_sorted.txt

diff out_sorted.txt nctest_output_expected_sorted.txt
tmp=$?
if [ $tmp -ne 0 ]; then
echo "********************"
echo "FAILURE: Test output does not match expected results.  Please do a manual comparison to see what might be going wrong."
echo "********************"
else
echo "SUCCESS"
/bin/rm -f out.txt out_sorted.txt
fi
