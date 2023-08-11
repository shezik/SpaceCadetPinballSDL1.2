#!/bin/bash
cd $(dirname $0)

case $1 in
clean)
    rm CMakeCache.txt Makefile cmake_install.cmake
    rm -r CMakeFiles
    exit 0;;
debug|Debug)
    type="Debug";;
release|Release|*)
    type="Release";;
DbgRelease|dbgrelease)
    type="RelWithDebInfo";;
esac

gcc_path=$(which mipsel-linux-gcc)
export SDL2DIR="${gcc_path}/../../mipsel-buildroot-linux-uclibc/sysroot/usr/lib"  
cmake -D CMAKE_C_COMPILER=mipsel-linux-gcc -D CMAKE_CXX_COMPILER=mipsel-linux-g++ -D CMAKE_C_COMPILER_LAUNCHER="" -D CMAKE_CXX_COMPILER_LAUNCHER="" -D CMAKE_C_LINKER_LAUNCHER="" -D CMAKE_CXX_LINKER_LAUNCHER="" -D SDL2DIR=${SDL2DIR} -D CMAKE_BUILD_TYPE=${type} .
make -j12
