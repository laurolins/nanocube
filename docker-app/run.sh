#!/bin/sh
set -x
export NAME=$1
export NCDIR=$2
export WEBCONFIG=$3
export DBFILE=$4

echo NAME=$NAME
echo NCDIR=$NCDIR
echo WEBCONFIG=$WEBCONFIG
echo DBFILE=$DBFILE

#link custom web config to /www
ln -s $WEBCONFIG  /www/config.json

#start webserver
nginx -c /app/nginx.conf

#start db rest server
cd /nanocube/bin
gunicorn -D -w 4 -k uvicorn.workers.UvicornWorker -b 127.0.0.1:54322 \
         db_rest_server:app

#start nanocubes server with all the nanocube files
/nanocube/bin/nanocube serve 54321 $NAME=`ls -1 $NCDIR/*.nanocube|paste -sd ':'`
