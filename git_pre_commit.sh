#!/usr/bin/env bash

set -o nounset
set -o pipefail
set -o errexit

echo "Building all clox builds..."
make all 1>/dev/null

echo "Testing release build..."
./run_tests.sh
