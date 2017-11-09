#!/bin/bash
FULLNAME=$1
FILENAME=$(basename "$FULLNAME")
EXTENSION="${FILENAME##*.}"
BASENAME="${FILENAME%.*}"
TEST="/tmp/${BASENAME}"

OPTIONAL_OUTPUT_DIR=$2
if [ "${OPTIONAL_OUTPUT_DIR}" != "" ]; then
	TEST="${OPTIONAL_OUTPUT_DIR}/${BASENAME}"
fi


FLAGS="
-Wall
-Werror
-Wextra
-Wno-unused-parameter
-Wno-sign-compare
-Wno-strict-aliasing
-Wno-unused-function
-Wno-unused-variable
-Wno-implicit-function-declaration
-std=gnu11
-DOS_LINUX"

DEBUG="-g -ggdb -DCHECK_ASSERTIONS -fno-omit-frame-pointer"
RELEASE="-g -ggdb -O3"
PROFILE="-g -ggdb -O2 -fno-omit-frame-pointer"
OPTIONS="${FLAGS} ${DEBUG}"
CMD="gcc -D${BASENAME}_UNIT_TEST $OPTIONS -o ${TEST} ${FULLNAME}"

echo $CMD
gcc -D${BASENAME}_UNIT_TEST $OPTIONS -o ${TEST} ${FULLNAME}
${TEST}

