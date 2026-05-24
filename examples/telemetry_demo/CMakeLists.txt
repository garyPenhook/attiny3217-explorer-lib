add_executable(telemetry_demo
    main.cpp
    app.cpp
    $<TARGET_OBJECTS:attiny3217_explorer_fuses>
)

target_link_libraries(telemetry_demo PRIVATE attiny3217_explorer)
attiny3217_explorer_apply_avr_options(telemetry_demo)

find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_OBJDUMP avr-objdump REQUIRED)
find_program(AVR_READELF avr-readelf REQUIRED)
find_program(AVR_NM avr-nm REQUIRED)

set(TELEMETRY_DEMO_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out/telemetry_demo")

set_target_properties(telemetry_demo PROPERTIES
    OUTPUT_NAME "telemetry_demo"
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${TELEMETRY_DEMO_OUTPUT_DIR}"
)

add_custom_command(
    TARGET telemetry_demo POST_BUILD
    COMMAND ${AVR_OBJCOPY} -O ihex -R .eeprom
            "$<TARGET_FILE:telemetry_demo>"
            "${TELEMETRY_DEMO_OUTPUT_DIR}/telemetry_demo.hex"
    COMMAND ${AVR_OBJCOPY} -O ihex
            --only-section=.fuse
            --change-section-lma .fuse=0
            --no-change-warnings
            "$<TARGET_FILE:telemetry_demo>"
            "${TELEMETRY_DEMO_OUTPUT_DIR}/telemetry_demo.fuses"
    COMMAND ${AVR_OBJDUMP}
            --disassemble
            --wide
            --demangle
            --line-numbers
            --section-headers
            --source
            "$<TARGET_FILE:telemetry_demo>"
            > "${TELEMETRY_DEMO_OUTPUT_DIR}/telemetry_demo.lss"
    COMMAND "${CMAKE_CURRENT_LIST_DIR}/../../scripts/report_avr_memory.sh"
            "$<TARGET_FILE:telemetry_demo>"
            32768
            3072
            "${AVR_READELF}"
            "${AVR_NM}"
            "${AVR_OBJDUMP}"
    VERBATIM
)
