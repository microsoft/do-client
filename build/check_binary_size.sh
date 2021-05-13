#!/bin/bash

# <script.sh> <binary_size> <expected_size>
if [ $# -lt 2 ]; then
  echo "Usage: <script.sh> <binary_path> <expected_size>"
  exit 1
fi

binary_path=$1
expected_size=$2
actual_size=`stat -c%s $binary_path`
change_pct=`echo "scale=2; ($actual_size - $expected_size) / $expected_size * 100" | bc`
echo Size check, expected = $expected_size, actual = $actual_size, change pct = $change_pct%
change_pct=$( printf "%.0f" $change_pct )
if [ $change_pct -gt 10 ]; then
    echo "Size check FAIL. Adjust expected_size if this increase is expected."
    exit 2
fi
