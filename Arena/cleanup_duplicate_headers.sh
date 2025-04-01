#!/bin/bash

echo "Scanning for duplicate headers between src/ and include/ directories..."
echo ""

# Create temporary files for listing headers
src_headers=$(mktemp)
include_headers=$(mktemp)

# Find all .hpp files in the src directory and subdirectories
find src -name "*.hpp" > "$src_headers"
# Find all .hpp files in the include directory
find include -name "*.hpp" > "$include_headers"

# Loop through each .hpp file in src directory
echo "Duplicate headers found:"
echo "========================"
duplicates_found=false

while read src_file; do
  # Get the base name (without path)
  base_name=$(basename "$src_file")
  
  # Check if there's a file with the same name in include
  grep -q "$base_name" "$include_headers"
  if [ $? -eq 0 ]; then
    # Find matching include file paths
    include_file=$(grep "/$base_name$" "$include_headers")
    
    echo "DUPLICATE: $src_file"
    echo "         & $include_file"
    echo ""
    duplicates_found=true
  fi
done < "$src_headers"

if [ "$duplicates_found" = false ]; then
  echo "No duplicates found."
fi

echo ""
echo "IMPORTANT: Before deleting any files, check that they are truly duplicates"
echo "and not just files with the same name but different content."
echo ""
echo "To review the differences between duplicate files, use:"
echo "diff -u <include_file> <src_file>"
echo ""
echo "To safely remove a duplicate header after verification, use:"
echo "git rm <src_file>"
echo ""

# Clean up temporary files
rm "$src_headers" "$include_headers" 