#!/usr/bin/env bash

source "$(dirname "$0")/scripts_common.sh"

# to get info on this script run it with '-h' flag

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly SCRIPT_NAME=$(basename "$0")

readonly TEST_DIR='test'
readonly BIN_DIR='bin'

readonly INVALID_FLAG_ERROR_CODE=2
readonly INVALID_ARG_ERROR_CODE=3
readonly MISSING_ARG_ERROR_CODE=4
readonly TOO_MANY_ARGS_ERROR_CODE=5
readonly MAKE_FAILURE_ERROR_CODE=6
readonly TEST_FAILURE_ERROR_CODE=7

# test types must be in lowercase
readonly UNIT_TEST_TYPE='unit'
readonly COMPONENT_TEST_TYPE='component'
readonly E2E_TEST_TYPE='e2e'
readonly ALL_TEST_TYPES=("$UNIT_TEST_TYPE" "$COMPONENT_TEST_TYPE" "$E2E_TEST_TYPE") # order matters

readonly MAX_ARG_COUNT=${#ALL_TEST_TYPES[@]}

TEST_TYPES_TO_RUN=()
FAILED_TEST_TYPES=() # used when KEEP_GOING_MODE is on

# options (can be set through CLI)
FAIL_FAST_MODE=$FALSE
KEEP_GOING_MODE=$FALSE
VERBOSE_MODE=$FALSE

readonly MANUAL="
NAME
       $SCRIPT_NAME - run cla tests

SYNOPSIS
       $SCRIPT_NAME [-h] [-v] [-f] [-k] [test_type]...

DESCRIPTION
       Test cla release build.

       Tests are divided into target categories based on their type.
       Available test types: $(sed 's/ /, /g' <<<${ALL_TEST_TYPES[*]}).

       All available test types are executed by default.
       User can override this behaviour by supplying test_type arguments (casing doesn't matter).
       In such case, only supplied test types will be executed; execution order mimics argument order.

       Upon test file failure, execution continues until the entire test type has been executed.
       At that point, execution is aborted (this behaviour can be altered with '-f' and '-k' flags).

OPTIONS
       -h
           Get help, print out the manual and exit.

       -v
           Turn on VERBOSE_MODE (increases output).

       -f
           Turn on FAIL_FAST_MODE (exits immediately after first test file failure).

           This option is mutually exclusive with '-k'.

       -k
           Turn on KEEP_GOING_MODE (keeps going after test type failure).
           Failed test types will be summarized at the end.

           This option is mutually exclusive with '-f'.

EXIT CODES
       Exit code indicates whether $SCRIPT_NAME successfully executed, or failed for some reason.
       Different exit codes indicate different failure causes:

       0  $SCRIPT_NAME successfully run, without raising any exceptions.

       $GENERIC_ERROR_CODE  Generic (unspecified on this list) failure occurred.

       $INVALID_FLAG_ERROR_CODE  Invalid flag supplied.

       $INVALID_ARG_ERROR_CODE  Invalid argument supplied.

       $MISSING_ARG_ERROR_CODE  Missing mandatory argument.

       $TOO_MANY_ARGS_ERROR_CODE  Too many arguments supplied (max number: ${MAX_ARG_COUNT}).

       $MAKE_FAILURE_ERROR_CODE  Make failure occurred.

       $TEST_FAILURE_ERROR_CODE  Test failure occurred.

       $INTERNAL_ERROR_CODE  Developer fuc**d up, blame him!
"

##################################################
#               UTILITY FUNCTIONS                #
##################################################

# @desc print global MANUAL variable
print_manual() {
  [[ $# -ne 0 ]] && throw_internal_error "print_manual() expects no arguments"

  echo "$MANUAL" | sed -e '1d' -e '$d'
}

# @desc log verbose `message` to stdout if VERBOSE_MODE is on
log_if_verbose() {
  [[ $# -ne 1 ]] && throw_internal_error "log_if_verbose() expects 'message' argument"

  local -r message="$1"

  [[ $VERBOSE_MODE -eq $TRUE ]] && echo -e "[VERBOSE] - $message"
}

# @desc check if `array` contains at least one element from `search_elements` arguments
# @return 0 if it does, 1 otherwise
array_contains() {
  [[ $# -lt 2 ]] && throw_internal_error "array_contains() expects 'array' and 'search_elements' arguments"

  local -r array="$1"
  shift
  local -r search_elements="$@"

  local element
  local search_element
  for element in $array; do
    for search_element in $search_elements; do
      [[ "$element" = "$search_element" ]] && return 0
    done
  done

  return 1
}

# @desc make `build`
make_build() {
  [[ $# -ne 1 ]] && throw_internal_error "make_build() expects 'build' argument"

  local -r build="$1"

  [[ $VERBOSE_MODE -eq $TRUE ]] && make "$build" || make "$build" 1>/dev/null
  [[ $? -ne 0 ]] && exit $MAKE_FAILURE_ERROR_CODE
}

# @desc handle `test_type` failure
handle_test_type_fail() {
  [[ $# -ne 1 ]] && throw_internal_error "handle_test_type_fail() expects 'test_type' argument"

  local -r test_type="$1"

  [[ $KEEP_GOING_MODE -eq $FALSE ]] && exit $TEST_FAILURE_ERROR_CODE
  FAILED_TEST_TYPES+=("$test_type")
}

# @desc run test executables corresponding to `test_type` `test_files`
run_test_executables() {
  [[ $# -ne 2 ]] && throw_internal_error "run_test_executables() expects 'test_type' and 'test_files' arguments"

  local -r test_type="$1"
  local -r test_files="$2"

  log_if_verbose "Running $test_type tests..."

  local test_file
  local test_executable_output # hide test_executable output unless it failed or VERBOSE_MODE is on
  local did_test_executable_failure_occur=$FALSE
  for test_file in $test_files; do
    local test_executable="./bin/test/${test_type}/${test_file::-2}" # remove '.c' extension

    [[ $VERBOSE_MODE -eq $TRUE ]] && $test_executable || test_executable_output=$($test_executable 2>&1)
    if [[ $? -ne 0 ]]; then
      did_test_executable_failure_occur=$TRUE
      [[ $VERBOSE_MODE -eq $FALSE ]] && echo "$test_executable_output"
      [[ $FAIL_FAST_MODE -eq $TRUE ]] && break
    fi
  done

  [[ $did_test_executable_failure_occur -eq $TRUE ]] && handle_test_type_fail "$test_type"
}

##################################################
#                 RUN FUNCTIONS                  #
##################################################

run_unit_tests() {
  [[ $# -ne 0 ]] && throw_internal_error "run_unit_tests() expects no arguments"

  local -r unit_test_files=$(find ${TEST_DIR}/unit -type f -name '*_spec.c')

  run_test_executables "$UNIT_TEST_TYPE" "$unit_test_files"
}

run_component_tests() {
  [[ $# -ne 0 ]] && throw_internal_error "run_component_tests() expects no arguments"

  local -r component_test_files=$(find ${TEST_DIR}/component -type f -name '*_test.c')

  run_test_executables "$COMPONENT_TEST_TYPE" "$component_test_files"
}

run_e2e_tests() {
  [[ $# -ne 0 ]] && throw_internal_error "run_e2e_tests() expects no arguments"

  log_if_verbose "Running E2E tests..."
  # nothing to run yet...
}

##################################################
#             EXECUTION ENTRY POINT              #
##################################################

# handle flags
while getopts ':hvfk' FLAG; do
  case "$FLAG" in
  h) print_manual && exit 0 ;;
  v) VERBOSE_MODE=$TRUE ;;
  f) FAIL_FAST_MODE=$TRUE ;;
  k) KEEP_GOING_MODE=$TRUE ;;
  :) throw_error "Flag '-${OPTARG}' requires argument" $MISSING_ARG_ERROR_CODE ;;
  ?) throw_error "Invalid flag '-${OPTARG}' supplied" $INVALID_FLAG_ERROR_CODE ;;
  esac
done

[[ $FAIL_FAST_MODE -eq $TRUE && $KEEP_GOING_MODE -eq $TRUE ]] &&
  throw_error "Mutually exclusive '-f' and '-k' flags supplied" $INVALID_FLAG_ERROR_CODE

# turn options into constants
readonly VERBOSE_MODE
readonly FAIL_FAST_MODE
readonly KEEP_GOING_MODE

# remove flags, leaving script arguments
shift $((OPTIND - 1))

# handle script arguments
[[ $# -gt $MAX_ARG_COUNT ]] &&
  throw_error "Too many arguments supplied (max number: ${MAX_ARG_COUNT})" $TOO_MANY_ARGS_ERROR_CODE

for SCRIPT_ARG in "$@"; do
  array_contains "${ALL_TEST_TYPES[*]}" "${SCRIPT_ARG,,}" ||
    throw_error "Invalid test_type argument supplied '$SCRIPT_ARG'" $INVALID_ARG_ERROR_CODE

  TEST_TYPES_TO_RUN+=("${SCRIPT_ARG,,}")
done

[[ ${#TEST_TYPES_TO_RUN[@]} -eq 0 ]] && TEST_TYPES_TO_RUN=("${ALL_TEST_TYPES[@]}")

# make builds required for testing
log_if_verbose "Making builds required for testing..."
array_contains "${TEST_TYPES_TO_RUN[*]}" "$E2E_TEST_TYPE" && make_build release
array_contains "${TEST_TYPES_TO_RUN[*]}" "$UNIT_TEST_TYPE" "$COMPONENT_TEST_TYPE" && make_build test

# run test types in specified order
for ((i = 0; i < ${#TEST_TYPES_TO_RUN[@]}; i++)); do
  [[ "${TEST_TYPES_TO_RUN[$i]}" = "$UNIT_TEST_TYPE" ]] && run_unit_tests
  [[ "${TEST_TYPES_TO_RUN[$i]}" = "$COMPONENT_TEST_TYPE" ]] && run_component_tests
  [[ "${TEST_TYPES_TO_RUN[$i]}" = "$E2E_TEST_TYPE" ]] && run_e2e_tests
done

# exit
[[ $KEEP_GOING_MODE -eq $TRUE && ${#FAILED_TEST_TYPES[@]} -gt 0 ]] &&
  throw_error "Failed test types: $(sed 's/ /, /g' <<<${FAILED_TEST_TYPES[*]})." $TEST_FAILURE_ERROR_CODE

exit 0
