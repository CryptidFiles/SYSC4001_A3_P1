#!/usr/bin/env bash

INPUT_DIR="input_files"
OUTPUT_DIR="output_files"

mkdir -p "$OUTPUT_DIR"

for i in $(seq 1 20); do
  SCENARIO_FILE="$INPUT_DIR/scenario$i.txt"

  echo "============================="
  echo "Running Scenario $i"
  echo "============================="

  if [ ! -f "$SCENARIO_FILE" ]; then
    echo "  -> SKIPPING: $SCENARIO_FILE not found."
    continue
  fi

  # ----- EP -----
  echo "  [EP]    running on scenario $i..."
  ./bin/interrupts_EP "$SCENARIO_FILE"

  EP_SRC_FILE=$(ls execution.txt* 2>/dev/null | head -n 1)
  if [ -n "$EP_SRC_FILE" ]; then
    mv "$EP_SRC_FILE" "$OUTPUT_DIR/EP_scenario$i.txt"
    echo "    -> Saved EP_scenario$i.txt (from $EP_SRC_FILE)"
  else
    echo "    !! EP did not produce any execution.txt* file"
  fi

  # ----- RR -----
  echo "  [RR]    running on scenario $i..."
  ./bin/interrupts_RR "$SCENARIO_FILE"

  RR_SRC_FILE=$(ls execution.txt* 2>/dev/null | head -n 1)
  if [ -n "$RR_SRC_FILE" ]; then
    mv "$RR_SRC_FILE" "$OUTPUT_DIR/RR_scenario$i.txt"
    echo "    -> Saved RR_scenario$i.txt (from $RR_SRC_FILE)"
  else
    echo "    !! RR did not produce any execution.txt* file"
  fi

  # ----- EP+RR -----
  echo "  [EP+RR] running on scenario $i..."
  ./bin/interrupts_EP_RR "$SCENARIO_FILE"

  EPRR_SRC_FILE=$(ls execution.txt* 2>/dev/null | head -n 1)
  if [ -n "$EPRR_SRC_FILE" ]; then
    mv "$EPRR_SRC_FILE" "$OUTPUT_DIR/EPRR_scenario$i.txt"
    echo "    -> Saved EPRR_scenario$i.txt (from $EPRR_SRC_FILE)"
  else
    echo "    !! EP+RR did not produce any execution.txt* file"
  fi

done
