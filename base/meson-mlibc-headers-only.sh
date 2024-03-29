#!/bin/bash
# Generate the real crossfile
./meson-create-prefix-crossfile.sh
rm -rf mlibc-build/*
mkdir -p ../sysroot/usr
meson mlibc mlibc-build --cross-file prefix-cross.txt --cross-file meson-cross.txt -Dprefix=$(cd ../sysroot/usr; pwd) -Dheaders_only=true
