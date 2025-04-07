#!/bin/bash

# Define the specific subdirectories to search within
DIR1="DigipenEngine/7percent"
DIR2="DigipenEngine/Assets/Scripts"
DIR3="DigipenEngine/EngineScripting"

# Define the file extensions to include
EXTS="c,h,cpp,ipp,cs"

# Define the output file
OUTPUT_FILE="count.txt"

# Print the current system time
echo "LOC Updated as of: $(date '+%d/%m/%y %H:%M:%S')" > "$OUTPUT_FILE"

# Start the output file with a header
{
  echo "Cloc Analysis Results"
  echo "====================="
  echo ""
} >> "$OUTPUT_FILE"

# Function to analyze a directory
analyze_dir() {
  local DIR="$1"
  if [ -d "$DIR" ]; then
    echo "Analyzing $DIR:" >> "$OUTPUT_FILE"
    cloc "$DIR" --include-ext="$EXTS" --by-file | grep -Pv "(http|SUM:|File |Counting:)" | sort >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
  else
    echo "Skipping $DIR (Directory not found)" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
  fi
}

# Analyze each directory
analyze_dir "$DIR1"
analyze_dir "$DIR2"
analyze_dir "$DIR3"

# Notify completion
echo "Analysis complete. Results are in $OUTPUT_FILE"
