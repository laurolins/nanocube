#!/bin/bash

cd web
python -m SimpleHTTPServer &

cd ../src
./ncserve < ../scripts/crime1.nano &
./ncserve < ../scripts/crime2.nano &
./ncserve < ../scripts/crime3.nano &
