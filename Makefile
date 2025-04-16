CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TARGET = build/sg

all: $(TARGET)

$(TARGET): $(OBJS) | build
	$(CC) $(CFLAGS) -o $@ $(OBJS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

repl: $(TARGET)
	$(TARGET)

test: $(TARGET)
	./test_sg.sh

.PHONY: all clean test repl
