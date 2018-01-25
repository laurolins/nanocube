#!/bin/bash
mkdir -p .aux

file_queries=".aux/name_query"
cat >$file_queries << END
schema schema()
total  q(crimes)
loc1   q(crimes.b('location',dive(p(2,1,2),8)))
loc1t  format('text');q(crimes.b('location',dive(p(2,1,2),8)))
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

# loc1   q(crimes.b('location',dive(p(2,1,2),8)))
# 
# 
# 
# if [ $CMD == "api" ]; then
# 	prefix="http://localhost:51234"
# 
# 
# 	for 
# 
# 	wget -q -O q_schema.json "${prefix}/schema()"
# 	wget -q -O q_total.json "${prefix}/q(crimes)"
# 	wget -q -O q_loc1.json "${prefix}/q(crimes)"
# 
# fi
