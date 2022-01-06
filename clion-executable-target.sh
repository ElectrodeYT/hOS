#!/bin/bash
( cd arch/x86_64 || exit 1; gdb -x ../../sysroot/boot/gdb.gdb -tui )