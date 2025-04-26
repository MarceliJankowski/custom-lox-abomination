#!/usr/bin/env bash

source "$(dirname "$0")/../common.sh"

set -o errexit

log_action "Checking required software availability and version conformance"
if ! is_cmd_available "bear"; then
  error "bear (tool for generating compilation database) is unavailable" $GENERIC_ERROR_CODE
elif [[ $(bear --version) != "bear 3."* ]]; then
  error "bear version doesn't conform to required '^3.0.0'" $GENERIC_ERROR_CODE
fi

log_action "Cleaning all cla builds"
make clean 1>/dev/null

log_action "Generating compile_commands.json (makes all cla builds)"
bear -- make all 1>/dev/null

exit 0
