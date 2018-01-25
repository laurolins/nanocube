#!/bin/bash
mkdir -p .aux

file_queries=".aux/name_query"
cat >$file_queries << END
schema          schema()
total           q(crimes)
loc1            q(crimes.b('location',dive(p(2,1,2),8)))
loc1t           format('text');q(crimes.b('location',dive(p(2,1,2),8)))
loc1i           format('text');q(crimes.b('location',dive(p(2,1,2),8),'img8'))
loc1i2          format('text');q(crimes.b('location',dive(p(2,1,2),8),'img11'))
type1           q(crimes.b('type',dive(1)))
type1n          q(crimes.b('type',dive(1),'name'))
type1nt         format('text');q(crimes.b('type',dive(1),'name'))
time1intseq     q(crimes.b('time',intseq(480,24,10,24)))
time1timeseries q(crimes.b('time',timeseries('2013-12-01',24*3600,10,24*3600)))
END

prefix="http://localhost:51234"
while IFS='' read -r line || [[ -n "$line" ]]; do
# 	echo "$line"
	name=$(echo "$line" | tr -s ' ' | cut -f 1 -d' ')
	query=$(echo "$line" | tr -s ' ' | cut -f 2 -d' ')
# 	echo "$name"
# 	echo "$query"
	wget -q -O "${name}.result" "${prefix}/${query}"
done < "$file_queries"

