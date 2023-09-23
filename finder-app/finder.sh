#!/bin/sh
# finder script as per assignment 1
# by Martin Mauersberg

if [ $# -lt 2 ]
then
    echo "this command requires two parameters (directory and search string)"
    exit 1
fi

FILESDIR=${1}
SEARCHSTR=${2}

if [ ! -d "${FILESDIR}" ]
then
    echo "directory ${FILESDIR} does not exist"
    exit 1
fi

echo "Path is ${FILESDIR} and file is ${SEARCHSTR}"

FILECOUNT=`find ${FILESDIR} -type f | wc -l`
SEARCHCOUNT=`grep -r "${SEARCHSTR}" ${FILESDIR} | wc -l` 
echo "The number of files are ${FILECOUNT} and the number of matching lines are ${SEARCHCOUNT}"


