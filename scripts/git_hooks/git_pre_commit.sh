#!/usr/bin/env bash

source "$(dirname $(readlink -e "$0"))/../scripts_common.sh"

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly UNTRACKED_FILES=$(git ls-files --others --exclude-standard || exit $GENERIC_ERROR_CODE)
readonly UNSTAGED_FILES=$(git diff --name-only || exit $GENERIC_ERROR_CODE)
readonly STAGED_FILES=$(git diff --staged --name-only || exit $GENERIC_ERROR_CODE)

readonly STAGED_C_FILES=$(grep '\.c$' <<<"$STAGED_FILES")
readonly STAGED_HEADER_FILES=$(grep '\.h$' <<<"$STAGED_FILES")

if [[ -n "$UNTRACKED_FILES" || -n "$UNSTAGED_FILES" ]]; then
  readonly RUN_STASH_ACTION=$TRUE
else
  readonly RUN_STASH_ACTION=$FALSE
fi

##################################################
#                   UTILITIES                    #
##################################################

# @desc pop git stash frame
pop_stash_frame() {
  [[ $# -ne 0 ]] && throw_internal_error "pop_stash_frame() expects no arguments"

  log_action "Popping stash frame"
  git stash pop 1>/dev/null ||
    throw_error "Failed to pop stash frame" $GENERIC_ERROR_CODE

  return 0
}

# @desc terminate action pipeline and exit with GENERIC_ERROR_CODE
abort_action_pipeline() {
  [[ $# -ne 0 ]] && throw_internal_error "abort_action_pipeline() expects no arguments"

  [[ $RUN_STASH_ACTION -eq $TRUE ]] && pop_stash_frame

  exit $GENERIC_ERROR_CODE
}

##################################################
#                ACTION PIPELINE                 #
##################################################

if [[ $RUN_STASH_ACTION -eq $TRUE ]]; then
  log_action "Stashing unstaged changes and untracked files"
  git stash save --keep-index --include-untracked 'git-pre-commit-frame' 1>/dev/null ||
    throw_error "Failed to stash unstaged changes and untracked files" $GENERIC_ERROR_CODE
fi

log_action "Checking formatting of staged changes"
if [[ -n "$STAGED_C_FILES" || -n "$STAGED_HEADER_FILES" ]]; then
  clang-format --dry-run -Werror $STAGED_C_FILES $STAGED_HEADER_FILES || abort_action_pipeline
fi

if [[ "$UNSTAGED_FILES $STAGED_FILES" = *'Makefile'* ]]; then
  log_action "Cleaning all cla builds due to modified Makefile"
  make clean 1>/dev/null || abort_action_pipeline
fi

log_action "Making all cla builds"
make all 1>/dev/null || abort_action_pipeline

log_action "Testing release build"
${SCRIPTS_DIR}/run_tests.sh || abort_action_pipeline

[[ $RUN_STASH_ACTION -eq $TRUE ]] && pop_stash_frame
exit 0
