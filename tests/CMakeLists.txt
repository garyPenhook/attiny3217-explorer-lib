# Host-side unit tests for the header-only pure logic. Built with the host
# compiler (not the AVR cross compiler) via the top-level BUILD_TESTS option.

add_library(attiny3217_explorer_test_headers INTERFACE)
target_include_directories(attiny3217_explorer_test_headers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR})
target_compile_features(attiny3217_explorer_test_headers INTERFACE cxx_std_20)
target_compile_options(attiny3217_explorer_test_headers INTERFACE -Wall -Wextra)

set(_explorer_tests
    test_bit_ops
    test_fast_format
    test_adc_scale)

foreach(_test ${_explorer_tests})
    add_executable(${_test} ${_test}.cpp)
    target_link_libraries(${_test} PRIVATE attiny3217_explorer_test_headers)
    add_test(NAME ${_test} COMMAND ${_test})
endforeach()
