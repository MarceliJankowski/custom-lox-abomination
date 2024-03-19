##################################################
#               EXTERNAL CONSTANTS               #
##################################################
# meant to provide simple layer of customizability

EXEC_NAME ?= clox
BUILD_DIR ?= build
BIN_DIR ?= bin
SRC_DIR := src
INCLUDE_DIR := include
TEST_DIR := test
TEST_LIBS := criterion

FIND ?= find
MKDIR := mkdir -p
RM := rm -r

CC ?=
CFLAGS ?=
CPPFLAGS ?=
TARGET_ARCH ?=
LDFLAGS ?=

BUILDS += release debug test
RELEASE_CFLAGS ?= -O3 -flto -march=native

# no optimizations; I've seen them interfere with UBSAN (-Og included)
DEBUG_CFLAGS ?= -ggdb -fno-omit-frame-pointer -fsanitize=address,undefined
DEBUG_CPPFLAGS ?= -D DEBUG

# allow user to set .DEFAULT_GOAL through DEFAULT_TARGET environmental variable (or CLI)
# while keeping default value assigned to DEFAULT_TARGET (works around '.DEFAULT_GOAL ?=' not being viable)
DEFAULT_TARGET ?= $(firstword ${BUILDS})
.DEFAULT_GOAL := ${DEFAULT_TARGET}

##################################################
#               INTERNAL VARIABLES               #
##################################################

non_test_builds := $(filter-out test,${BUILDS})

compile_cflags := -Wall -Wextra -Werror -std=c17 -pedantic
compile_cppflags := -I ${INCLUDE_DIR} -MMD -MP

compile = $(strip ${CC} ${compile_cppflags} ${CPPFLAGS} ${compile_cflags} ${CFLAGS} ${TARGET_ARCH} -c)
link = $(strip ${CC} ${compile_cppflags} ${CPPFLAGS} ${compile_cflags} ${CFLAGS} ${LDFLAGS} ${TARGET_ARCH})

sources := $(shell ${FIND} ${SRC_DIR} -name '*.c')
objects := $(patsubst %.c,%.o,${sources})
dependencies := $(foreach build,${non_test_builds},$(patsubst %.o,${BUILD_DIR}/${build}/%.d,${objects}))

unit_tests := $(shell ${FIND} ${TEST_DIR}/unit -name '*.c')
unit_test_objects := $(patsubst %.c,${BUILD_DIR}/test/%.o,${unit_tests})

integration_tests := $(shell ${FIND} ${TEST_DIR}/integration -name '*.c')
integration_test_objects := $(patsubst %.c,${BUILD_DIR}/test/%.o,${integration_tests})

dependencies += $(patsubst %.o,%.d,${unit_test_objects} ${integration_test_objects})

# objects to be included in test build; 'main.o' is excluded because criterion brings its own main entry point
test_objects := $(addprefix ${BUILD_DIR}/release/,$(filter-out ${SRC_DIR}/main.o,${objects}))

clean_build_targets := $(foreach build,${BUILDS},clean-${build})
clean_targets := clean ${clean_build_targets}

##################################################
#                   FUNCTIONS                    #
##################################################

# $(call generate_rule_building_objects_for_given_build,build)
define generate_rule_building_objects_for_given_build
# prevent make from deleting object files
.NOTINTERMEDIATE: ${BUILD_DIR}/$1/%.o

${BUILD_DIR}/$1/%.o: %.c
	@ ${MKDIR} $$(dir $$@)
	$${compile} $$< -o $$@
endef

# $(call generate_rule_cleaning_given_build,build)
define generate_rule_cleaning_given_build
clean-$1:
	${RM} ${BUILD_DIR}/$1 ${BIN_DIR}/$1
endef

##################################################
#                     RULES                      #
##################################################

.DELETE_ON_ERROR:
.PHONY: all ${clean_targets} ${BUILDS}

# "build" specific variables
release: compile_cflags += ${RELEASE_CFLAGS}
debug: compile_cppflags += ${DEBUG_CPPFLAGS}
debug: compile_cflags += ${DEBUG_CFLAGS}

all: ${BUILDS}

# build "build"
${non_test_builds}: %: ${BIN_DIR}/%/${EXEC_NAME}

# build test "build"
test: ${BIN_DIR}/test/unit ${BIN_DIR}/test/integration

# build "build" executable
${BIN_DIR}/%/${EXEC_NAME}: $(addprefix ${BUILD_DIR}/%/,${objects})
	@ ${MKDIR} $(dir $@)
	${link} $^ -o $@

# build test "build" executables
${BIN_DIR}/test/unit: ${unit_test_objects}
${BIN_DIR}/test/integration: ${integration_test_objects}
${BIN_DIR}/test/%: ${test_objects}
	@ ${MKDIR} $(dir $@)
	${link} $(foreach test_lib,${TEST_LIBS},-l ${test_lib}) $^ -o $@

# build "build" objects
$(foreach build,${BUILDS}, \
  $(eval $(call generate_rule_building_objects_for_given_build,${build})))

# clean "build"
$(foreach build,${BUILDS}, \
  $(eval $(call generate_rule_cleaning_given_build,${build})))

clean:
	${RM} ${BUILD_DIR} ${BIN_DIR}

##################################################
#                  DEPENDENCIES                  #
##################################################

# include automatically generated makefiles tracking header dependencies (unless user just wants to clean up)
ifndef MAKECMDGOALS
# if there are no command goals, user doesn't want to clean up
  -include ${dependencies}
else
  ifneq "$(filter ${clean_targets},${MAKECMDGOALS})" "${MAKECMDGOALS}"
# if there are command goals but not all of them are clean targets, user doesn't want to just clean up
    -include ${dependencies}
  endif
endif
