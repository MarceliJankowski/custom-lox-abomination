#!/usr/bin/env bash

set -o nounset
set -o pipefail

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly GENERIC_ERROR_CODE=1
readonly GIT_ERROR_CODE=2
readonly INTERNAL_ERROR_CODE=255 # indicates that developer fucked up

readonly REPO_ROOT_DIR=$(git rev-parse --show-toplevel || exit $GIT_ERROR_CODE)
readonly GIT_HOOKS_DIR=${REPO_ROOT_DIR}/git_hooks

readonly UNSTAGED_FILES=$(git diff --name-only || exit $GIT_ERROR_CODE)
readonly STAGED_FILES=$(git diff --staged --name-only || exit $GIT_ERROR_CODE)

##################################################
#                   UTILITIES                    #
##################################################

# @desc log `message` to stderr and exit with INTERNAL_ERROR_CODE
throw_internal_error() {
  local message="$1"

  [[ $# -ne 1 ]] && message="throw_internal_error() expects 'message' argument"

  echo -e "[INTERNAL_ERROR] - $message" 1>&2

  exit $INTERNAL_ERROR_CODE
}

# @desc log `message` to stderr and exit with `exit_code`
throw_error() {
  [[ $# -ne 2 ]] && throw_internal_error "throw_error() expects 'message' and 'exit_code' arguments"

  local -r message="$1"
  local -r exit_code="$2"

  echo -e "[ERROR] - $message" 1>&2

  exit $exit_code
}

##################################################
#                 MISCELLANEOUS                  #
##################################################

# validate constant paths
[[ ! -d "$REPO_ROOT_DIR" ]] && throw_internal_error "REPO_ROOT_DIR '$REPO_ROOT_DIR' is not a directory"
[[ ! -d "$GIT_HOOKS_DIR" ]] && throw_internal_error "GIT_HOOKS_DIR '$GIT_HOOKS_DIR' is not a directory"

cd "$REPO_ROOT_DIR" ||
  throw_error "Failed to navigate into '$REPO_ROOT_DIR'" $GENERIC_ERROR_CODE
