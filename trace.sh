#!/usr/bin/env bash

# sudo bash trace.sh ./program

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
function tracing_on()
{
    get_function $1
    TRACE_FUNCTION="$TRACE_FUNCTION --trace-funcs=$1 -graph-funcs=$1"
    print "Add $1 to filter"
}

#tracing_on "sched_fork"
#
##tracing_on "perf_event_init_task"
#
#tracing_on "audit_alloc"
#
## Only Initialize linked list
##tracing_on "shm_init_task"
#
#tracing_on "security_task_alloc"
#
#tracing_on "copy_semundo"
#
##tracing_on "copy_files"
#tracing_on "dup_fd"
#
##tracing_on "copy_fs"
#tracing_on "copy_fs_struct"
#
##tracing_on "copy_sighand"
#
##tracing_on "copy_signal"
#
##tracing_on "copy_mm"
#tracing_on "dup_mm"
#tracing_on "dup_mmap"
#tracing_on "uprobe_dup_mmap"
#tracing_on "vma_dup_policy"
#tracing_on "dup_userfaultfd"
#tracing_on "vm_area_dup"
#tracing_on "copy_page_range"
#tracing_on "__ksm_enter" # ksm_fork
#tracing_on "__khugepaged_enter" # khugepaged_fork
#
#
#tracing_on "copy_namespaces"
#
##tracing_on "__copy_io"
#
#tracing_on "copy_thread"
#
#tracing_on "copy_process"
#
#tracing_on "alloc_pid"
#tracing_on "cgroup_can_fork"
#tracing_on "sched_cgroup_fork"
#tracing_on "klp_copy_process"
#tracing_on "sched_core_fork"
#tracing_on "proc_fork_connector"
#tracing_on "sched_post_fork"
#tracing_on "cgroup_post_fork"
#tracing_on "uprobe_copy_process"
#tracing_on "cgroup_fork"
#tracing_on "dup_task_struct"
#tracing_on "copy_creds"
#tracing_on "__mpol_dup"
#
#tracing_on "_raw_spin_lock"
#tracing_on "_raw_spin_lock_irq"

PFLAGS="
        --tracer=function_graph"

# For single thread right now (see the man page).
#perf ftrace $TRACE_FUNCTION $PFLAGS $1

function ftrace_fs()
{
    echo 0 > $TRACE_PATH/tracing_on
    echo "function_graph" > $TRACE_PATH/current_tracer
    echo $TRACE_FUNCTION > $TRACE_PATH/set_ftrace_filter
    echo $$ > $TRACE_PATH/set_ftrace_pid

    echo 1 > $TRACE_PATH/tracing_on
    $1
    echo 0 > $TRACE_PATH/tracing_on
    echo > $TRACE_PATH/set_ftrace_pid
    cat $TRACE_PATH/trace > ftrace.log
}

ftrace_fs
