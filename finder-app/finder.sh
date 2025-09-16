#!/bin/sh
set -e

if [ $# -ne 2 ]; then
  echo "Error: two arguments required: <filesdir> <searchstr>" >&2
  exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d "$filesdir" ]; then
  echo "Error: $filesdir is not a directory" >&2
  exit 1
fi

# Count files
file_count=$(find "$filesdir" -type f | wc -l)

# BusyBox grep supports -r but not --exclude-dir
match_count=$(grep -r "$searchstr" "$filesdir" | wc -l || true)

echo "The number of files are $file_count and the number of matching lines are $match_count"

