#!/bin/bash
set -e

if ! [ -d "bin" ]; then
     mkdir bin
fi

gcc -g main.c -o bin/ss