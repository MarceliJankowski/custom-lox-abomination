#!/usr/bin/env bash

source "$(dirname $(readlink -e "$0"))/../scripts_common.sh"

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly UNSTAGED_FILES=$(git diff --name-only || exit $GENERIC_ERROR_CODE)
readonly STAGED_FILES=$(git diff --staged --name-only || exit $GENERIC_ERROR_CODE)

readonly STAGED_C_FILES=$(grep '\.c$' <<<"$STAGED_FILES")
readonly STAGED_HEADER_FILES=$(grep '\.h$' <<<"$STAGED_FILES")

##################################################
#                   UTILITIES                    #
##################################################

# @desc pop stashed unstaged changes
pop_stashed_unstaged_changes() {
  [[ $# -ne 0 ]] && throw_internal_error "pop_stashed_unstaged_changes() expects no arguments"

  log_action "Popping stashed unstaged changes"
  git stash pop 1>/dev/null ||
    throw_error "Failed to pop stashed unstaged changes" $GENERIC_ERROR_CODE

  return 0
}

# @desc terminate action pipeline and exit with GENERIC_ERROR_CODE
abort_action_pipeline() {
  [[ $# -ne 0 ]] && throw_internal_error "abort_action_pipeline() expects no arguments"

  pop_stashed_unstaged_changes

  exit $GENERIC_ERROR_CODE
}

##################################################
#                ACTION PIPELINE                 #
##################################################

log_action "Stashing unstaged changes"
git stash save --keep-index --include-untracked 'git-pre-commit-unstaged-changes' 1>/dev/null ||
  throw_error "Failed to stash unstaged changes" $GENERIC_ERROR_CODE

log_action "Checking formatting of staged files"
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

pop_stashed_unstaged_changes
exit 0
