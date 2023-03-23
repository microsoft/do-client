#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if [ $# -lt 2 ]; then
  echo "Usage: <script.sh> <expected_size> <one or more file paths>"
  exit 1
fi

expected_size=$1
shift

for p in "$@"
do
  echo "Processing file: $p"
  if [ ! -f $p ]; then
    echo "File not found"
    exit 2
  fi
  actual_size=`stat -c%s $p`
  change_pct=`echo "scale=2; ($actual_size - $expected_size) / $expected_size * 100" | bc`
  echo Size check, expected = $expected_size, actual = $actual_size, change pct = $change_pct%
  change_pct=$( printf "%.0f" $change_pct )
  if [ $change_pct -ge 2 ]; then
    echo "[FAIL] Size increased beyond threshold. Adjust expected_size if this is expected."
    exit 3
  elif [ $change_pct -le -5 ]; then
    echo "[WARNING] Size reduced beyond threshold. Adjust expected_size if this is expected."
    exit 4
  fi
done
