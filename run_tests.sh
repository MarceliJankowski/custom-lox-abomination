#!/usr/bin/env bash

set -o nounset
set -o pipefail

# to get info on this script run it with '-h' flag

##################################################
#                GLOBAL VARIABLES                #
##################################################

readonly SCRIPT_NAME=$(basename "$0")
readonly PROJECT_ROOT_DIR=$(dirname "$0")

readonly TRUE=0
readonly FALSE=1

readonly INVALID_FLAG_ERROR_CODE=1
readonly INVALID_ARG_ERROR_CODE=2
readonly MISSING_ARG_ERROR_CODE=3
readonly TOO_MANY_ARGS_ERROR_CODE=4
readonly MAKE_FAILURE_ERROR_CODE=5
readonly TEST_FAILURE_ERROR_CODE=6
readonly INTERNAL_ERROR_CODE=255

# test types must be in lowercase
readonly UNIT_TEST_TYPE='unit'
readonly INTEGRATION_TEST_TYPE='integration'
readonly E2E_TEST_TYPE='e2e'
readonly ALL_TEST_TYPES=("$UNIT_TEST_TYPE" "$INTEGRATION_TEST_TYPE" "$E2E_TEST_TYPE")

readonly MAX_ARG_COUNT=${#ALL_TEST_TYPES[@]}

TEST_TYPES_TO_RUN=()
FAILED_TEST_TYPES=() # used when KEEP_GOING_MODE is on

# options (can be set through CLI)
FAIL_FAST_MODE=$FALSE
KEEP_GOING_MODE=$FALSE
VERBOSE_MODE=$FALSE

readonly MANUAL="
NAME
      $SCRIPT_NAME - run clox tests

SYNOPSIS
      $SCRIPT_NAME [-h] [-v] [-f] [-k] [test_type]...

DESCRIPTION
      Test clox release build.

      Tests are divided into target categories based on their type.
      Available test types: $(sed 's/ /, /g' <<<${ALL_TEST_TYPES[@]}).

      All available test types are executed by default.
      User can override this behaviour by supplying test_type arguments (casing doesn't matter).
      In such case, only supplied test types will be executed; execution order mimics argument order.

OPTIONS
      -h
          Get help, print out the manual and exit.

      -v
          Turn on VERBOSE_MODE (increases output).

      -f
          Turn on FAIL_FAST_MODE (exits immediately after first test failure).

          This option is mutually exclusive with '-k'.

      -k
          Turn on KEEP_GOING_MODE (keeps going after test type failure).
          Failed test types will be summarized at the end.

          This option is mutually exclusive with '-f'.

EXIT CODES
      Exit code indicates whether $SCRIPT_NAME successfully executed, or failed for some reason.
      Different exit codes indicate different failure causes:

      0  $SCRIPT_NAME successfully run, without raising any exceptions.

      $INVALID_FLAG_ERROR_CODE  Invalid flag supplied.

      $INVALID_ARG_ERROR_CODE  Invalid argument supplied.

      $MISSING_ARG_ERROR_CODE  Missing mandatory argument.

      $TOO_MANY_ARGS_ERROR_CODE  Too many arguments supplied (max number: ${MAX_ARG_COUNT}).

      $MAKE_FAILURE_ERROR_CODE  Make failure occoured.

      $TEST_FAILURE_ERROR_CODE  Test failure occurred.

      $INTERNAL_ERROR_CODE  Developer fuc**d up, blame him!
"

##################################################
#               UTILITY FUNCTIONS                #
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
	exit "$exit_code"
}

# @desc print global MANUAL variable
print_manual() {
	[[ $# -ne 0 ]] && throw_internal_error "print_manual() expects no arguments"

	echo "$MANUAL" | sed -e '1d' -e '$d'
}

# @desc log `message` to stdout if VERBOSE_MODE is on
log_verbose() {
	[[ $# -ne 1 ]] && throw_internal_error "log_verbose() expects 'message' argument"

	local -r message="$1"

	[[ $VERBOSE_MODE -eq $TRUE ]] && echo -e "[VERBOSE] - $message"
}

# @desc check if `value` is an `array` element
# @return 0 if it is, 1 otherwise
is_array_element() {
	[[ $# -lt 2 ]] && throw_internal_error "is_array_element() expects 'value' and 'array' arguments"

	local -r value="$1"
	shift
	local -r array="$@"

	local element
	for element in $array; do
		[[ "$element" = "$value" ]] && return 0
	done

	return 1
}

# @desc handle test type failure
handle_test_type_fail() {
	[[ $# -ne 1 ]] && throw_internal_error "handle_test_type_fail() expects 'test_type' argument"

	local -r test_type="$1"

	[[ $KEEP_GOING_MODE -eq $FALSE ]] && exit $TEST_FAILURE_ERROR_CODE
	FAILED_TEST_TYPES+=("$test_type")
}

# @desc make `build`
make_build() {
	[[ $# -ne 1 ]] && throw_internal_error "make_build() expects 'build' argument"

	local -r build="$1"

	if [[ $VERBOSE_MODE -eq $TRUE ]]; then
		make "$build" || exit $MAKE_FAILURE_ERROR_CODE
	else
		make "$build" 1>/dev/null || exit $MAKE_FAILURE_ERROR_CODE
	fi
}

# @desc run ./bin/test/`exec_name`
# @return propagates executable exit code
run_test_exec() {
	[[ $# -ne 1 ]] && throw_internal_error "run_test_exec() expects 'exec_name' argument"

	local -r exec_name="$1"
	local flags=()

	# these are criterion flags
	[[ $FAIL_FAST_MODE -eq $TRUE ]] && flags+=('--fail-fast')
	[[ $VERBOSE_MODE -eq $TRUE ]] && flags+=('--verbose')

	./bin/test/$exec_name "${flags[@]}"
}

##################################################
#                 RUN FUNCTIONS                  #
##################################################

run_unit_tests() {
	[[ $# -ne 0 ]] && throw_internal_error "run_unit_tests() expects no arguments"

	log_verbose "Running unit tests..."
	run_test_exec unit || handle_test_type_fail "$UNIT_TEST_TYPE"
}

run_integration_tests() {
	[[ $# -ne 0 ]] && throw_internal_error "run_integration_tests() expects no arguments"

	log_verbose "Running integration tests..."
	run_test_exec integration || handle_test_type_fail "$INTEGRATION_TEST_TYPE"
}

run_e2e_tests() {
	[[ $# -ne 0 ]] && throw_internal_error "run_e2e_tests() expects no arguments"

	log_verbose "Running E2E tests..."
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
	is_array_element "${SCRIPT_ARG,,}" "${ALL_TEST_TYPES[@]}" ||
		throw_error "Invalid test_type argument supplied '$SCRIPT_ARG'" $INVALID_ARG_ERROR_CODE

	TEST_TYPES_TO_RUN+=("${SCRIPT_ARG,,}")
done

[[ ${#TEST_TYPES_TO_RUN[@]} -eq 0 ]] && TEST_TYPES_TO_RUN=("${ALL_TEST_TYPES[@]}")

# navigate into project root directory so that user can execute this script from anywhere
cd "$PROJECT_ROOT_DIR"
[[ -d .git ]] || throw_internal_error "Expected $SCRIPT_NAME to be located in project root directory"

# make builds required for testing
log_verbose "Making builds required for testing..."
is_array_element "$E2E_TEST_TYPE" "$TEST_TYPES_TO_RUN" && make_build release
if is_array_element "$INTEGRATION_TEST_TYPE" "$TEST_TYPES_TO_RUN" ||
	is_array_element "$UNIT_TEST_TYPE" "$TEST_TYPES_TO_RUN"; then
	make_build test
fi

# run test types in the specified order
for ((i = 0; i < ${#TEST_TYPES_TO_RUN[@]}; i++)); do
	[[ "${TEST_TYPES_TO_RUN[$i]}" = "$UNIT_TEST_TYPE" ]] && run_unit_tests
	[[ "${TEST_TYPES_TO_RUN[$i]}" = "$INTEGRATION_TEST_TYPE" ]] && run_integration_tests
	[[ "${TEST_TYPES_TO_RUN[$i]}" = "$E2E_TEST_TYPE" ]] && run_e2e_tests
done

# exit
[[ $KEEP_GOING_MODE -eq $TRUE && ${#FAILED_TEST_TYPES[@]} -gt 0 ]] &&
	throw_error "Failed test types: ${FAILED_TEST_TYPES[@]}" $TEST_FAILURE_ERROR_CODE

exit 0
