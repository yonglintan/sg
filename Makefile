CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc

# Define source directories
SRC_DIRS = src src/ast src/frontend src/backend src/runtime

# Find all source files
SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))

# Generate object file paths in a flat build directory
OBJS := $(patsubst %.c,build/%.o,$(notdir $(SRCS)))

# Add VPATH to help make find source files
VPATH := $(SRC_DIRS)

TARGET = build/sing

all: $(TARGET)

$(TARGET): $(OBJS) | build
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Rule for all object files
build/%.o: %.c | build
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
	$(TARGET) $(FILE)

.PHONY: all clean test repl
