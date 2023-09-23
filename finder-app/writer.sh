#!/bin/sh
# writer script as per assignment 1
# by Martin Mauersberg

if [ $# -lt 2 ]
then
    echo "this command requires two parameters (file and string)"
    exit 1
fi

WRITEFILE=${1}
WRITESTR=${2}

mkdir -p "$(dirname "$WRITEFILE")"
echo ${WRITESTR} > ${WRITEFILE}

if [ ! -f ${WRITEFILE} ]
then
    echo "file could not be created"
    exit 1
fi
