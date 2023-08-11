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
esac

export SDL2DIR="/lib/x86_64-linux-gnu"  
cmake -D CMAKE_C_COMPILER=gcc -D CMAKE_CXX_COMPILER=g++ -D CMAKE_C_COMPILER_LAUNCHER="" -D CMAKE_CXX_COMPILER_LAUNCHER="" -D CMAKE_C_LINKER_LAUNCHER="" -D CMAKE_CXX_LINKER_LAUNCHER="" -D SDL2DIR=${SDL2DIR} -D CMAKE_BUILD_TYPE=${type} .
make -j12
