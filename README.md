# Benchmark tools

## ftrace with google piechart (and jsfiddle.net)

#### Step 1 - Set the function filiter

Add the function you want to trace into `tracing_function_list` file.
For the comment add `#` in front of the line.
For exmaple, Add `sched_fork` function will be like:

```
# tracing_function_list
sched_fork
```

#### Step 2 - ftrace with the program

There are some options you can set:

- `p`:
    Use the perf interface to set the ftrace.
    Right now it only for the single thread program.
    See the manual page for more information.
- `f`:
    Use the file system (`/sys/kernel/tracing`) to set the ftrace.
- `l`:
    The file list all the function you want to trace. The default file is
    'tracing_function_list'.
- `i`:
    The binary file you want to trace/execute.

Use `trace.sh` to trace your program. For example:

```bash
# the ./program should be second argument
sudo bash trace.sh [option:pf] -l function_list -i bin
```

#### Step 3 - Parser the ftrace output

Build the parser.

```bash
make parser
```

There are some options you can set:

- `ftrace_output`:
    The file name that will be scanned by the parser. The default name is
    `ftrace_output`.
- `check_cpu`:
    In some of the case (i.e., the single thread mode) we don't check the
    cpu number since it has the false positive. But for the mutiple thread
    (e.g., ftrace with function-fork option), we should check the cpu number
    since it might be the context switch without notification.

```bash
./parser_ftrace [options]
```

#### Step 4 - PieChart

Then go to the jsfiddle.net (or other available environment) to get the piechart.
The source codes for JS is in `draw_pie_chart.js`.

### More information

- https://developers.google.com/chart/interactive/docs/gallery/piechart
- https://jsfiddle.net/linD026/a1fsxpkj/
