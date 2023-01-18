# Benchmark tools

## ftrace with google piechart (jsfiddle.net)

#### Step 1 - Set the function filiter

In `trace.sh` set `trace_on $function`.

#### Step 2 - Get the ftrace output

Copy the function section you want to analyze to `ftrace_output`.
For example, we trace the benchmark.

```bash
make benchmark
sudo bash trace.sh ./benchmark
```

Then build the parser.

```bash
make parser
./parser_ftrace
```

#### Step 3 - PieChart

Then goto the jsfiddle.net to get the piechart.
The source codes for JS is in `draw_pie_chart.js`.

### More information

- https://developers.google.com/chart/interactive/docs/gallery/piechart
- https://jsfiddle.net/linD026/a1fsxpkj/
