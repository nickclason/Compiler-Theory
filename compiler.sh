#!/bin/bash
output/compiler "$@"
gcc output/output.o src/runtime.c -o a.out


