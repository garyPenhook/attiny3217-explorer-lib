# CMake toolchain file for building this library with the AVR GCC toolchain.
#
# Usage:
#   cmake -S . -B build --toolchain cmake/avr-gcc-toolchain.cmake ...
# or via CMakePresets.json (see the "avr" preset).
#
# The actual MCU/CPU options (-mmcu, -Os, -flto, ...) are applied per target by
# attiny3217_explorer_apply_avr_options() in the top-level CMakeLists.txt. This
# file only selects the cross compiler so CMake does not fall back to the host
# C/C++ compiler.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Allow overriding the toolchain prefix/location, e.g.
#   -DAVR_TOOLCHAIN_PREFIX=avr-  -DAVR_TOOLCHAIN_DIR=/opt/avr-gcc/bin
set(AVR_TOOLCHAIN_PREFIX "avr-" CACHE STRING "AVR GCC tool prefix")
set(AVR_TOOLCHAIN_DIR "" CACHE PATH "Optional directory containing the AVR GCC tools")

if(AVR_TOOLCHAIN_DIR)
    set(_avr_hint "${AVR_TOOLCHAIN_DIR}")
endif()

find_program(AVR_C_COMPILER    "${AVR_TOOLCHAIN_PREFIX}gcc"     HINTS ${_avr_hint})
find_program(AVR_CXX_COMPILER  "${AVR_TOOLCHAIN_PREFIX}g++"     HINTS ${_avr_hint})
find_program(AVR_OBJCOPY       "${AVR_TOOLCHAIN_PREFIX}objcopy" HINTS ${_avr_hint})
find_program(AVR_OBJDUMP       "${AVR_TOOLCHAIN_PREFIX}objdump" HINTS ${_avr_hint})
find_program(AVR_SIZE          "${AVR_TOOLCHAIN_PREFIX}size"    HINTS ${_avr_hint})
find_program(AVR_GCC_AR        "${AVR_TOOLCHAIN_PREFIX}gcc-ar"  HINTS ${_avr_hint})
find_program(AVR_GCC_RANLIB    "${AVR_TOOLCHAIN_PREFIX}gcc-ranlib" HINTS ${_avr_hint})

if(NOT AVR_CXX_COMPILER)
    message(FATAL_ERROR
        "Could not find ${AVR_TOOLCHAIN_PREFIX}g++ on PATH. Install the AVR GCC "
        "toolchain or set -DAVR_TOOLCHAIN_DIR=/path/to/avr-gcc/bin.")
endif()

set(CMAKE_C_COMPILER   "${AVR_C_COMPILER}"   CACHE FILEPATH "AVR C compiler")
set(CMAKE_CXX_COMPILER "${AVR_CXX_COMPILER}" CACHE FILEPATH "AVR C++ compiler")

# Use the LTO-aware archiver/ranlib when present so -flto static libraries work.
if(AVR_GCC_AR AND AVR_GCC_RANLIB)
    set(CMAKE_AR     "${AVR_GCC_AR}"     CACHE FILEPATH "AVR GCC LTO-aware archiver")
    set(CMAKE_RANLIB "${AVR_GCC_RANLIB}" CACHE FILEPATH "AVR GCC LTO-aware ranlib")
endif()

# We are cross compiling: never try to run built executables on the host, and
# only look for libraries/headers in the toolchain, not the host system.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
