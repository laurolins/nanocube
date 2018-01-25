#!/bin/bash
CMD=$1
if [ $CMD == "api" ]; then
	prefix="http://localhost:51234"
	wget -q -O q_schema.json "${prefix}/schema()"
	wget -q -O q_totals.json "${prefix}/q(crimes)"
fi
