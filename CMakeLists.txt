cmake_minimum_required(VERSION 3.21)

project(attiny3217_explorer_lib LANGUAGES CXX)

set(ATTINY3217_EXPLORER_MCU attiny3217 CACHE STRING "Target AVR MCU")
set(ATTINY3217_EXPLORER_F_CPU 20000000UL CACHE STRING "CPU clock used by firmware")
set(ATTINY3217_EXPLORER_DFP_INCLUDE
    "/home/gary/.mchp_packs/Microchip/ATtiny_DFP/3.4.278/include"
    CACHE PATH "Microchip ATtiny DFP include directory")

option(ATTINY3217_EXPLORER_BUILD_EXAMPLES "Build bundled example firmware" OFF)

find_program(ATTINY3217_EXPLORER_GCC_AR avr-gcc-ar)
find_program(ATTINY3217_EXPLORER_GCC_RANLIB avr-gcc-ranlib)
if(ATTINY3217_EXPLORER_GCC_AR AND ATTINY3217_EXPLORER_GCC_RANLIB)
    set(CMAKE_AR "${ATTINY3217_EXPLORER_GCC_AR}" CACHE FILEPATH "AVR GCC LTO-aware archiver" FORCE)
    set(CMAKE_RANLIB "${ATTINY3217_EXPLORER_GCC_RANLIB}" CACHE FILEPATH "AVR GCC LTO-aware ranlib" FORCE)
    set(CMAKE_CXX_COMPILER_AR "${ATTINY3217_EXPLORER_GCC_AR}" CACHE FILEPATH "AVR GCC LTO-aware C++ archiver" FORCE)
    set(CMAKE_CXX_COMPILER_RANLIB "${ATTINY3217_EXPLORER_GCC_RANLIB}" CACHE FILEPATH "AVR GCC LTO-aware C++ ranlib" FORCE)
    set(CMAKE_CXX_ARCHIVE_CREATE "${ATTINY3217_EXPLORER_GCC_AR} qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_APPEND "${ATTINY3217_EXPLORER_GCC_AR} q <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH "${ATTINY3217_EXPLORER_GCC_RANLIB} <TARGET>")
endif()

function(attiny3217_explorer_apply_avr_options target)
    target_compile_features(${target} PUBLIC cxx_std_23)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS ON)

    target_compile_definitions(${target} PUBLIC
        F_CPU=${ATTINY3217_EXPLORER_F_CPU}
        __AVR_ATtiny3217__
    )

    target_compile_options(${target} PRIVATE
        -mmcu=${ATTINY3217_EXPLORER_MCU}
        -Os
        -flto
        -mrelax
        -mcall-prologues
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
        -fno-use-cxa-atexit
        -Wall
        -Wextra
    )

    get_target_property(target_type ${target} TYPE)
    if(NOT target_type STREQUAL "STATIC_LIBRARY" AND NOT target_type STREQUAL "OBJECT_LIBRARY")
        target_link_options(${target} PRIVATE
            -mmcu=${ATTINY3217_EXPLORER_MCU}
            -Os
            -flto
            -mrelax
            -mcall-prologues
            -Wl,--gc-sections
        )
    endif()
endfunction()

add_library(attiny3217_explorer STATIC
    src/analog_sensor.cpp
    src/blink_task.cpp
    src/button_irq.cpp
    src/oled.cpp
    src/platform_init.cpp
    src/timer0.cpp
    src/uart0_rx.cpp
)

target_include_directories(attiny3217_explorer PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${ATTINY3217_EXPLORER_DFP_INCLUDE}
)

set_target_properties(attiny3217_explorer PROPERTIES
    OUTPUT_NAME "attiny3217_explorer"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib"
)

attiny3217_explorer_apply_avr_options(attiny3217_explorer)

# Link this object into firmware targets that should carry the library's
# source-controlled ATtiny3217 fuse bytes in the ELF .fuse section.
add_library(attiny3217_explorer_fuses OBJECT src/fuses.cpp)
target_include_directories(attiny3217_explorer_fuses PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${ATTINY3217_EXPLORER_DFP_INCLUDE}
)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_fuses)

if(ATTINY3217_EXPLORER_BUILD_EXAMPLES)
    add_subdirectory(examples/telemetry_demo)
endif()
