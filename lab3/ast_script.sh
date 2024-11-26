#!/bin/bash

# Create an output directory if it doesn't exist
mkdir -p output

# Define the output file paths
local_output="output/local.txt"
reference_output="output/reference.txt"

# Run the commands and save their outputs
A3Sema test.c > "$local_output"
# if [ $? -ne 0 ]; then
#   echo "Error: A3Sema command failed."
#   exit 1
# fi

/cad2/ece467f/public/A3SemaRefPrint test.c > "$reference_output"
# if [ $? -ne 0 ]; then
#   echo "Error: A3SemaRefPrint command failed."
#   exit 1
# fi

# Compare the output files
diff "$local_output" "$reference_output" > output/diff_output.txt

# Check if there were differences
if [ $? -eq 0 ]; then
  echo "The outputs are identical."
else
  echo "The outputs differ. See output/diff_output.txt for details."
fi