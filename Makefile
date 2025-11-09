##################################################
#               EXTERNAL CONSTANTS               #
##################################################
# meant to provide simple layer of customizability

LANG_IMPL_NAME := cla
SRC_DIR := src
SCRIPTS_DIR := scripts
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin
TESTS_DIR := tests
TEST_LIBS := cmocka

# either posix or windows
TARGET_SYSTEM ?= posix

CC ?=
WINDOWS_CC ?= x86_64-w64-mingw32-gcc
CFLAGS ?=
CPPFLAGS ?=
TARGET_ARCH ?=
LDFLAGS ?=
LDLIBS ?=
FIND ?= find
ECHO ?= echo
MKDIR := mkdir -p
RM := rm -rf
GENERATE_COMPILATION_DATABASE := ${SCRIPTS_DIR}/miscellaneous/generate_compilation_database.sh
RUN_TESTS := ${SCRIPTS_DIR}/test_runner/run_tests.sh

# this variable gets used by script executing targets for argument passing
ARGS ?=

LANG_IMPL_BUILDS += release debug
BUILDS := ${LANG_IMPL_BUILDS} tests

RELEASE_CFLAGS ?= -O3 -flto -march=native

# no optimizations; I've seen them interfere with UBSAN (-Og included)
DEBUG_CFLAGS ?= -ggdb -fno-omit-frame-pointer -fsanitize=address,undefined
DEBUG_CPPFLAGS ?= -D DEBUG

# allow user to set .DEFAULT_GOAL through DEFAULT_TARGET environmental variable (or CLI)
# while keeping default value assigned to DEFAULT_TARGET (works around '.DEFAULT_GOAL ?=' not being viable)
DEFAULT_TARGET ?= help
.DEFAULT_GOAL := ${DEFAULT_TARGET}

##################################################
#               INTERNAL VARIABLES               #
##################################################

ifeq "${TARGET_SYSTEM}" "posix"
  c_compiler := ${CC}
  lang_impl_exec_name := ${LANG_IMPL_NAME}
else ifeq "${TARGET_SYSTEM}" "windows"
  c_compiler := ${WINDOWS_CC}
  lang_impl_exec_name := ${LANG_IMPL_NAME}.exe
else
  $(error TARGET_SYSTEM '${TARGET_SYSTEM}' is invalid)
endif

internal_tests_dir := ${TESTS_DIR}/internal
test_utils_dir := ${internal_tests_dir}/utils
unit_tests_dir := ${internal_tests_dir}/unit
component_tests_dir := ${internal_tests_dir}/component

compile_cflags := -Wall -Wextra -Werror -std=c17 -pedantic
compile_cppflags := -I ${INCLUDE_DIR}
link_flags :=
link_libs := -lm

sources := $(shell ${FIND} ${SRC_DIR} -type f -name '*.c')
source_objects := $(patsubst %.c,%.o,${sources})

unit_tests := $(shell ${FIND} ${unit_tests_dir} -type f -name '*.c')
unit_test_executables := $(patsubst %.c,${BIN_DIR}/tests/unit/%,${unit_tests})

component_tests := $(shell ${FIND} ${component_tests_dir} -type f -name '*.c')
component_test_executables := $(patsubst %.c,${BIN_DIR}/tests/component/%,${component_tests})

# release objects that component tests depend on; 'main.o' is excluded because each test file defines its own entry point
component_test_release_objects := $(addprefix ${BUILD_DIR}/release/,$(filter-out ${SRC_DIR}/main.o,${source_objects}))

common_test_utils := $(shell ${FIND} ${test_utils_dir}/common -type f -name '*.c')
unit_test_utils := ${common_test_utils} $(shell ${FIND} ${test_utils_dir}/unit -type f -name '*.c')
component_test_utils := ${common_test_utils} $(shell ${FIND} ${test_utils_dir}/component -type f -name '*.c')

unit_test_makefiles := $(shell ${FIND} ${unit_test_dir} -type f -name '*.mk')
unit_test_mk_target_prefix := ${BIN_DIR}/tests/unit/${unit_tests_dir}
unit_test_mk_prerequisite_prefix := ${BUILD_DIR}/release/${SRC_DIR}

# compiler generated makefiles tracking header dependencies
dependency_makefiles := $(foreach build,${LANG_IMPL_BUILDS},$(patsubst %.o,${BUILD_DIR}/${build}/%.d,${source_objects}))
dependency_makefiles += $(patsubst %.c,${BUILD_DIR}/tests/%.d,${unit_tests} ${component_tests})

clean_build_targets := $(foreach build,${BUILDS},clean-${build})
clean_targets := clean ${clean_build_targets}

##################################################
#           TARGET-SPECIFIC VARIABLES            #
##################################################

