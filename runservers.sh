#!/bin/bash

cd web
python -m SimpleHTTPServer 12345 &

cd ../src
ncserve --port=29512 < ../scripts/crime1.nano &
ncserve --port=29513 < ../scripts/crime2.nano &
ncserve --port=29514 < ../scripts/crime3.nano &
