CC ?= gcc
CFLAGS := -g
#CFLAGS += -O2

BENCHMARK := benchmark
BENCHMARK_SRC := $(BENCHMARK:=.c)
BENCHMARK_OBJ := $(BENCHMARK_SRC:.c=.o)

PARSE := parse_ftrace_to_table
PARSE_SRC := $(PARSE:=.c)
PARSE_OBJ := $(PARSE_SRC:.c=.o)

GEN := $(BENCHMARK)
GEN += $(BENCHMARK_OBJ)

GEN += $(PARSE)
GEN += $(PARSE_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

benchmark: $(BENCHMARK_OBJ)
	$(CC) $(BENCHMARK_OBJ) -o $(BENCHMARK)

parse: $(PARSE_OBJ)
	$(CC) $(PARSE_OBJ) -o $(PARSE)

clean:
	rm -f $(GEN)
