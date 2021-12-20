#!/bin/bash
# Check if the script is being sourced or not
[[ $0 != $BASH_SOURCE ]] && true || (echo "please source this script and dont run it"; exit 0)
if ! [[ ":$PATH:" == *":$(dirname $(realpath $BASH_SOURCE))/compiler/opt/bin:"* ]]; then
  export PATH=$(dirname $(realpath $BASH_SOURCE))/compiler/opt/bin:$PATH
fi
