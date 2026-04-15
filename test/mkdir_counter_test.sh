#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

passed=0
failed=0

check_output() {
    expected_output="$1"
    actual_output="$2"

    if [[ "$actual_output" == "$expected_output" ]]; then
        echo -e "${GREEN}Test Passed!${NC}"
        ((passed++))
    else
        echo -e "${RED}Test Failed!${NC}"
        echo "Expected: $expected_output"
        echo "Actual:   $actual_output"
        ((failed++))
    fi
}

make > /dev/null 2>&1

if [ $? -ne 0 ]; then
  echo -e "${RED}Could not Compile the kernel module${NC}"
  exit 1
fi

# load the kernel module
sudo insmod kernel_wasm.ko

if [ $? -ne 0 ]; then
  echo -e "${RED}Failed to load kernel module.${NC}"
  exit 1
fi

# create directories to observe the report output
mkdir -p /tmp/wasm_reports

# load the WASM binary
sudo ./wasm_manager load wasm_probes/mkdir_counter.wasm > /dev/null 2>&1

# call the report function
# Assuming wasm_manager is the interface for triggering reports
sudo ./wasm_manager report 1 > /tmp/wasm_reports/report_output.log

actual_output=$(tail -n 1 /tmp/wasm_reports/report_output.log)

expected_output="Report: mkdir was called 0 times"

check_output "$expected_output" "$actual_output"

mkdir /tmp/wasm_test1
mkdir /tmp/wasm_test2

# There might be a race condition somewhere here, rerun or fix hmm
sleep 1

sudo ./wasm_manager report 1 > /tmp/wasm_reports/report_output.log
actual_output=$(tail -n 1 /tmp/wasm_reports/report_output.log)
expected_output="Report: mkdir was called 2 times"

check_output "$expected_output" "$actual_output"

# clean up
sudo rmmod kernel_wasm
rm -rf /tmp/wasm_reports
rm -rf /tmp/wasm_test1
rm -rf /tmp/wasm_test2

echo
echo "====================="
echo -e "${GREEN}Tests Passed: $passed${NC}"
echo -e "${RED}Tests Failed: $failed${NC}"
echo "====================="

# Exit with failure if any test failed
if [[ $failed -ne 0 ]]; then
    exit 1
fi