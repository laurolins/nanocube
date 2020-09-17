#!/bin/sh
set -x

#link custom web config to /www
ln -s /data/config.json  /www

#start webserver
nginx -c /app/nginx.conf

#start nanocubes server with all the nanocube files
/nanocube/bin/nanocube serve 54321 nc=`ls -1 /data/*.nanocube|paste -sd ':'`
