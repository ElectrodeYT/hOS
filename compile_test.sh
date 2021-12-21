#!/bin/bash
# wow such complicated
if ! ( ./check-path.sh ); then
  echo check-path.sh failed, source change-path.sh
  exit -1
fi
make clean
make compile-services
if ! (test -f "sysroot/testa.elf" && test -f "sysroot/testb.elf";); then
  echo "Test services failed to compile"; false
fi
make -j16
make clean
make cloc
