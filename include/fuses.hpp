#pragma once

#include <avr/io.h>
#include <stdint.h>

// Build-time fuse policy. src/fuses.cpp emits these values into the ELF .fuse
// section so the fuse image is controlled by source code.
namespace fuses {

// Reserved fuse bytes should remain unprogrammed.
constexpr uint8_t reserved = 0xffu;

// Device-header defaults that match the current firmware assumptions.
constexpr uint8_t wdtcfg = FUSE_WDTCFG_DEFAULT;
constexpr uint8_t bodcfg = FUSE_BODCFG_DEFAULT;
constexpr uint8_t osccfg = FUSE_OSCCFG_DEFAULT;
constexpr uint8_t tcd0cfg = FUSE_TCD0CFG_DEFAULT;
constexpr uint8_t syscfg0 = FUSE_SYSCFG0_DEFAULT;
constexpr uint8_t syscfg1 = FUSE_SYSCFG1_DEFAULT;
constexpr uint8_t append = FUSE_APPEND_DEFAULT;
constexpr uint8_t bootend = FUSE_BOOTEND_DEFAULT;

}  // namespace fuses