release: compile_cflags += ${RELEASE_CFLAGS}
debug: compile_cppflags += ${DEBUG_CPPFLAGS}
debug: compile_cflags += ${DEBUG_CFLAGS}
test-executables: compile_cppflags += -I ${test_utils_dir}
test-executables: compile_cflags += -Wno-unused-parameter
test-executables: link_libs += $(foreach test_lib,${TEST_LIBS},-l${test_lib})

# fix clang failing to link test executables (this happens when some but not all objects are compiled with '-flto' flag)
ifeq "${c_compiler}" "clang"
  test-executables: link_flags += -fuse-ld=lld
endif

##################################################
#                   FUNCTIONS                    #
##################################################

# $(call compile,source_file,output_file)
define compile
$(strip ${c_compiler} \
  -c -MMD -MP \
  ${compile_cppflags} ${CPPFLAGS} \
  ${compile_cflags} ${CFLAGS} \
  ${TARGET_ARCH} \
  ${1} -o ${2})
endef

# $(call link,object_files,output_file)
define link
$(strip ${c_compiler} \
  ${compile_cppflags} ${CPPFLAGS} \
  ${compile_cflags} ${CFLAGS} \
  ${TARGET_ARCH} \
  ${link_flags} ${LDFLAGS} \
  ${1} -o ${2} \
  ${link_libs} ${LDLIBS})
endef

# $(call generate_rule_building_objects_for_given_build,build)
define generate_rule_building_objects_for_given_build
# prevent make from deleting object files
.NOTINTERMEDIATE: ${BUILD_DIR}/$1/%.o

${BUILD_DIR}/$1/%.o: %.c
	@ ${MKDIR} $$(dir $$@)
	$$(call compile,$$<,$$@)
endef

# $(call generate_rule_cleaning_given_build,build)
define generate_rule_cleaning_given_build
clean-$1:
	${RM} ${BUILD_DIR}/$1 ${BIN_DIR}/$1
endef

##################################################
#                CANNED SEQUENCES                #
##################################################

define make_target_dir_and_link_prerequisites_into_target
	@ ${MKDIR} $(dir $@)
	$(call link,$^,$@)
endef

##################################################
#                     RULES                      #
##################################################

.DELETE_ON_ERROR:
.PHONY: all ${BUILDS} test-executables ${clean_targets} run-tests compilation-database help

all: ${BUILDS}

# make language implementation builds
${LANG_IMPL_BUILDS}: %: ${BIN_DIR}/%/${lang_impl_exec_name}

# make tests build (split into 2 targets to avoid release/tests specific variables being applied simultaneously)
tests: release test-executables
test-executables: ${unit_test_executables} ${component_test_executables}

# make language implementation executables
${BIN_DIR}/%/${lang_impl_exec_name}: $(addprefix ${BUILD_DIR}/%/,${source_objects})
	${make_target_dir_and_link_prerequisites_into_target}

# make test executables
${BIN_DIR}/tests/unit/%: ${BUILD_DIR}/tests/%.o ${unit_test_utils}
	${make_target_dir_and_link_prerequisites_into_target}

${BIN_DIR}/tests/component/%: ${BUILD_DIR}/tests/%.o ${component_test_release_objects} ${component_test_utils}
	${make_target_dir_and_link_prerequisites_into_target}

# make build objects
$(foreach build,${BUILDS}, \
  $(eval $(call generate_rule_building_objects_for_given_build,${build})))

# clean builds
$(foreach build,${BUILDS}, \
  $(eval $(call generate_rule_cleaning_given_build,${build})))

clean:
	${RM} ${BUILD_DIR} ${BIN_DIR}

run-tests:
	@ ${RUN_TESTS} ${ARGS}

compilation-database:
	@ ${GENERATE_COMPILATION_DATABASE}

help:
	@ ${ECHO} "Targets:"
	@ ${ECHO} "    * release -- make release build"
	@ ${ECHO} "    * debug -- make debug build"
	@ ${ECHO} "    * tests -- make tests build"
	@ ${ECHO} "    * all -- make all builds"
	@ ${ECHO} "    * clean -- clean all builds"
	@ ${ECHO} "    * clean-{build} -- clean specified {build}"
	@ ${ECHO} "    * run-tests [ARGS] -- run cla tests"
	@ ${ECHO} "    * compilation-database -- make compile_commands.json"
	@ ${ECHO} "    * help -- display target list (default)"

##################################################
#                    INCLUDES                    #
##################################################

# only include makefiles if user doesn't want to just clean up
ifndef MAKECMDGOALS
# if there are no command goals, user doesn't want to clean up
  -include ${dependency_makefiles} ${unit_test_makefiles}
else
  ifneq "$(filter ${clean_targets},${MAKECMDGOALS})" "${MAKECMDGOALS}"
# if there are command goals but not all of them are clean targets, user doesn't want to just clean up
    -include ${dependency_makefiles} ${unit_test_makefiles}
  endif
endif
