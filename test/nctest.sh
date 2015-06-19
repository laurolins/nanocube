#!/bin/bash

# Determine what OS you are running (Linux, MacOS are only ones supported)
uname -a | grep -i linux >> /dev/null
tmp=$?
if [ $tmp -ne 0 ]; then
uname -a | grep -i darwin >> /dev/null
tmp2=$?
if [ $tmp2 -ne 0 ]; then
os=unknown
else
os=mac
fi
else
os=linux
fi

# Test for wget (exit on error)
which wget >> /dev/null
if [ "$?" = "1" ]; then
	echo "********************"
	echo "The nctest script requires wget, but it was not found on your system."
	echo "Please install it or update your PATH environment variable to include wget."
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


# Compare the output (out.txt) against the appropriate results file.

if [ $os == "linux" ]; then

diff out.txt nctest_output_expected.ubuntu.14.04.txt
tmp=$?
if [ $tmp -ne 0 ]; then
echo "********************"
echo "FAILURE: Test output does not match expected results for Linux.  Something is likely wrong with the setup."
echo "********************"
else
echo "SUCCESS"
/bin/rm -f out.txt
fi

elif [ $os == "mac" ]; then
diff out.txt nctest_output_expected.macos10.10.txt

tmp=$?
if [ $tmp -ne 0 ]; then
echo "********************"
echo "FAILURE: Test output does not match expected results for MacOS.  Something is likely wrong with the setup."
echo "********************"
else
echo "SUCCESS"
/bin/rm -f out.txt
fi

else
echo "********************"
echo "Your operating system does not appear to be Linux or MacOS.  You can try comparing out.txt to one"
echo "of the two output files included, but it is likely that they will differ in some way.  You may be"
echo "tell if there are only minor differences, which could indicate that your setup is correct. If you"
echo "are seeing unexplained differences, please try posting your results to the nanocubes discussion forum."
echo "********************"
fi
