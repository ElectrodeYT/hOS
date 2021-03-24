#!/bin/bash

# wow such complicated
make clean
trap '' INT; make qemu-debug; true
make clean
make cloc
