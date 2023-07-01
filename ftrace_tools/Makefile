CC ?= gcc
CFLAGS := -g
CFLAGS += -O2
#CFLAGS += -fsanitize=leak

BENCHMARK := benchmark
BENCHMARK_SRC := $(BENCHMARK:=.c)
BENCHMARK_OBJ := $(BENCHMARK_SRC:.c=.o)

PARSE := parser_ftrace
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

parser: $(PARSE_OBJ)
	$(CC) $(PARSE_OBJ) -o $(PARSE)

indent:
	clang-format -i *.[ch]

clean:
	rm -f $(GEN)
