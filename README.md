# Kernel Wasm Runtime

A Linux kernel module that embeds a WebAssembly (Wasm) runtime (using Wasm3 for now) and exposes a character device interface for loading, activating, deactivating, and unloading Wasm modules in kernel space.
This enables dynamic kernel instrumentation and extension using safe, sandboxed Wasm code.

## Compile WASM3 and User-Space Program

Make sure the system you are working with can compile a Linux kernel module. One great tutorial is [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/). Compile the code with the provided Makefile or regular c compiler as follows.

- Program that takes input from user-space to WASM3 kernel module
  
    ```
    gcc -o wasm_manager wasm_user_input.c
    ```

- Wasm kernel module
  
  ```
  make
  ```

## Running the program

### Testing the Kernel Module

At this point you can try running some of the test scripts provided in the test directory. This is a good sanity check to make sure everything is in place.

**Possible issue encountered:** need to disable secure boot to insert a kernel module. See [screenshot](docs/screenshots/bios4.jpg).

### Using Wasm kernel module
  
- Insert/load the kernel module
  
  ```
  sudo insmod kernel_wasm.ko
  ```

- List all the loaded wasm modules

  ```
  sudo ./wasm_manager list
  ```

- Load a wasm module -> prints a wasm descriptor to the user
  
  ```
  sudo ./wasm_manager load "wasm_file_path"
  ```

- Report (print a message to the user)

  ```
  sudo ./wasm_manager report "wasm_descriptor"
  ```

- Temporarily activate a wasm module

  ```
  sudo ./wasm_manager activate "wasm_descriptor"
  ```

- Temporarily deactivate a wasm module

  ```
  sudo ./wasm_manager deactivate "wasm_descriptor"
  ```

- Unload a wasm module

  ```
  sudo ./wasm_manager unload "wasm_descriptor"
  ```


- Remove the kernel module

  ```
  sudo rmmod wasm3kernel_entry
  ```

### Wasm Module Example

The structure of the wasm modules provided by the users require three important functions: `pre`, `post`, and `report`. The user can choose to omit any of these functions if they don't require them.

For now the kernel module only supports tracing syscalls. So the user needs to export the respective functions in the form of `kprobe:syscall:pre/post`

for example if the user wants to trace the mkdir syscall their wasm module should have the following structure:

```Cpp
__attribute__((export_name("kprobe:__x64_sys_mkdir:pre")))
void f1() {
}

__attribute__((export_name("kprobe:__x64_sys_mkdir:post")))
void f2() {
}

// or you can just name this function report
__attribute__((export_name("report")))
char* f3() {
}
```

More examples can be found in the wasm_probes directory. It is also important to note that the report function is required to return a string. The user can choose to format it in any way they want. The kernel module just relays this information back to the user.