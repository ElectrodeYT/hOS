#!/bin/bash
rm prefix-cross.txt 2> /dev/null; true
echo "[constants]" > prefix-cross.txt
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PREFIX="$(realpath $SCRIPT_DIR/../compiler/opt/bin)/"
if command -v ccache &> /dev/null
then
    PREFIX="$PREFIX"
fi
echo "prefix = '$PREFIX'" >> prefix-cross.txt