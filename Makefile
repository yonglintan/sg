CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./
SRCS = src/main.c src/scanner.c
OBJS = $(SRCS:.c=.o)
TARGET = sg

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

repl: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) tests/test.sg

test-errors: $(TARGET)
	-./$(TARGET) tests/error_tests.sg

test-strings: $(TARGET)
	-./$(TARGET) tests/unterminated_string.sg

test-all: test test-errors test-strings

.PHONY: all clean test test-errors test-strings test-all repl
