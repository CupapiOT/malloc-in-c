#!/bin/bash
# Runs the use_halloc program with arguments.
make clean
make build
./use_halloc.out "$@"
