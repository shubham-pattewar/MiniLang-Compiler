# MiniLang Compiler

An educational mini compiler implemented in C++17. MiniLang source code is tokenized, parsed into an AST, checked for basic semantic errors, translated to a three-address-style intermediate representation (IR), optionally optimized, and executed by a small virtual machine.

> This is a learning project, not a production compiler. It currently supports integer values only and has a deliberately small language and runtime.

## Features

- Lexer/scanner with source line and column positions
- Recursive-descent parser and AST printer
- `let` declarations, variable assignment, `print(...)`, blocks, `if`/`else`, and `while`
- Integer arithmetic: `+`, `-`, `*`, `/`, unary `-`
- Comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=` (zero is false; non-zero is true)
- Semantic checks for duplicate declarations and undeclared variable use
- Intermediate representation and virtual-machine execution
- Optional local constant folding
- Helpful syntax/runtime errors and an instruction limit for runaway loops
- Example programs

## Requirements

- C++17 compiler: GCC/MinGW-w64, Clang, or MSVC
- Optional: CMake 3.16+

## Build with g++

From the repository root:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic src/main.cpp -o minilang
```

On Windows, this normally creates `minilang.exe`.

## Build with CMake

```bash
cmake -S . -B build
cmake --build build
```

## Run examples

```bash
./minilang examples/hello.mini
./minilang examples/arithmetic.mini
./minilang examples/loops.mini
./minilang examples/conditionals.mini
```

On Windows PowerShell, use `./minilang.exe examples/hello.mini` (or the path inside your build output folder when using CMake).

## Compiler inspection modes

```bash
./minilang --tokens examples/arithmetic.mini
./minilang --ast examples/arithmetic.mini
./minilang --ir examples/arithmetic.mini
./minilang --optimize examples/arithmetic.mini
```

- `--tokens`: inspect lexical tokens
- `--ast`: inspect the abstract syntax tree
- `--ir`: inspect generated instructions
- `--optimize`: optimize the IR, print it, then execute it
- no flag: compile and execute the source program

## MiniLang syntax

```text
let identifier = expression;
identifier = expression;
print(expression);

if (expression) {
    statements
} else {
    statements
}

while (expression) {
    statements
}
```

Expressions support integer literals, variables, parentheses, unary minus, arithmetic, and comparisons. Line comments begin with `//`.

## Example

Input:

```text
let x = 10;
let y = 20;
print(x + y * 2);
```

Output:

```text
50
```

## Architecture

1. **Lexer** — converts characters into tokens.
2. **Parser** — validates grammar and constructs AST nodes.
3. **Semantic checks** — ensure variables are declared before use and not redeclared.
4. **IR generation** — emits a simple instruction list with temporaries and jumps.
5. **Optimizer** — performs local constant folding where safe.
6. **Virtual machine** — executes IR instructions.

## Current limitations

- Only integer values are supported.
- Variables use a simple program-wide declaration table; lexical block scopes are not implemented.
- The optimizer is intentionally basic and does not perform control-flow analysis or dead-code elimination.
- IR is executed by the custom VM; this project does not yet emit native machine code or LLVM IR.
- Integer overflow is not comprehensively checked.

## Next improvements

- Add proper lexical scopes and block-local variables.
- Add strings, booleans, and a type checker.
- Add source maps for runtime errors.
- Add more automated tests and error recovery.
- Add LLVM IR output or native code generation as a separate milestone.

## Academic integrity note

If you use this project for an application or portfolio, study and test the implementation, make your own modifications, and describe only the components you can explain and have personally verified. Do not claim features you have not implemented.
