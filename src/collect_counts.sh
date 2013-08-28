#!/bin/bash
# rm _data.txt
# add header to _data.txt
find . | grep nanocube-count | head -n 1 | xargs head -n 1 
# >> _data.txt
# get lines of the generated files
ls -1 | grep nanocube-count | xargs tail -n 1 | egrep -v "($^|==)" 
#>> _data.txt
