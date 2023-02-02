#!/usr/bin/env bash

# sudo bash trace.sh [option:pf] ./program

function print()
{
    echo "[script] $1"
}

while getopts "pf" flag
do
    case "${flag}" in
        p) interface="perf";;
        f) interface="fs";;
    esac
done

print "The program you want to trace is: $2"
if [ "$interface" = "" ]; then
     print "sudo bash trace.sh [option:pf] ./program"
     exit 0
fi
print "Set $interface as interface"

TRACE_PATH="/sys/kernel/tracing"
TRACE_FUNCTION=""

# cleanup
echo 0 > $TRACE_PATH/tracing_on
echo > $TRACE_PATH/set_ftrace_filter
echo > $TRACE_PATH/trace

# name
function get_function()
{
    local file="$TRACE_PATH/available_filter_functions"
    cat $file | grep -x "$1" >> /dev/null
    if [ $? -ne 0 ]; then
        print "$1 not found in $file"
        exit 0
    fi
}

# name_of_function
function tracing_on()
{
    get_function $1
    if [ "$interface" = "perf" ]; then
        TRACE_FUNCTION="$TRACE_FUNCTION --trace-funcs=$1 -graph-funcs=$1"
    elif [ "$interface" = "fs" ]; then
        echo $1 >> $TRACE_PATH/set_ftrace_filter
    fi
    print "Add $1 to filter"
}


IFS='
'

TRACING_FUNCTION_LIST=`cat tracing_function_list`
for item in $TRACING_FUNCTION_LIST; do
    if [[ "$item" != "#"* ]]; then
        tracing_on "$item"
    fi
done

function ftrace_perf()
{
    PFLAGS="
            --tracer=function_graph"

    # For single thread right now (see the man page).
    perf ftrace $TRACE_FUNCTION $PFLAGS $1
}

function ftrace_fs()
{
    echo "function_graph" > $TRACE_PATH/current_tracer
    print "set function_graph"

    print "set_ftrace_filter has: {"
    cat $TRACE_PATH/set_ftrace_filter
    print "}"

    echo function-fork > $TRACE_PATH/trace_options
    print "set function-fork"

    echo $$ > $TRACE_PATH/set_ftrace_pid
    print "set pid $$"

    echo 1 > $TRACE_PATH/tracing_on
    $2
    echo 0 > $TRACE_PATH/tracing_on

    echo > $TRACE_PATH/set_ftrace_pid
    cat $TRACE_PATH/trace > ftrace.log
}

if [ "$interface" = "perf" ]; then
    ftrace_perf
elif [ "$interface" = "fs" ]; then
    ftrace_fs
fi
