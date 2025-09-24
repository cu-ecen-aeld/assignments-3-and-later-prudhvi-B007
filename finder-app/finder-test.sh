#!/bin/sh
# finder-test.sh
# This script tests the writer and finder functionality for AESD assignments.
# It creates test files, runs finder.sh, and saves results in /tmp.

set -u  # Exit immediately if a variable is unset

# Number of files to create
NUMFILES=10
# String to write into each file
WRITESTR=AELD_IS_FUN
# Directory where test files will be created
WRITEDIR=/tmp/aeld-data

# Username is read from the configuration file installed in /etc
username=$(cat /etc/finder-app/conf/username.txt)

# Sanity check for arguments (not really used, but left in for safety)
if [ $# -lt 3 ]
then
    echo "Usage: $0 numfiles writestr directory"
    exit 1
fi

# Announce what we’re doing
echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

# Clean up any old test data
rm -rf "${WRITEDIR}"

# Determine which assignment we are on (assignment.txt also comes from /etc)
assignment=$(cat /etc/finder-app/conf/assignment.txt)

# If not assignment1, we must explicitly create the directory
if [ $assignment != 'assignment1' ]
then
    mkdir -p "${WRITEDIR}"
fi

# Loop to create NUMFILES text files using the writer program
for i in $( seq 1 $NUMFILES)
do
    # writer is installed in /usr/bin, so no "./" prefix is needed
    writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

# Run finder.sh to search the directory for the string
# finder.sh is also installed in /usr/bin
OUTPUTSTRING=$(finder.sh "$WRITEDIR" "$WRITESTR")

# Save the results to a temporary file
echo "$OUTPUTSTRING" > /tmp/assignment4-result.txt


# Remove the test data directory
rm -rf /tmp/aeld-data

# Check that the result file exists and exit accordingly
if [ -f /tmp/assignment4-result.txt ]
then
    echo "Success"
    exit 0
else
    exit 1
fi
