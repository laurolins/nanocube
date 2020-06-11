#!/bin/bash
#
# raw data
# lat, lon, speed
#
cat <<EOF > raw_data
-20|-90|30
-30|-91|60
EOF

#
# mapping file (from raw to nanocube)
#
cat <<EOF > tmp_map
index_dimension('location',input(1,2),latlon(25));
# samples_count will add +1 for each new record in the input
measure_dimension('samples_count',input(),f64);
# speed_sum coming from column 3  will add +1 for each new record in the input
measure_dimension('speed_sum',input(3),f64);
EOF

# create index
cat raw_data | nanocube create -stdin tmp_map example.nanocube -sep='|'

# serve index
nanocube serve 12345 table=example.nanocube -threads=6 &
pid_server="$!"

# query get all the metrics
wget -O - -q "http://localhost:12345/format('psv');q(table)"
#
# samples_count|speed_sum
# 2.000000|90.000000
#
# from this you can get the average: 45

# kill server
kill $pid_server
