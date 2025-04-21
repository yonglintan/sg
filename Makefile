CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./
SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TARGET = build/sing

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

# Run a .sg file
run:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run FILE=filename.sg [OUT=output.txt]"; \
		exit 1; \
	fi
	@if [ ! -f "$(FILE)" ]; then \
		echo "Error: File '$(FILE)' not found."; \
		exit 1; \
	fi
	@echo "Running $(FILE)..."
	@if [ -z "$(OUT)" ]; then \
		$(TARGET) $(FILE); \
	else \
		$(TARGET) $(FILE) > $(OUT); \
		echo "Output saved to $(OUT)"; \
	fi

.PHONY: all clean test repl
