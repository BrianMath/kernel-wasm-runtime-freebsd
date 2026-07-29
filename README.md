# Kernel Wasm Runtime for FreeBSD

This repository was inspired by [Faisal-Saleh's kernel-wasm-runtime](https://github.com/Faisal-Saleh/kernel-wasm-runtime). Differently from them, this project focused on creating a **FreeBSD** loadable kernel module (KLD) that embeds a Wasm3 runtime.

As an example, this project also implemented a simple firewall for IP packages coming from `ufg.br`.

Everything I made is inside of the `source/` folder.

## What you need
1. [FreeBSD](https://www.freebsd.org/where/) (of course)
2. [Task](https://github.com/go-task/task) to run the commands
3. Make, since some of Task commands are shortcuts to Make commands
4. [Rust and Cargo](https://doc.rust-lang.org/cargo/getting-started/installation.html) to compile Rust programs into Wasm
5. [Clang 22](https://github.com/llvm/llvm-project/releases#release-llvmorg-22.1.7) to compile C programs into Wasm

## Understanding the project

The main file is `source/wasmodule.c`. It contains the code of the "wasmodule" KLD. That includes the libs needed for the KLD, besides some network libs (explained later) and also the Wasm3 headers.

The `loader()` function contains the code that interacts with Wasm3 in different ways to show the possibilities of this module. There's also some code that interacts with Pfil to intercept network packages and print their src and dst IP addresses.

In `source/funcs/` there are Rust and C programs along with their Wasm translated code. In `source/` you'll find a header file for each of these programs, and these headers are included in `source/wasmodule.c`.

## Compiling the KLD

First things first, change to the `source/` directory with 
```bash
cd source/
```
### Building the functions
For **Rust** functions:
```bash
task "build <function_name>"
```

For **C** functions: \
Before building you need to edit the `Taskfile.yml` file in the `cbuild *` task. So far it is hardcoded for the `sum_mem.c` file and you need to add the flag `-Wl,--export=<function_name>` for each exported function. Then you can build with
```bash
task "cbuild <function_name>"
```

### Creating the headers
For **Rust** functions:
```bash
task "header <function_name>"
```

For **C** functions:
```bash
task "cheader <function_name>"
```

### Compiling the module
Remember to modify the `wasmodule.c` file including the generated headers in the last step. Then you can compile the module with
```bash
task
```

## Using the KLD

- **Load** the KLD to the kernel with
```bash
task load
```

- **Check** that the KLD was correctly loaded with
```bash
kldstat
```

- **See** the output from the KLD with
```bash
dmesg
```

- **Unload** the KLD from the kernel with
```bash
task unload
```
