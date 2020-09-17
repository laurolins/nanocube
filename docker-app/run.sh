#!/bin/sh
set -x
export NAME=$1
export NCDIR=$2
export WEBCONFIG=$3
export DBFILE=$4

#link custom web config to /www
ln -s $WEBCONFIG  /www/config.json

#start webserver
nginx -c /app/nginx.conf

#start db rest server
uvicorn /nanocube/bin/db_rest_server:app --port 54322 &

#start nanocubes server with all the nanocube files
/nanocube/bin/nanocube serve 54321 $NAME=`ls -1 $NCDIR/*.nanocube|paste -sd ':'`
