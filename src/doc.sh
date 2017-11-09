#!/bin/bash
INPUT="$1"
OUTPUT="${INPUT}.doc"
echo "${OUTPUT}"

# get version name based on the VERSION file and the git revision
BASE_VERSION="$(cat VERSION)"
REVISION="$(git rev-list --count HEAD)"
MODIFIED=""
if [[ $(git status --porcelain | grep " M") ]]; then
	  MODIFIED="m"
fi
VERSION_NAME="${BASE_VERSION}r${REVISION}${MODIFIED}"

# MODIFIED=$(git status --porcelain | grep " M" | wc -l)
# __VERSION__

# find all BEGIN_DOC_STRING lines
grep -n "^BEGIN_DOC_STRING" "${INPUT}" | cut -f 2 -d \ > /tmp/n
grep -n "^BEGIN_DOC_STRING" "${INPUT}" | cut -f 1 -d : > /tmp/b
grep -n "^END_DOC_STRING"   "${INPUT}" | cut -f 1 -d : > /tmp/e
paste /tmp/b /tmp/e /tmp/n > /tmp/r

echo "" > "${OUTPUT}"
# read each line as an array
while read -ra LINE; do
	FIRST="$((${LINE[0]}+1))"
	LAST="$((${LINE[1]}-1))"
	NAME="${LINE[2]}"
	echo "static const char ${NAME}[] = {" >> "${OUTPUT}"
	sed -n ${FIRST},${LAST}p ${INPUT} | sed 's/__VERSION__/'${VERSION_NAME}'/g' > /tmp/i
	cat /tmp/i | xxd -i >> ${OUTPUT}
	echo ", 0x00 };" >> "${OUTPUT}"
done < /tmp/r



# cat /tmp/r | awk 'BEGIN {} { OFS=""; print "sed -n \x27",$1+1,",",$2-1,"p\x27" }' | xargs -I{} bash -c "{} ${FILENAME} | xxd -i > /tmp/s"


# generate a file with the lines from b+1 to e-1




