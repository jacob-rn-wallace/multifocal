#!/bin/sh
# Runs compat_native_program_test twice (module present, module
# absent) and diffs stdout - see that file's header comment. Exits
# nonzero if the two runs differ, printing the diff.
set -e
cd "$(dirname "$0")"
./build/compat_native_program_test with    >build/compat_native_out_with.txt    2>build/compat_native_err_with.txt
./build/compat_native_program_test without >build/compat_native_out_without.txt 2>build/compat_native_err_without.txt
if diff -u build/compat_native_out_with.txt build/compat_native_out_without.txt; then
    echo "PASS: identical real stored-program output with MultiFOCAL's module present vs. absent."
    exit 0
else
    echo "FAIL: stored-program output differs depending on whether MultiFOCAL's module is present."
    exit 1
fi
