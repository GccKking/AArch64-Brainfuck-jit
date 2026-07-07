# AArch64-Brainfuck-jit
This is a JIT compiler for the Brainfuck language targeting the AArch64 architecture. I came across an existing implementation for AMD64 and adapted its framework to build this AArch64 version. It supports both JIT compilation and interpreter execution modes, and has passed all test scripts in the test directory.
## Usage
(Ensure you are running on an AArch64-based Linux system. If not, errors such as SIGILL may occur. To run on AArch64-based Macs (Apple Silicon Macs), apart from specifying the `MAP_JIT` flag in `mmap` calls, additional modifications may be required for compatibility with the macOS ABI. You may conduct testing on your own)

Assume the compiled executable is named `jit`.

To run the Brainfuck program via the interpreter:

```bash
./jit -i <filename>
```

To compile and execute via JIT:

```bash
./jit -j <filename>
```

## Comment
Some commented-out code provides additional debugging features: the `print()` function outputs native AArch64 machine code generated during compilation. This project does not include a built-in disassembler, so you can use GDB to disassemble instructions instead.
There are also utilities to print the execution time of both JIT compilation and interpreter modes, along with the starting address of dynamically generated machine code and memory addresses of libc functions (these features are also annotated in the source code).

The project consists of only a small number of source files, so no Makefile build script is provided. You can compile it directly with the following command:

```bash
g++ AArch64_jit.cpp -o jit
```

If you need to debug the source code with GDB, append the `-g` flag during compilation.

The built-in interpreter is a standard Brainfuck bytecode virtual machine with a straightforward implementation that requires no extra explanation.

The JIT compiler converts processed Brainfuck byte streams into AArch64 machine code, writes the code into memory, and executes it.
## Comments on JIT compilation
BrainFuck's pre-optimization logic is straightforward: merge `n` consecutive identical increment or decrement operations into a single operation with operand `n`, and recognize `[+]` and `[-]` as zero-clearing operations.

The optimized BrainFuck byte stream will be split into the following basic operations: `op_add`, `op_sub`, `op_addn`, `op_subn`, `op_setz`, `op_jt`, `op_jf`, `op_in`, `op_out`.

### `op_add`
This operation increments the value stored in the memory cell pointed to by the BF pointer by n; the value of n is stored in the opcode structure after pre-optimization. The corresponding AArch64 assembly instructions for this operation are as follows: 

```asm
ldrb w8, [x19]
add w8, w8, #n
strb w8, [x19]
```
