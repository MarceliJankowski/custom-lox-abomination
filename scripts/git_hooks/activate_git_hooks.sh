#!/usr/bin/env bash

source "$(dirname "$0")/../scripts_common.sh"

##################################################
#                GLOBAL VARIABLES                #
##################################################

REPO_GIT_DIR=$(git rev-parse --git-common-dir) || exit $GENERIC_ERROR_CODE
readonly REPO_GIT_DIR
readonly GIT_HOOKS_DIR=${SCRIPTS_DIR}/git_hooks

[[ ! -d "$GIT_HOOKS_DIR" ]] && internal_error "GIT_HOOKS_DIR '${GIT_HOOKS_DIR}' is not a directory"

##################################################
#                   UTILITIES                    #
##################################################

# @desc create symbolic link pointing to GIT_HOOKS_DIR/`target_path` located at REPO_ROOT_DIR/.git/hooks/`link_path`
symlink_git_hook() {
  [[ $# -ne 2 ]] && internal_error "symlink_git_hook() expects 'target_path' and 'link_path' arguments"

  local -r target_path="${GIT_HOOKS_DIR}/${1}"
  local -r link_path="${REPO_GIT_DIR}/hooks/${2}"

  [[ ! -f "$target_path" ]] && internal_error "target_path '${target_path}' is not a file"
  [[ ! -x "$target_path" ]] && warning "Git hook '${target_path}' is not executable by '$(whoami)' user"

  [[ -e "$link_path" && ! -L "$link_path" ]] &&
    error "Git hook '$link_path' already exists and is not a symbolic link (manual intervention required)" $GENERIC_ERROR_CODE

  if [[ -L "$link_path" ]]; then
    rm "$link_path" ||
      error "Failed to remove pre-existing '$link_path' symlink" $GENERIC_ERROR_CODE
  fi

  ln -s "$target_path" "$link_path" ||
    error "Failed to symlink '${link_path}' to '${target_path}'" $GENERIC_ERROR_CODE

  return 0
}

##################################################
#                    SYMLINKS                    #
##################################################

log_action "Symlinking git_pre_commit.sh hook"
symlink_git_hook git_pre_commit.sh pre-commit

exit 0
