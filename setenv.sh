#!/bin/bash
export NANOCUBE_SRC=$PWD/$(dirname "${BASH_SOURCE[0]}")
export NANOCUBE_BIN=$NANOCUBE_SRC/bin

# make the official NC binaries the priority on the paths
export PATH=$NANOCUBE_BIN:$PATH

