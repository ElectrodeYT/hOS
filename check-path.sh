#!/bin/bash
echo check-path run
if [[ ":$PATH:" == *":$(dirname $(realpath $BASH_SOURCE))/compiler/opt/bin:"* ]]; then
  exit 0
else
  echo "path not correctly set, please source change-path.sh"
  exit 1
fi