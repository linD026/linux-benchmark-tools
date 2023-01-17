#!/usr/bin/env bash

function print()
{
    echo "[script] $1"
}

TRACE_PATH="/sys/kernel/tracing"
TRACE_FUNCTION=""

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
function trace_on()
{
    get_function $1
    TRACE_FUNCTION="$TRACE_FUNCTION --trace-funcs=$1 -graph-funcs=$1"
    print "Add $1 to filter"
}

trace_on "sched_fork"

#trace_on "perf_event_init_task"

trace_on "audit_alloc"

# Only Initialize linked list
#trace_on "shm_init_task"

trace_on "security_task_alloc"

trace_on "copy_semundo"

#trace_on "copy_files"
trace_on "dup_fd"

#trace_on "copy_fs"
trace_on "copy_fs_struct"

trace_on "copy_sighand"

trace_on "copy_signal"

#trace_on "copy_mm"
trace_on "dup_mm"

trace_on "copy_namespaces"

#trace_on "__copy_io"

trace_on "copy_thread"

trace_on "copy_process"

PFLAGS="
        --tracer=function_graph"

perf ftrace $TRACE_FUNCTION $PFLAGS $1
