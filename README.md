# Sing

Sing is a simple, C-inspired language that uses Singlish colloquialisms as keywords, aiming to be a more fun and familiar programming language to Singaporeans, especially for those learning to code for the first time.

## Features

- C-like syntax structure
- Singlish keywords
- Variables
- Control Flow
- Functions
- Closures
- Basic data types (numbers, strings, booleans, nil)
- Dynamic typing
- Interactive REPL (Read-Eval-Print Loop)

## Getting Started

### Prerequisites

- A C compiler (like `gcc` or `clang`)
- `make` build tool

### Building

1.  Clone the repository.
2.  Navigate to the project directory (`cd sg`).
3.  Run the `make` command:
    ```bash
    make
    ```
    This will compile the source code and place the executable (`sg` or `sg.exe`) in the `build/` directory.

### Running

You can run Sing as a REPL or to run script files by running the built executable, or using the `make` command:

1.  **Interactive REPL:** Start up the interpreter for interactive code execution using `make repl`.

    ```bash
    make repl  # or alternatively run the sg executable `./build/sg`
    > chope x = 10 lah // Example command
    > print x + 5 lah // Example command
    15
    ```

2.  **Running a Script File:** Pass the path to a `.sg` file as an argument.

    ```bash
    make run FILE=path/to/your/script.sg

    # alternatively
    ./build/sg path/to/your/script.sg
    ```

    Sample scripts can be found in the `bin/samples/` directory.

## Project Structure

- `src/`: Contains the C source code for the interpreter (scanner, parser, interpreter, etc.).
- `tests/`: Contains test scripts for verifying language features.
- `Makefile`: Defines build rules for compiling the project.

## Example

```sing
chope a = 1 lah
chope b = 2; // Semicolons can also be used to terminate lines
print a + b lah // Output: 3

can (a < b) {
  print "a is smaller lah";
} cannot {
  print "b is smaller or same lah";
}
// Output: a is smaller lah

howdo sum(x, y) {
  print x + y lah
}

sum(5, 8); // Output: 13
```
