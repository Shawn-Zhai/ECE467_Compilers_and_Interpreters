#!/bin/bash

# Define paths
INPUT_FILE="test.sc"
LOCAL_DIR="output/local"
REF_DIR="output/ref"
LLVM_DIS="/cad2/ece467f/llvm-project/build/bin/llvm-dis"
A4GEN_LOCAL="A4Gen"
A4GEN_REF="/cad2/ece467f/public/A4GenRef"

# Step 1: Copy the input file to both local and reference directories
cp "$INPUT_FILE" "$LOCAL_DIR/"
cp "$INPUT_FILE" "$REF_DIR/"

# Step 2: Run A4Gen on the local test file
$A4GEN_LOCAL "$LOCAL_DIR/$INPUT_FILE"

# Step 3: Generate the disassembly for the local .bc file
$LLVM_DIS "$LOCAL_DIR/test.bc"

# Step 4: Run the reference A4Gen on the reference test file
$A4GEN_REF "$REF_DIR/$INPUT_FILE"

# Step 5: Generate the disassembly for the reference .bc file
$LLVM_DIS "$REF_DIR/test.bc"

echo "All tasks completed successfully!"