#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
host_cc=${HOST_CC:-cc}
if [ -n "${UMPS3_INCLUDE_DIR:-}" ]; then
    umps3_include_dir=$UMPS3_INCLUDE_DIR
elif [ -f /usr/include/umps3/umps/libumps.h ]; then
    umps3_include_dir=/usr/include/umps3
else
    umps3_include_dir=/usr/local/include/umps3
fi

if [ ! -f "$umps3_include_dir/umps/libumps.h" ]; then
    echo "uMPS3 headers not found at $umps3_include_dir/umps/libumps.h" >&2
    echo "Set UMPS3_INCLUDE_DIR to the include prefix containing umps/." >&2
    exit 1
fi

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/so2024-tests.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

common_flags="-std=gnu11 -Wall -Wextra -Werror -I$repo_dir -I$umps3_include_dir -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast"

# The kernel ABI uses 32-bit pointer arguments. A low pthread stack keeps the
# host-side SSI request pointers representable while exercising the real code.
"$host_cc" $common_flags -pthread \
    "$script_dir/print_length_test.c" \
    "$repo_dir/phase3/utils_phase3.c" \
    "$repo_dir/phase3/sst_utils.c" \
    -o "$build_dir/print_length_test"

"$host_cc" $common_flags \
    "$script_dir/softblock_termination_test.c" \
    "$repo_dir/phase1/pcb.c" \
    "$repo_dir/phase2/utils.c" \
    "$repo_dir/phase2/SSI.c" \
    -o "$build_dir/softblock_termination_test"

"$host_cc" $common_flags \
    "$script_dir/startup_messages_test.c" \
    "$repo_dir/phase3/vmSupport.c" \
    "$repo_dir/phase3/initProc.c" \
    -o "$build_dir/startup_messages_test"

"$host_cc" $common_flags -c \
    "$repo_dir/phase3/sysSupport.c" \
    -o "$build_dir/sysSupport.o"

"$host_cc" $common_flags -ffunction-sections -fdata-sections -pthread \
    "$script_dir/abnormal_completion_test.c" \
    "$repo_dir/phase3/sysSupport.c" \
    "$repo_dir/phase3/sst_utils.c" \
    -Wl,--gc-sections \
    -o "$build_dir/abnormal_completion_test"

"$build_dir/print_length_test"
"$build_dir/softblock_termination_test"
"$build_dir/startup_messages_test"
"$build_dir/abnormal_completion_test"
