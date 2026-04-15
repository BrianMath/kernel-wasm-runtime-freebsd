#!/bin/bash
set -euo pipefail

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Config
LTP_TEST="mkdir02"
LTP_BIN="/opt/ltp/testcases/bin/$LTP_TEST"
RESULT_DIR="/tmp/trace_compare"
mkdir -p "$RESULT_DIR"

sudo bpftrace -o "$RESULT_DIR/bpftrace_output.log" bpf_probes/mkdir_counter.bt &
BPFTRACE_PID=$!
sleep 1

sudo "$LTP_BIN" > "$RESULT_DIR/ltp_output.log" 2>&1 || true

sudo kill -TERM "$BPFTRACE_PID"
wait "$BPFTRACE_PID" || true

sudo insmod kernel_wasm.ko || true
sleep 1
sudo ./wasm_manager load wasm_probes/mkdir_counter.wasm > /dev/null 2>&1
sleep 1

sudo "$LTP_BIN" > "$RESULT_DIR/ltp_output.log" 2>&1 || true

sudo ./wasm_manager report 1 > "$RESULT_DIR/wasm_output.log"
grep -Eo '[0-9]+' "$RESULT_DIR/wasm_output.log" | tail -1 > "$RESULT_DIR/wasm_count.txt"
grep -Eo '[0-9]+' "$RESULT_DIR/bpftrace_output.log" | tail -1 > "$RESULT_DIR/bpf_count.txt"

sudo rmmod kernel_wasm

BPFTRACE_COUNT=$(cat "$RESULT_DIR/bpf_count.txt")

WASM_COUNT=$(cat "$RESULT_DIR/wasm_count.txt")

if [ "$BPFTRACE_COUNT" -eq "$WASM_COUNT" ]; then
  echo -e "${GREEN}Test Passed ${NC}"
  passed=1
else
  echo -e "${RED}Test Failed${NC}"
  passed=0
fi

rm -rf "$RESULT_DIR"

# Exit with failure if any test failed
if [[ "$passed" -eq 0 ]]; then
    exit 1
fi