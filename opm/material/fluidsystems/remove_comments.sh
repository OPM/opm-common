#!/bin/bash

# Usage: ./remove_comments.sh <source_file.cpp> <destination_file.cpp>

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <source_file.cpp> <destination_file.cpp>"
    exit 1
fi

input_file="$1"
output_file="$2"

sed '/\/\*/!b;:a;/\*\//!{N;ba};s/\/\*.*\*\///' "$input_file" | sed 's/\/\/.*//' > "$output_file"

echo "Comments removed from $input_file and output to $output_file"
