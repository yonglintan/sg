CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./
SRCS = $(wildcard src/*.c)
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
	./test_sg.sh

.PHONY: all clean test repl
