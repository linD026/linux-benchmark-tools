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
trace_on "dup_mmap"
trace_on "uprobe_dup_mmap"
trace_on "vma_dup_policy"
trace_on "dup_userfaultfd"
trace_on "vm_area_dup"
trace_on "copy_page_range"
trace_on "__ksm_enter" # ksm_fork
trace_on "__khugepaged_enter" # khugepaged_fork


trace_on "copy_namespaces"

#trace_on "__copy_io"

trace_on "copy_thread"

trace_on "copy_process"

trace_on "alloc_pid"
trace_on "cgroup_can_fork"
trace_on "sched_cgroup_fork"
trace_on "klp_copy_process"
trace_on "sched_core_fork"
trace_on "proc_fork_connector"
trace_on "sched_post_fork"
trace_on "cgroup_post_fork"
trace_on "uprobe_copy_process"
trace_on "cgroup_fork"
trace_on "dup_task_struct"
trace_on "copy_creds"
trace_on "__mpol_dup"

trace_on "_raw_spin_lock"
trace_on "_raw_spin_lock_irq"

PFLAGS="
        --tracer=function_graph"

perf ftrace $TRACE_FUNCTION $PFLAGS $1
