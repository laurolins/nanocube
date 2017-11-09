#!/bin/bash
for i in $(seq 1 4); do
	nanocube csv -header=header bug4e.csv m1.map bug.nanocube -filter=0,$i;
	nanocube draw -show-ids bug.nanocube bug.dot;
	dot -Tpdf -obug$i.pdf bug.dot;
done
# make sure we preserve the log
nanocube csv -header=header bug4e.csv m1.map bug.nanocube

if [ $1 ]; then
	OS=$(uname)
	if [ ${OS} = "Darwin" ]; then
		lldb nanocube -- csv -header=header bug4e.csv m1.map out1.nanocube;
	else
		gdb --args nanocube csv -header=header bug4e.csv m1.map out1.nanocube;
	fi
fi
