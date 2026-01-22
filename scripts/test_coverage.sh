#!/bin/sh
#
# This file is mostly AI generated.
set -euo pipefail
set -x

# Absolute paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."
COVERAGE_DIR="$ROOT_DIR/coverage"

# Clean and prepare coverage dir
rm -rf "$COVERAGE_DIR"
mkdir -p "$COVERAGE_DIR"

# Copy all relevant files to coverage directory
cp "$ROOT_DIR/ls_args.h" "$COVERAGE_DIR/"
mkdir -p "$COVERAGE_DIR/tests"
cp "$ROOT_DIR/tests/tests.c" "$COVERAGE_DIR/tests/"
cp "$ROOT_DIR/tests/ls_test.h" "$COVERAGE_DIR/tests/"
cp "$SCRIPT_DIR/Makefile-cov" "$COVERAGE_DIR/Makefile"

cd "$COVERAGE_DIR"

export CFLAGS="-fprofile-arcs -ftest-coverage -DNDEBUG"
# Build test binary with coverage instrumentation
# Also disable asserts because they do not count for us
make -B

# Run test binary (generates .gcda files here)
./tests_cov_tmp

# Generate coverage reports (all output local to coverage/)
gcovr --root . --object-directory . --exclude-directories tests --output coverage.txt
gcovr --root . --object-directory . --exclude-directories tests --html --html-details -o coverage.html

echo "Coverage summary: $COVERAGE_DIR/coverage.txt"
echo "HTML report: $COVERAGE_DIR/coverage.html"
