#!/bin/bash

SRC_DIR="src"
LIB_DIR="libs"

gcc -Wall -Wextra -pedantic -c $SRC_DIR/hello_from_static_lib.c -o $SRC_DIR/hello_from_static_lib.o
ar rcs $LIB_DIR/libhello.a $SRC_DIR/hello_from_static_lib.o

gcc -Wall -Wextra -pedantic $SRC_DIR/hello.c -L$LIB_DIR -lhello  -o hello

if [ $? -eq 0 ]; then 
    echo "file compiled"
else
    echo "error"
fi