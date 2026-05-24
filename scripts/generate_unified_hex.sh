#!/usr/bin/env bash
set -euo pipefail

# Merge a primary program HEX and optional secondary HEX. This wrapper is
# defensive because hexmate may report conflicts when generated inputs overlap.
primary_hex=${1:?primary hex required}
secondary_hex=${2:-}
output_hex=${3:?output hex required}
hexmate_bin=${4:-}

# The primary image is mandatory; without it there is nothing useful to emit.
if [[ ! -f "$primary_hex" ]]; then
    echo "primary hex not found: $primary_hex" >&2
    exit 1
fi

# Ensure the output directory exists before copying or merging.
mkdir -p "$(dirname "$output_hex")"

copy_primary() {
    # Fallback path used when no safe merge is possible.
    cp "$primary_hex" "$output_hex"
    echo "Unified HEX: copied primary image to $output_hex"
}

# If no secondary image exists, the unified output is just the primary image.
if [[ -z "$secondary_hex" || ! -f "$secondary_hex" ]]; then
    copy_primary
    exit 0
fi

# Resolve paths so self-merges can be detected reliably.
primary_real=$(realpath "$primary_hex")
secondary_real=$(realpath "$secondary_hex")
output_real=$(realpath -m "$output_hex")

# Avoid using the same file as both input and output.
if [[ "$primary_real" == "$secondary_real" || "$secondary_real" == "$output_real" ]]; then
    copy_primary
    exit 0
fi

# hexmate is optional; fall back to the primary image when unavailable.
if [[ -z "$hexmate_bin" || ! -x "$hexmate_bin" ]]; then
    echo "Unified HEX: hexmate unavailable, copying primary image" >&2
    copy_primary
    exit 0
fi

# Merge into a temporary file first so a failed merge cannot corrupt the output.
tmp_output=$(mktemp "${output_hex}.tmp.XXXXXX")
log_output=$(mktemp "${output_hex}.log.XXXXXX")
cleanup() {
    # Remove temporary merge outputs on success or failure.
    rm -f "$tmp_output" "$log_output"
}
trap cleanup EXIT

# Successful hexmate run becomes the unified output.
if "$hexmate_bin" "$primary_hex" "$secondary_hex" -O"$tmp_output" >"$log_output" 2>&1; then
    mv "$tmp_output" "$output_hex"
    echo "Unified HEX: merged $primary_hex and $secondary_hex into $output_hex"
    exit 0
fi

# If hexmate rejects the merge, keep the primary image usable and show the log.
echo "Unified HEX: secondary image conflicts with primary, copying primary image instead" >&2
cat "$log_output" >&2
copy_primary
