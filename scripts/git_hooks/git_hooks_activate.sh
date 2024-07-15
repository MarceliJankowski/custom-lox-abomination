#!/usr/bin/env bash

source "$(dirname "$0")/../scripts_common.sh"

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly GIT_HOOKS_DIR=${SCRIPTS_DIR}/git_hooks
[[ ! -d "$GIT_HOOKS_DIR" ]] && throw_internal_error "GIT_HOOKS_DIR '${GIT_HOOKS_DIR}' is not a directory"

##################################################
#                   UTILITIES                    #
##################################################

# @desc create symbolic link pointing to GIT_HOOKS_DIR/`target_path` located at REPO_ROOT_DIR/.git/hooks/`link_path`
symlink_git_hook() {
  [[ $# -ne 2 ]] && throw_internal_error "symlink_git_hook() expects 'target_path' and 'link_path' arguments"

  local -r target_path="${GIT_HOOKS_DIR}/${1}"
  local -r link_path="${REPO_ROOT_DIR}/.git/hooks/${2}"

  [[ ! -e "$target_path" ]] && throw_internal_error "target_path '${target_path}' doesn't exist!"

  [[ -e "$link_path" && ! -L "$link_path" ]] &&
    throw_error "link_path '$link_path' already exists and is not a symbolic link (manual intervention required)" $GENERIC_ERROR_CODE

  if [[ -L "$link_path" ]]; then
    rm "$link_path" ||
      throw_error "Failed to remove pre-existing '$link_path' symlink" $GENERIC_ERROR_CODE
  fi

  ln -s "$target_path" "$link_path" ||
    throw_error "Failed to symlink '${link_path}' to '${target_path}'" $GENERIC_ERROR_CODE

  return 0
}

##################################################
#                    SYMLINKS                    #
##################################################

log_action "Symlinking git_pre_commit.sh hook"
symlink_git_hook git_pre_commit.sh pre-commit

exit 0
