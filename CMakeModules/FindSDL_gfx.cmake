# Locate SDL_MIXER library
#
# This module defines:
#
# ::
#
#   SDL_GFX_LIBRARIES, the name of the library to link against
#   SDL_GFX_INCLUDE_DIRS, where to find the headers
#   SDL2_MIXER_FOUND, if false, do not try to link against
#
#
#
# For backward compatibility the following variables are also set:
#
# ::
#
#   SDLMIXER_LIBRARY (same value as SDL_GFX_LIBRARIES)
#   SDLMIXER_INCLUDE_DIR (same value as SDL_GFX_INCLUDE_DIRS)
#   SDLMIXER_FOUND (same value as SDL2_MIXER_FOUND)
#
#
#
# $SDLDIR is an environment variable that would correspond to the
# ./configure --prefix=$SDLDIR used in building SDL.
#
# Created by Eric Wing.  This was influenced by the FindSDL.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(SDL_GFX_INCLUDE_DIR SDL_gfxPrimitives.h
        HINTS
        ENV SDLGFXDIR
        ENV SDL2DIR
        PATH_SUFFIXES SDL
        # path suffixes to search inside ENV{SDLDIR}
        include/SDL include
        PATHS ${SDL_GFX_PATH}
        )

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(VC_LIB_PATH_SUFFIX lib/x64)
else()
    set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

find_library(SDL_GFX_LIBRARY
        NAMES SDL_gfx
        HINTS
        ENV SDLGFXDIR
        ENV SDL2DIR
        PATH_SUFFIXES lib bin ${VC_LIB_PATH_SUFFIX}
        PATHS ${SDL_GFX_PATH}
        )

set(SDL_GFX_LIBRARIES ${SDL_GFX_LIBRARY})
set(SDL_GFX_INCLUDE_DIRS ${SDL_GFX_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL_gfx
        REQUIRED_VARS SDL_GFX_LIBRARIES SDL_GFX_INCLUDE_DIRS)

# for backward compatibility
set(SDLGFX_LIBRARY ${SDL_GFX_LIBRARIES})
set(SDLGFX_INCLUDE_DIR ${SDL_GFX_INCLUDE_DIRS})
set(SDLGFX_FOUND ${SDL_GFX_FOUND})

mark_as_advanced(SDL_GFX_LIBRARY SDL_GFX_INCLUDE_DIR)