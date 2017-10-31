#!/bin/bash
export NANOCUBE_SRC=$PWD/$(dirname "${BASH_SOURCE[0]}")
export NANOCUBE_BIN=$NANOCUBE_SRC/bin

# make the official NC binaries the priority on the paths
export PATH=$NANOCUBE_BIN:$PATH

MYHOST=$(hostname -A 2>/dev/null)
if [ $? -ne 0 ]; then
   MYHOST=$(hostname -f)
fi

MYHOST=$(echo $MYHOST| cut -d ' ' -f 1)
export MYHOST
