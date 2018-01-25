#!/bin/bash
CMD=$1

if [ $CMD == "create" ]; then
	nanocube create chicago-crimes-10k-sample.csv chicago-crimes.map chicago-crimes-10k-sample.nanocube -report-frequency=1000 -report-cache=0
elif [ $CMD == "create-and-serve" ]; then
	# refresh every 1 second (stop insertion, copy updated index, swap serving index, delete outdated index, resume)
	# note we need at least 3x the max memory of a cube being created to handle the create and serve
	nanocube create chicago-crimes-10k-sample.csv chicago-crimes.map chicago-crimes-10k-sample.nanocube -report-frequency=1000 -report-cache=0 -serve-port=12345 -serve-threads=4 -serve-refresh-rate=1
elif [ $CMD == "serve" ]; then
	# assumes the index above was already created
	nanocube serve 12345 x=chicago-crimes-10k-sample.nanocube -threads=4
elif [ $CMD == "serve-debug" ]; then
	# assumes the index above was already created
	gdb --args nanocube serve 11111 x=chicago-crimes-10k-sample.nanocube -threads=4
fi
