#!/bin/bash
# parse args
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PREFIX=$(realpath $SCRIPT_DIR/opt)
SYSROOT=$(realpath $SCRIPT_DIR/../sysroot)
echo "using PREFIX=$PREFIX, SYSROOT=$SYSROOT"
# Confirm the correct opt path is in path
if ! [[ ":$PATH:" == *":$PREFIX/bin:"* ]]; then
  echo "path not set correctly; please source change-path.sh"
  echo "path is: $PATH"
  echo "path should have in it: $PREFIX/bin"
  exit 0;
fi

# Check if we want to clean the cross compiler build dirs (and opt)
if [[ " $@ " =~ " clean " ]]; then
  echo cleaning directories
  rm -rf gcc-11.2.0 binutils-2.37 build-gcc build-binutils opt/* 2> /dev/null; true
fi
if ! [ -f "gcc-11.2.0.tar.xz" ]; then
  echo downloading gcc
  wget http://mirror.easyname.at/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz
fi
if ! [ -f "binutils-2.37.tar.xz" ]; then
  echo downloading binutils
  wget http://mirror.easyname.at/gnu/binutils/binutils-2.37.tar.xz
fi
if ! [ -d "gcc-11.2.0" ]; then
  echo extracting gcc
  tar -xf gcc-11.2.0.tar.*
  echo patching gcc
  patch -p1 < ../2-gcc-11.2.0.patch
  ( cd gcc-11.2.0; ./contrib/download_prerequisites )
fi
if ! [ -d "binutils-2.37" ]; then
  echo extracting binutils
  tar -xf binutils-2.37.tar.*
  echo patching binutils
  patch -p1 < ../1-binutils-2.37.patch
fi
echo cleaning entire project
make -C .. clean
echo create sysroot headers
# Install mlibc headers
( cd ../base; ./meson-mlibc-headers-only.sh )
( cd ../base; ninja -C mlibc-build; ninja -C mlibc-build install )
# Config binutils
if ! [ -d build-binutils ]; then
  echo config binutils
  mkdir build-binutils
  (cd build-binutils; ../binutils-2.37/configure --target=x86_64-hos --prefix="$PREFIX" --with-sysroot="$SYSROOT" --enable-languages=c,c++ )
  if [ $? -ne 0 ]; then
    echo config failed
    exit -1
  fi
fi
# Build binutils
echo build binutils
(cd build-binutils; make -j$(nproc) )
if [ $? -ne 0 ]; then
  echo build failed
  exit -1
fi
# Install binutils
echo install binutils
(cd build-binutils; make install -j$(nproc) )
# Config gcc
if ! [ -d build-gcc ]; then
  echo config gcc
  mkdir -p build-gcc
  (cd build-gcc; ../gcc-11.2.0/configure --target=x86_64-hos --prefix="$PREFIX" --with-sysroot="$SYSROOT" --enable-languages=c,c++ )
  if [ $? -ne 0 ]; then
    echo config failed
    exit -1
  fi
fi
# Build gcc
echo build gcc
(cd build-gcc; make all-gcc all-target-libgcc -j$(nproc) )
if [ $? -ne 0 ]; then
  echo build failed
  exit -1
fi
# Install gcc
echo install gcc
(cd build-gcc; make install-gcc install-target-libgcc -j$(nproc) )

