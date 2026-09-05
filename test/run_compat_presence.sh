#!/bin/sh
# Runs compat_presence_test twice (module present, module absent) and
# diffs stdout - see compat_presence_test.c's header comment. Exits
# nonzero if the two runs differ, printing the diff.
set -e
cd "$(dirname "$0")"
./build/compat_presence_test with    >build/compat_out_with.txt    2>build/compat_err_with.txt
./build/compat_presence_test without >build/compat_out_without.txt 2>build/compat_err_without.txt
if diff -u build/compat_out_with.txt build/compat_out_without.txt; then
    echo "PASS: identical native-operation output with MultiFOCAL's module present vs. absent."
    exit 0
else
    echo "FAIL: native-operation output differs depending on whether MultiFOCAL's module is present."
    exit 1
fi
