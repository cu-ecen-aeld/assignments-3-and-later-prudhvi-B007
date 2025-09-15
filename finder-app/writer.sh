#!/usr/bin/env sh
set -euo pipefail

if [ $# -ne 2 ]; then
  echo "Error: two arguments required: <writefile> <writestr>" >&2
  exit 1
fi

writefile=$1
writestr=$2

# Create the parent directory path if needed
dirpath=$(dirname "$writefile")
mkdir -p "$dirpath" || { echo "Error: could not create directory $dirpath" >&2; exit 1; }

# Overwrite file with the given string
echo "$writestr" > "$writefile" || { echo "Error: could not write to $writefile" >&2; exit 1; }
