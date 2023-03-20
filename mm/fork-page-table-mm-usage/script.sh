#!/usr/bin/env bash

echo -n "[exec] [PID $$] VmPTE:"
awk '/VmPTE:/{ sum = $2 } END { printf "%u", sum }' /proc/$$/status
echo " kB"
