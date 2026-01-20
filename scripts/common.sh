#!/usr/bin/env bash

set -o nounset
set -o pipefail

##################################################
#                   UTILITIES                    #
##################################################

# Log internal error `message` to stderr and exit with INTERNAL_ERROR_CODE.
internal_error() {
  local message="$1"

  [[ $# -ne 1 ]] && message="internal_error() expects 'message' argument"

  echo -e "[INTERNAL_ERROR] - $message" 1>&2

  exit $INTERNAL_ERROR_CODE
}

# Log error `message` to stderr and exit with `exit_code`.
error() {
  [[ $# -ne 2 ]] && internal_error "error() expects 'message' and 'exit_code' arguments"

  local -r message="$1"
  local -r exit_code="$2"

  echo -e "[ERROR] - $message" 1>&2

  exit $exit_code
}

# Log warning `message` to stdout.
warning() {
  [[ $# -ne 1 ]] && internal_error "warning() expects 'message' argument"

  local -r message="$1"

  echo -e "[WARNING] - $message"
}

# Log action `message` to stdout.
log_action() {
  [[ $# -ne 1 ]] && internal_error "log_action() expects 'message' argument"

  local -r message="$1"

  echo -e "[ACTION] - ${message}..."
}

# Determine whether `cmd` is available in PATH.
# @return TRUE if it is, FALSE otherwise.
is_cmd_available() {
  [[ $# -ne 1 ]] && internal_error "is_cmd_available() expects 'cmd' argument"

  local -r cmd="$1"

  command -v "$cmd" &>/dev/null || return $FALSE # unavailable

  return $TRUE # available
}

# Determine whether `library` is available.
# @return TRUE if it is, FALSE otherwise.
is_library_available() {
  [[ $# -ne 1 ]] && internal_error "is_library_available() expects 'library' argument"

  local -r library="$1"

  is_cmd_available "pkgconf" || error "pkgconf is unavailable" $GENERIC_ERROR_CODE
  pkgconf --exists "$library" || return $FALSE # unavailable

  return $TRUE # available
}

# Retrieve version of `library`.
# This function asserts `library` availability.
# @stdout `library` version.
get_library_version() {
  [[ $# -ne 1 ]] && internal_error "get_library_version() expects 'library' argument"

  local -r library="$1"

  is_library_available "$library" || error "Library '${library}' is unavailable" $GENERIC_ERROR_CODE

  is_cmd_available "pkgconf" || error "pkgconf is unavailable" $GENERIC_ERROR_CODE
  pkgconf --modversion "$library" || error "Failed to retrieve version of '${library}' library"
}

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

[[ ! -d "$WORKING_TREE_ROOT_DIR" ]] && internal_error "WORKING_TREE_ROOT_DIR '${WORKING_TREE_ROOT_DIR}' is not a directory"
[[ ! -d "$SCRIPTS_DIR" ]] && internal_error "SCRIPTS_DIR '${SCRIPTS_DIR}' is not a directory"

##################################################
#          SET SCRIPT WORKING DIRECTORY          #
##################################################

cd "$WORKING_TREE_ROOT_DIR" ||
  error "Failed to navigate into '$WORKING_TREE_ROOT_DIR'" $GENERIC_ERROR_CODE
