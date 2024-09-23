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

WORKING_TREE_ROOT_DIR=$(git rev-parse --show-toplevel) || exit $GENERIC_ERROR_CODE
readonly WORKING_TREE_ROOT_DIR
readonly SCRIPTS_DIR=${WORKING_TREE_ROOT_DIR}/scripts

##################################################
#                   UTILITIES                    #
##################################################

# @desc log internal error `message` to stderr and exit with INTERNAL_ERROR_CODE
internal_error() {
  local message="$1"

  [[ $# -ne 1 ]] && message="internal_error() expects 'message' argument"

  echo -e "[INTERNAL_ERROR] - $message" 1>&2

  exit $INTERNAL_ERROR_CODE
}

# @desc log error `message` to stderr and exit with `exit_code`
error() {
  [[ $# -ne 2 ]] && internal_error "error() expects 'message' and 'exit_code' arguments"

  local -r message="$1"
  local -r exit_code="$2"

  echo -e "[ERROR] - $message" 1>&2

  exit $exit_code
}

# @desc log warning `message` to stdout
warning() {
  [[ $# -ne 1 ]] && internal_error "warning() expects 'message' argument"

  local -r message="$1"

  echo -e "[WARNING] - $message"
}

# @desc log action `message` to stdout
log_action() {
  [[ $# -ne 1 ]] && internal_error "log_action() expects 'message' argument"

  local -r message="$1"

  echo -e "[ACTION] - ${message}..."
}

##################################################
#                 MISCELLANEOUS                  #
##################################################

# validate path constants
[[ ! -d "$WORKING_TREE_ROOT_DIR" ]] && internal_error "WORKING_TREE_ROOT_DIR '${WORKING_TREE_ROOT_DIR}' is not a directory"
[[ ! -d "$SCRIPTS_DIR" ]] && internal_error "SCRIPTS_DIR '${SCRIPTS_DIR}' is not a directory"

cd "$WORKING_TREE_ROOT_DIR" ||
  error "Failed to navigate into '$WORKING_TREE_ROOT_DIR'" $GENERIC_ERROR_CODE
