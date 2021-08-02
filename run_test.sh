#!/bin/bash

# wow such complicated
make clean
make compile-services
trap '' INT; make qemu-debug -j16; true
make clean
make cloc
