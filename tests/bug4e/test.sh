#!/bin/bash
if ! [ $1 ] || ! [ $2 ] || ! [ $3 ] ; then
	echo "Usage: bash test.sh CSV BEGIN END"
	exit
fi

BEGIN=$2
END=$3
DATASET=$1

COUNT=$((${END}-${BEGIN}))

MAP1=m1.map
SET1=out_"${BEGIN}"_"${COUNT}"_m1.nanocube

MAP2=m2.map
SET2=out_"${BEGIN}"_"${COUNT}"_m2.nanocube

FILTER="-filter=${BEGIN},${COUNT}"

nanocube csv -header=header "${DATASET}" "${MAP1}" "${SET1}" "${FILTER}" 2> /dev/null
nanocube csv -header=header "${DATASET}" "${MAP2}" "${SET2}" "${FILTER}" 2> /dev/null
nanocube memory "${SET1}" 2> /tmp/x; grep "cache: PNode" /tmp/x >   /tmp/r;
nanocube memory "${SET2}" 2> /tmp/x; grep "cache: PNode" /tmp/x >>  /tmp/r;
cat /tmp/r

nanocube draw "${SET1}" g1.dot; dot -Tpdf "-og1_${BEGIN}_${COUNT}.pdf" g1.dot
nanocube draw "${SET2}" g2.dot; dot -Tpdf "-og2_${BEGIN}_${COUNT}.pdf" g2.dot

