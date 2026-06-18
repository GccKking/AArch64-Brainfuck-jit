# AArch64-Brainfuck-jit
This is a JIT compiler for the Brainfuck language targeting the AArch64 architecture. I came across an existing implementation for AMD64 and adapted its framework to build this AArch64 version. It supports both JIT compilation and interpreter execution modes, and has passed all test scripts in the test directory.
## Usage
(Ensure you are running on an AArch64-based Linux system.If not, errors such as SIGILL may occur)

Assume the compiled executable is named ==jit==.

To run the Brainfuck program via the interpreter:

`./jit -i <filename>`

To compile and execute via JIT:

`./jit -j <filename>`
