#!/usr/bin/env bash

# See:
# - https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html
# - https://man7.org/linux/man-pages/man2/perf_event_open.2.html
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid

echo 0 | sudo tee /proc/sys/kernel/kptr_restrict
