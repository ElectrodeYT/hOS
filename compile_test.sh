#!/bin/bash

# wow such complicated
make clean
make compile-services
if ! (test -f "sysroot/testa.elf" && test -f "sysroot/testb.elf";); then
  echo "Test services failed to compile"; false
fi
make -j16
make clean
make cloc
