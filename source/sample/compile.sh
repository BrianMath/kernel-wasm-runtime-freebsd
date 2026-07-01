#!/bin/sh

clang22 -O2 --target=wasm32 -mcpu=mvp -nostdinc -nostdlib -Wl,--no-entry -Wl,--export=sum_get sum_mem.c -o sum_mem.wasm
