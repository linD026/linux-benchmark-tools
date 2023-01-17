CC ?= gcc
CFLAGS := -g
CFLAGS += -O2

BENCHMARK := benchmark
BENCHMARK_SRC := $(BENCHMARK:=.c)
BENCHMARK_OBJ := $(BENCHMARK_SRC:.c=.o)

GEN := $(BENCHMARK)
GEN += $(BENCHMARK_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

benchmark: $(BENCHMARK_OBJ)
	$(CC) $(BENCHMARK_OBJ) -o $(BENCHMARK)

clean:
	rm -f $(GEN)
