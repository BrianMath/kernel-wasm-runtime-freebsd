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
sudo ./wasm_manager load wasm_probes/mkdir_mode.wasm > /dev/null 2>&1

sudo ./wasm_manager list > /dev/null 2>&1

mkdir -p /tmp/wasm_test
sudo ./wasm_manager report 1 > /tmp/wasm_reports/report_output.log
actual_output=$(tail -n 1 /tmp/wasm_reports/report_output.log)
expected_output="Report: The mode of the directory created is 511"

check_output "$expected_output" "$actual_output"


# clean up
sudo rmmod kernel_wasm
rm -rf /tmp/wasm_reports
rm -rf /tmp/wasm_test

echo
echo "====================="
echo -e "${GREEN}Tests Passed: $passed${NC}"
echo -e "${RED}Tests Failed: $failed${NC}"
echo "====================="

# Exit with failure if any test failed
if [[ $failed -ne 0 ]]; then
    exit 1
fi
