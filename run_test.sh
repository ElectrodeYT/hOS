#!/bin/bash
# wow such complicated
if ! ( ./check-path.sh ); then
  echo check-path.sh failed, source change-path.sh
  exit -1
fi
make clean
make compile-services
if [$? -ne 0]; then
  echo "Test services failed to compile"; false
else
  trap '' INT; make qemu-debug -j16; true
fi
make clean
make cloc
