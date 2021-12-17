#!/bin/bash
rm -rf mlibc-build/*
meson mlibc mlibc-build --cross-file meson-cross.txt -Dstatic=true -Dprefix=$(cd ../sysroot/usr; pwd) 
