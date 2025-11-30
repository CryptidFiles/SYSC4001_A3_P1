#!/usr/bin/env bash

INPUT_DIR="input_files"
OUTPUT_DIR="output_files"

mkdir -p "$OUTPUT_DIR"

for i in $(seq 1 20); do
  echo "============================="
  echo "Running Scenario $i"
  echo "============================="

  ./bin/interrupts_EP "$INPUT_DIR/scenario$i.txt"
  mv execution.txt "$OUTPUT_DIR/EP_scenario$i.txt"

  ./bin/interrupts_RR "$INPUT_DIR/scenario$i.txt"
  mv execution.txt "$OUTPUT_DIR/RR_scenario$i.txt"

  ./bin/interrupts_EP_RR "$INPUT_DIR/scenario$i.txt"
  mv execution.txt "$OUTPUT_DIR/EPRR_scenario$i.txt"
done
