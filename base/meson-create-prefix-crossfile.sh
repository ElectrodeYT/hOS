#!/bin/bash
rm prefix-cross.txt 2> /dev/null; true
echo "[constants]" > prefix-cross.txt
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PREFIX="$(realpath $SCRIPT_DIR/../compiler/opt/bin)/"
echo "prefix = '$PREFIX'" >> prefix-cross.txt