CC ?= gcc
CFLAGS := -g
CFLAGS += -O2
#CFLAGS += -fsanitize=leak

PARSE := parser_ftrace
PARSE_SRC := $(PARSE:=.c)
PARSE_OBJ := $(PARSE_SRC:.c=.o)

GEN += $(PARSE)
GEN += $(PARSE_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

parser: $(PARSE_OBJ)
	$(CC) $(PARSE_OBJ) -o $(PARSE)

indent:
	clang-format -i *.[ch]

clean:
	rm -f $(GEN)
