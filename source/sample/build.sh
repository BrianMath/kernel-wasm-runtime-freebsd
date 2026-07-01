#!/bin/sh

clang -I../source -o module module.c source/libm3.a -lm
