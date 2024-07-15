#!/usr/bin/env bash

set -o nounset
set -o pipefail

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly TRUE=0
readonly FALSE=1

readonly GENERIC_ERROR_CODE=1
readonly INTERNAL_ERROR_CODE=255

readonly REPO_ROOT_DIR=$(git rev-parse --show-toplevel || exit $GENERIC_ERROR_CODE)
readonly SCRIPTS_DIR=${REPO_ROOT_DIR}/scripts

##################################################
#                   UTILITIES                    #
##################################################

# @desc log internal error `message` to stderr and exit with INTERNAL_ERROR_CODE
throw_internal_error() {
  local message="$1"

  [[ $# -ne 1 ]] && message="throw_internal_error() expects 'message' argument"

  echo -e "[INTERNAL_ERROR] - $message" 1>&2

  exit $INTERNAL_ERROR_CODE
}

# @desc log error `message` to stderr and exit with `exit_code`
throw_error() {
  [[ $# -ne 2 ]] && throw_internal_error "throw_error() expects 'message' and 'exit_code' arguments"

  local -r message="$1"
  local -r exit_code="$2"

  echo -e "[ERROR] - $message" 1>&2
  exit $exit_code
}

# @desc log action `message` to stdout
log_action() {
  [[ $# -ne 1 ]] && throw_internal_error "log_action() expects 'message' argument"

  local -r message="$1"

  echo -e "[ACTION] - ${message}..."
}

##################################################
#                 MISCELLANEOUS                  #
##################################################

# validate path constants
[[ ! -d "$REPO_ROOT_DIR" ]] && throw_internal_error "REPO_ROOT_DIR '${REPO_ROOT_DIR}' is not a directory"
[[ ! -d "$SCRIPTS_DIR" ]] && throw_internal_error "SCRIPTS_DIR '${SCRIPTS_DIR}' is not a directory"

cd "$REPO_ROOT_DIR" ||
  throw_error "Failed to navigate into '$REPO_ROOT_DIR'" $GENERIC_ERROR_CODE
