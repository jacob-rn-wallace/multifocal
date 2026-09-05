#!/bin/sh
# Runs sto_rcl_test for 3 independent value/register combinations, one
# fresh process per trial (deliberate - see sto_rcl_test.c's header
# comment on why nut_boot_cx() makes looping trials in one process
# unsafe). Exits nonzero if any trial fails.
set -e
cd "$(dirname "$0")"
fails=0
./build/sto_rcl_test 42 07 || fails=$((fails+1))
./build/sto_rcl_test 99 12 || fails=$((fails+1))
./build/sto_rcl_test 5 03 || fails=$((fails+1))
if [ "$fails" -eq 0 ]; then
    echo "PASS: real STO/RCL round-trip correctly across all 3 trials."
    exit 0
else
    echo "FAIL: $fails of 3 trials mismatched."
    exit 1
fi
