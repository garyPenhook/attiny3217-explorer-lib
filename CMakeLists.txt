cmake_minimum_required(VERSION 3.21)

project(attiny3217_explorer_lib
    VERSION 0.2.0
    DESCRIPTION "Reusable ATtiny3217 Curiosity Nano Explorer peripheral library"
    HOMEPAGE_URL "https://github.com/garyPenhook/attiny3217-explorer-lib"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

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
option(ATTINY3217_EXPLORER_BUILD_TESTS "Build host-side unit tests" OFF)
option(ATTINY3217_EXPLORER_STRICT_WARNINGS "Treat warnings as errors for development builds" OFF)
option(ATTINY3217_EXPLORER_INSTALL "Generate install and package-export rules" ON)

# Whether we are actually building firmware (cross AVR) or host tests. Host
# tests are built with the host compiler and must not see AVR-only flags.
set(_ATTINY3217_EXPLORER_IS_AVR FALSE)
if(CMAKE_CXX_COMPILER MATCHES "avr-")
    set(_ATTINY3217_EXPLORER_IS_AVR TRUE)
endif()

# ---------------------------------------------------------------------------
# Host-only path: build the unit tests and stop. The pure logic under test is
# header-only, so we do not need the AVR toolchain here.
# ---------------------------------------------------------------------------
if(ATTINY3217_EXPLORER_BUILD_TESTS AND NOT _ATTINY3217_EXPLORER_IS_AVR)
    enable_testing()
    add_subdirectory(tests)
    return()
endif()

if(NOT _ATTINY3217_EXPLORER_IS_AVR)
    message(WARNING
        "CMAKE_CXX_COMPILER does not look like an avr- cross compiler. "
        "Configure with --toolchain cmake/avr-gcc-toolchain.cmake (or the 'avr' "
        "preset) to build firmware targets.")
endif()

# ---------------------------------------------------------------------------
# Toolchain sanity checks for firmware/packaging tooling.
# ---------------------------------------------------------------------------
foreach(_tool avr-objcopy avr-objdump avr-size)
    string(MAKE_C_IDENTIFIER "ATTINY3217_EXPLORER_PROG_${_tool}" _var)
    find_program(${_var} ${_tool})
    if(NOT ${_var})
        message(WARNING "Could not find ${_tool}; some post-build artifacts may be unavailable.")
    endif()
endforeach()

find_program(ATTINY3217_EXPLORER_GCC_AR avr-gcc-ar)
find_program(ATTINY3217_EXPLORER_GCC_RANLIB avr-gcc-ranlib)
if(ATTINY3217_EXPLORER_GCC_AR AND ATTINY3217_EXPLORER_GCC_RANLIB)
    # LTO archives must be created with the gcc wrappers so the LTO plugin runs.
    set(CMAKE_AR "${ATTINY3217_EXPLORER_GCC_AR}" CACHE FILEPATH "AVR GCC LTO-aware archiver" FORCE)
    set(CMAKE_RANLIB "${ATTINY3217_EXPLORER_GCC_RANLIB}" CACHE FILEPATH "AVR GCC LTO-aware ranlib" FORCE)
    set(CMAKE_CXX_COMPILER_AR "${ATTINY3217_EXPLORER_GCC_AR}" CACHE FILEPATH "AVR GCC LTO-aware C++ archiver" FORCE)
    set(CMAKE_CXX_COMPILER_RANLIB "${ATTINY3217_EXPLORER_GCC_RANLIB}" CACHE FILEPATH "AVR GCC LTO-aware C++ ranlib" FORCE)
    set(CMAKE_CXX_ARCHIVE_CREATE "${ATTINY3217_EXPLORER_GCC_AR} qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_APPEND "${ATTINY3217_EXPLORER_GCC_AR} q <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_FINISH "${ATTINY3217_EXPLORER_GCC_RANLIB} <TARGET>")
endif()

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# Shared AVR build flags
# ---------------------------------------------------------------------------

# MCU/optimisation flags needed to compile any translation unit for the part.
# The active MCU comes from -mmcu, not from a hard-coded __AVR_*__ definition.
set(ATTINY3217_EXPLORER_AVR_COMPILE_FLAGS
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
    -fno-use-cxa-atexit)

set(ATTINY3217_EXPLORER_AVR_LINK_FLAGS
    -mmcu=${ATTINY3217_EXPLORER_MCU}
    -Os
    -flto
    -mrelax
    -mcall-prologues
    -Wl,--gc-sections)

set(ATTINY3217_EXPLORER_WARNING_FLAGS -Wall -Wextra)
if(ATTINY3217_EXPLORER_STRICT_WARNINGS)
    # Stricter checks gated behind an opt-in so normal builds stay quiet. The
    # pervasive conversions in AVR register code are left out deliberately.
    list(APPEND ATTINY3217_EXPLORER_WARNING_FLAGS
        -Wshadow -Wdouble-promotion -Wcast-align -Werror)
endif()

# Apply MCU compile/link flags + warnings to a compiled target (STATIC/OBJECT/EXE).
function(attiny3217_explorer_apply_avr_options target)
    target_compile_options(${target} PRIVATE
        ${ATTINY3217_EXPLORER_AVR_COMPILE_FLAGS}
        ${ATTINY3217_EXPLORER_WARNING_FLAGS})

    get_target_property(target_type ${target} TYPE)
    if(NOT target_type STREQUAL "STATIC_LIBRARY" AND NOT target_type STREQUAL "OBJECT_LIBRARY")
        target_link_options(${target} PRIVATE ${ATTINY3217_EXPLORER_AVR_LINK_FLAGS})
    endif()
endfunction()

# ---------------------------------------------------------------------------
# hal: header-only ATtiny3217 peripheral helpers. Carries the public include
# path, the F_CPU definition, the C++ standard, and (for convenience) the MCU
# flags so downstream firmware targets compile correctly by just linking it.
# ---------------------------------------------------------------------------
add_library(attiny3217_explorer_hal INTERFACE)
add_library(attiny3217_explorer::hal ALIAS attiny3217_explorer_hal)

target_compile_features(attiny3217_explorer_hal INTERFACE cxx_std_23)
set_target_properties(attiny3217_explorer_hal PROPERTIES
    INTERFACE_CXX_EXTENSIONS ON
    EXPORT_NAME hal)
target_compile_definitions(attiny3217_explorer_hal INTERFACE F_CPU=${ATTINY3217_EXPLORER_F_CPU})
target_compile_options(attiny3217_explorer_hal INTERFACE ${ATTINY3217_EXPLORER_AVR_COMPILE_FLAGS})
target_link_options(attiny3217_explorer_hal INTERFACE ${ATTINY3217_EXPLORER_AVR_LINK_FLAGS})
target_include_directories(attiny3217_explorer_hal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
if(ATTINY3217_EXPLORER_DFP_INCLUDE)
    target_include_directories(attiny3217_explorer_hal INTERFACE
        $<BUILD_INTERFACE:${ATTINY3217_EXPLORER_DFP_INCLUDE}>)
endif()

# Helper: declare a compiled component library with consistent properties.
function(attiny3217_explorer_add_component target)
    cmake_parse_arguments(arg "" "" "SOURCES;LINK" ${ARGN})
    add_library(${target} STATIC ${arg_SOURCES})
    # Short, namespaced consumer-facing name (e.g. attiny3217_explorer::board).
    string(REPLACE "attiny3217_explorer_" "" _export_name ${target})
    add_library(attiny3217_explorer::${_export_name} ALIAS ${target})
    target_link_libraries(${target} PUBLIC attiny3217_explorer_hal ${arg_LINK})
    attiny3217_explorer_apply_avr_options(${target})
    set_target_properties(${target} PROPERTIES
        EXPORT_NAME ${_export_name}
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/lib")
endfunction()

# board: Curiosity Nano Explorer board devices (LED, analog input wrapper).
attiny3217_explorer_add_component(attiny3217_explorer_board
    SOURCES src/analog_sensor.cpp src/blink_task.cpp)

# drivers: reusable device drivers (SSD1306 OLED).
attiny3217_explorer_add_component(attiny3217_explorer_drivers
    SOURCES src/oled.cpp)

# Optional ISR-owning components. Each claims a global interrupt vector, so they
# are separate archives that consumers opt into explicitly.
attiny3217_explorer_add_component(attiny3217_explorer_timer0_irq
    SOURCES src/timer0.cpp)
attiny3217_explorer_add_component(attiny3217_explorer_uart0_rx_irq
    SOURCES src/uart0_rx.cpp)
attiny3217_explorer_add_component(attiny3217_explorer_button_irq
    SOURCES src/button_irq.cpp)

# platform_init: opinionated demo bring-up that wires the above together.
attiny3217_explorer_add_component(attiny3217_explorer_platform_init
    SOURCES src/platform_init.cpp
    LINK
        attiny3217_explorer_board
        attiny3217_explorer_drivers
        attiny3217_explorer_button_irq
        attiny3217_explorer_timer0_irq
        attiny3217_explorer_uart0_rx_irq)

# Convenience umbrella: the common non-ISR composition (hal + board + drivers).
add_library(attiny3217_explorer INTERFACE)
add_library(attiny3217_explorer::attiny3217_explorer ALIAS attiny3217_explorer)
target_link_libraries(attiny3217_explorer INTERFACE
    attiny3217_explorer_hal
    attiny3217_explorer_board
    attiny3217_explorer_drivers)

# Link this object into firmware targets that should carry the library's
# source-controlled ATtiny3217 fuse bytes in the ELF .fuse section.
add_library(attiny3217_explorer_fuses OBJECT src/fuses.cpp)
add_library(attiny3217_explorer::fuses ALIAS attiny3217_explorer_fuses)
set_target_properties(attiny3217_explorer_fuses PROPERTIES EXPORT_NAME fuses)
target_link_libraries(attiny3217_explorer_fuses PUBLIC attiny3217_explorer_hal)
attiny3217_explorer_apply_avr_options(attiny3217_explorer_fuses)

if(ATTINY3217_EXPLORER_BUILD_EXAMPLES)
    add_subdirectory(examples/telemetry_demo)
endif()

# ---------------------------------------------------------------------------
# Install & package export
# ---------------------------------------------------------------------------
if(ATTINY3217_EXPLORER_INSTALL)
    include(CMakePackageConfigHelpers)

    set(_explorer_components
        attiny3217_explorer_hal
        attiny3217_explorer_board
        attiny3217_explorer_drivers
        attiny3217_explorer_timer0_irq
        attiny3217_explorer_uart0_rx_irq
        attiny3217_explorer_button_irq
        attiny3217_explorer_platform_init
        attiny3217_explorer
        attiny3217_explorer_fuses)

    install(TARGETS ${_explorer_components}
        EXPORT attiny3217_explorerTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR})

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")

    set(_explorer_cmakedir ${CMAKE_INSTALL_LIBDIR}/cmake/attiny3217_explorer)

    install(EXPORT attiny3217_explorerTargets
        FILE attiny3217_explorerTargets.cmake
        NAMESPACE attiny3217_explorer::
        DESTINATION ${_explorer_cmakedir})

    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/attiny3217_explorerConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/attiny3217_explorerConfig.cmake"
        INSTALL_DESTINATION ${_explorer_cmakedir})

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/attiny3217_explorerConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/attiny3217_explorerConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/attiny3217_explorerConfigVersion.cmake"
        DESTINATION ${_explorer_cmakedir})
endif()
