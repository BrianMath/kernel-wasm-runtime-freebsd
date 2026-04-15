#!/bin/bash
# test/test_connect_counter.sh
set -euo pipefail

# ---- Colors ----
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

passed=0
failed=0

# ---- Config ----
SERVER_PORT=${SERVER_PORT:-9000}
PROXY_PORT=${PROXY_PORT:-8080}
LOOPS=${LOOPS:-5}
SLEEP_AFTER_TRAFFIC=${SLEEP_AFTER_TRAFFIC:-0.5}

PROXY_BIN="${PROXY_BIN:-./user_test/toyproxy}"
NC_BIN="${NC_BIN:-nc}"

KMOD_PATH="${KMOD_PATH:-./kernel_wasm.ko}"
KMOD_NAME="${KMOD_NAME:-kernel_wasm}"
MANAGER="${MANAGER:-./wasm_manager}"
WASM_PATH="${WASM_PATH:-./wasm_probes/connect_counter.wasm}"
WASM_ID="${WASM_ID:-1}"   # you said this is always 1

TMPDIR="/tmp/wasm_connect_test"
REPORT_LOG="$TMPDIR/report_output.log"

# ---- Helpers ----
check_output() {
    local expected_output="$1"
    local actual_output="$2"
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

is_loaded() {
  lsmod | awk '{print $1}' | grep -qx "$KMOD_NAME"
}

ensure_module_loaded() {
  if ! is_loaded; then
    sudo insmod "$KMOD_PATH"
  fi
}

unload_module_if_loaded() {
  if is_loaded; then
    sudo rmmod "$KMOD_NAME" || true
  fi
}

reset_module_and_load_wasm() {
  # clean state for counter
  unload_module_if_loaded
  ensure_module_loaded
  # load the WASM probe
  sudo "$MANAGER" load "$WASM_PATH" >/dev/null 2>&1
}

pids=()
stop_background() {
  for pid in "${pids[@]:-}"; do
    if kill -0 "$pid" 2>/dev/null; then
      kill "$pid" 2>/dev/null || true
      sleep 0.15
      kill -9 "$pid" 2>/dev/null || true
    fi
  done
  pids=()
}

cleanup() {
  stop_background
  unload_module_if_loaded
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

wait_listen() {
  local port="$1" tries=30
  while ! ss -ltn 2>/dev/null | awk '{print $4}' | grep -q ":$port$"; do
    sleep 0.1
    ((tries--)) || { echo "Timeout waiting on :$port"; return 1; }
  done
}

start_server() {
  $NC_BIN -lk "$SERVER_PORT" >/dev/null 2>&1 &
  pids+=($!)
  wait_listen "$SERVER_PORT"
}

start_proxy() {
  local mode="$1"  # enable | disable
  "$PROXY_BIN" "$mode" >/dev/null 2>&1 &
  pids+=($!)
  wait_listen "$PROXY_PORT"
}

client_once() {
  printf "hello-from-client\n" | $NC_BIN 127.0.0.1 "$PROXY_PORT" >/dev/null 2>&1 || true
  sleep 0.05
}

report_line() {
  mkdir -p "$TMPDIR"
  sudo "$MANAGER" report "$WASM_ID" > "$REPORT_LOG"
  tail -n 1 "$REPORT_LOG"
}

run_scenario() {
  local mode="$1" loops="$2"

  # fresh load & counter
  reset_module_and_load_wasm

  # start server and proxy, ensure they are listening
  start_server
  start_proxy "$mode"

  # N client connects
  for ((i=1;i<=loops;i++)); do
    client_once
  done

  # allow proxy to complete backend connect(s)
  sleep "$SLEEP_AFTER_TRAFFIC"

  # report BEFORE teardown to avoid races
  local actual_output
  actual_output="$(report_line)"

  # now teardown
  stop_background

  # expected line (exact)
  local expect_count
  if [[ "$mode" == "enable" ]]; then
    expect_count=1
  else
    expect_count=$loops
  fi
  local expected_output="Report: connect was called ${expect_count} times"

  check_output "$expected_output" "$actual_output"
}

# ---- Build (quiet) ----
make > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
  echo -e "${RED}Could not Compile the kernel module${NC}"
  exit 1
fi

# ---- Run tests ----
run_scenario enable "$LOOPS"
run_scenario disable "$LOOPS"

# ---- Summary ----
echo
echo "====================="
echo -e "${GREEN}Tests Passed: $passed${NC}"
echo -e "${RED}Tests Failed: $failed${NC}"
echo "====================="

# exit code reflects failures
[[ $failed -ne 0 ]] && exit 1 || exit 0
