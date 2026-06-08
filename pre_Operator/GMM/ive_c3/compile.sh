#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE="${SCRIPT_DIR}/merged_noopencv.cpp"
OUTPUT="${SCRIPT_DIR}/merged_noopencv"

echo "Compiling ${SOURCE}..."

g++ -o "${OUTPUT}" "${SOURCE}" \
    -O2 \
    -Wall \
    -std=c++11 \
    -lm \
    -lz

echo "Build successful: ${OUTPUT}"
