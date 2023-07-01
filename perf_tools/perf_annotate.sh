#!/usr/bin/env bash

# Annotate script

#VMLINUX="~/linux/linux/vmlinux"

perf annotate -k $VMLINUX -d symbol
