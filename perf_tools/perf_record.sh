#!/usr/bin/env bash

# Record script

PFLAGS="
        -a
        --freq=max
        5"


make clean
make benchmark

echo "-----------------------------------"
echo "[script] perf record flags: $PFLAGS"
echo "-----------------------------------"

perf record ./benchmark $PFLAGS
