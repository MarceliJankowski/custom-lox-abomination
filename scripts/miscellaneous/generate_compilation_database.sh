#!/usr/bin/env bash

source "$(dirname "$0")/../common.sh"

set -o errexit

log_action "Cleaning all cla builds"
make clean 1>/dev/null

log_action "Generating compile_commands.json (makes all cla builds)"
bear -- make all 1>/dev/null

exit 0
