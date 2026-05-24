#!/usr/bin/env bash
set -euo pipefail

# Print a compact flash/SRAM usage report for an AVR ELF.
if [[ $# -lt 6 ]]; then
    echo "usage: $0 <elf> <flash-bytes> <sram-bytes> <avr-readelf> <avr-nm> <avr-objdump>" >&2
    exit 2
fi

# Positional inputs come from CMake so the script does not depend on PATH.
elf_path=$1
flash_total=$2
sram_total=$3
avr_readelf_bin=$4
avr_nm_bin=$5
avr_objdump_bin=$6

# Cache section headers once; section_hex_size parses this text repeatedly.
readelf_output="$("$avr_readelf_bin" -W -S "$elf_path")"

section_hex_size() {
    # Return a section size as a hex string, or 0 if the section is absent.
    local section_name=$1
    local size_hex

    size_hex=$(
        printf '%s\n' "$readelf_output" |
            awk -v section_name="$section_name" '
                $1 == "[" && $3 == section_name {
                    print $7
                    exit
                }
            '
    )

    if [[ -z "${size_hex}" ]]; then
        printf '0'
        return
    fi

    printf '%s' "$size_hex"
}

hex_to_dec() {
    # Bash arithmetic can convert hexadecimal strings with the 16# prefix.
    local size_hex=$1
    printf '%d' "$((16#$size_hex))"
}

# Flash usage includes loadable program bytes; SRAM usage includes initialized,
# zeroed, and no-init RAM sections.
text_size=$(hex_to_dec "$(section_hex_size .text)")
rodata_size=$(hex_to_dec "$(section_hex_size .rodata)")
data_size=$(hex_to_dec "$(section_hex_size .data)")
bss_size=$(hex_to_dec "$(section_hex_size .bss)")
noinit_size=$(hex_to_dec "$(section_hex_size .noinit)")

flash_used=$((text_size + rodata_size + data_size))
sram_used=$((data_size + bss_size + noinit_size))
flash_free=$((flash_total - flash_used))
sram_free=$((sram_total - sram_used))
flash_pct=$((flash_used * 100 / flash_total))
sram_pct=$((sram_used * 100 / sram_total))

# High-level memory summary.
printf 'Memory usage for %s\n' "$(basename "$elf_path")"
printf '  Flash: %5d / %5d bytes (%3d%%), %5d bytes free\n' \
    "$flash_used" "$flash_total" "$flash_pct" "$flash_free"
printf '    .text=%d .rodata=%d .data=%d\n' \
    "$text_size" "$rodata_size" "$data_size"
printf '  SRAM:  %5d / %5d bytes (%3d%%), %5d bytes free\n' \
    "$sram_used" "$sram_total" "$sram_pct" "$sram_free"
printf '    .data=%d .bss=%d .noinit=%d\n' \
    "$data_size" "$bss_size" "$noinit_size"

printf 'Top functions\n'
"$avr_objdump_bin" -t -C "$elf_path" |
    awk '
        # Function symbols in .text include both user functions and ISRs.
        $3 == "F" && $4 == ".text" {
            size = strtonum("0x" $5)
            symbol = substr($0, index($0, $6))
            if (size > 0 && symbol !~ /^(__do_clear_bss|__call_main|__init_sp|_exit|exit)$/) {
                print size "\t" symbol
            }
        }
    ' |
    sort -n |
    tail -n 8 |
    awk -F '\t' '{ printf("  %5d  %s\n", $1, $2) }'

printf 'Top SRAM symbols\n'
"$avr_nm_bin" -S --size-sort --radix=d --demangle --print-size "$elf_path" |
    awk -v sram_total="$sram_total" '
        # Keep only BSS/data-like symbols whose addresses live in AVR SRAM.
        # This filters out special memory sections such as .fuse.
        $3 ~ /^[BbDdVv]$/ {
            address = $1 + 0
            sram_start = 8388608
            sram_end = sram_start + sram_total
            if (address < sram_start || address >= sram_end) {
                next
            }
            symbol = substr($0, index($0, $4))
            if (symbol !~ /^(__bss_|__data_|_end$|__heap_|__stack$)/) {
                print $2 "\t" $3 "\t" symbol
            }
        }
    ' |
    sort -n |
    tail -n 5 |
    awk -F '\t' '{ printf("  %5d  %s  %s\n", $1, $2, $3) }'
