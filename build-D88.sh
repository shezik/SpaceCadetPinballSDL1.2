#!/bin/bash
gcc_path=$(which mipsel-linux-gcc)
export SDLDIR="${gcc_path}/../../mipsel-buildroot-linux-uclibc/sysroot/usr/lib"  
# echo ${SDLDIR}
cmake -D CMAKE_C_COMPILER=mipsel-linux-gcc -D CMAKE_CXX_COMPILER=mipsel-linux-g++ -D SDL2DIR=${SDLDIR} -D CMAKE_BUILD_TYPE=Release .
