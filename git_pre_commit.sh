#!/usr/bin/env bash

set -o nounset
set -o pipefail
set -o errexit

echo "Making all cla builds..."
make all 1>/dev/null

echo "Testing release build..."
./run_tests.sh
