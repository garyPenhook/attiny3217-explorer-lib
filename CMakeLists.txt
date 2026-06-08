cmake_minimum_required(VERSION 3.21)

project(attiny3217_explorer_lib LANGUAGES CXX)

set(ATTINY3217_EXPLORER_MCU attiny3217 CACHE STRING "Target AVR MCU")
set(ATTINY3217_EXPLORER_F_CPU 20000000UL CACHE STRING "CPU clock used by firmware")

set(_ATTINY3217_EXPLORER_DFP_INCLUDE_DEFAULT "")
if(DEFINED ENV{ATTINY3217_EXPLORER_DFP_INCLUDE})
    set(_ATTINY3217_EXPLORER_DFP_INCLUDE_DEFAULT "$ENV{ATTINY3217_EXPLORER_DFP_INCLUDE}")
endif()
set(ATTINY3217_EXPLORER_DFP_INCLUDE
    "${_ATTINY3217_EXPLORER_DFP_INCLUDE_DEFAULT}"
    CACHE PATH "Optional Microchip ATtiny DFP include directory")
if(ATTINY3217_EXPLORER_DFP_INCLUDE AND NOT IS_DIRECTORY "${ATTINY3217_EXPLORER_DFP_INCLUDE}")
    message(FATAL_ERROR
        "ATTINY3217_EXPLORER_DFP_INCLUDE does not exist: "
        "${ATTINY3217_EXPLORER_DFP_INCLUDE}")
endif()

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

function(attiny3217_explorer_apply_public_includes target)
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    )

    if(ATTINY3217_EXPLORER_DFP_INCLUDE)
        target_include_directories(${target} PUBLIC
            ${ATTINY3217_EXPLORER_DFP_INCLUDE}
        )
    endif()
endfunction()

add_library(attiny3217_explorer STATIC
    src/analog_sensor.cpp
    src/blink_task.cpp
    src/oled.cpp
)

attiny3217_explorer_apply_public_includes(attiny3217_explorer)

set_target_properties(attiny3217_explorer PROPERTIES
    OUTPUT_NAME "attiny3217_explorer"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib"
)

attiny3217_explorer_apply_avr_options(attiny3217_explorer)

add_library(attiny3217_explorer_timer0 STATIC src/timer0.cpp)
attiny3217_explorer_apply_public_includes(attiny3217_explorer_timer0)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_timer0)
set_target_properties(attiny3217_explorer_timer0 PROPERTIES
    OUTPUT_NAME "attiny3217_explorer_timer0"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib"
)

add_library(attiny3217_explorer_button_irq STATIC src/button_irq.cpp)
target_link_libraries(attiny3217_explorer_button_irq PUBLIC attiny3217_explorer)
attiny3217_explorer_apply_public_includes(attiny3217_explorer_button_irq)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_button_irq)
set_target_properties(attiny3217_explorer_button_irq PROPERTIES
    OUTPUT_NAME "attiny3217_explorer_button_irq"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib"
)

add_library(attiny3217_explorer_uart0_rx STATIC src/uart0_rx.cpp)
attiny3217_explorer_apply_public_includes(attiny3217_explorer_uart0_rx)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_uart0_rx)
set_target_properties(attiny3217_explorer_uart0_rx PROPERTIES
    OUTPUT_NAME "attiny3217_explorer_uart0_rx"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib"
)

add_library(attiny3217_explorer_platform_init STATIC src/platform_init.cpp)
target_link_libraries(attiny3217_explorer_platform_init PUBLIC
    attiny3217_explorer
    attiny3217_explorer_button_irq
    attiny3217_explorer_timer0
    attiny3217_explorer_uart0_rx
)
attiny3217_explorer_apply_public_includes(attiny3217_explorer_platform_init)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_platform_init)
set_target_properties(attiny3217_explorer_platform_init PROPERTIES
    OUTPUT_NAME "attiny3217_explorer_platform_init"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib"
)

# Link this object into firmware targets that should carry the library's
# source-controlled ATtiny3217 fuse bytes in the ELF .fuse section.
add_library(attiny3217_explorer_fuses OBJECT src/fuses.cpp)
attiny3217_explorer_apply_public_includes(attiny3217_explorer_fuses)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_fuses)

if(ATTINY3217_EXPLORER_BUILD_EXAMPLES)
    add_subdirectory(examples/telemetry_demo)
endif()
