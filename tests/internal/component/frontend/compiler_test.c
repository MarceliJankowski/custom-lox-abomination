#include "frontend/compiler.h"

#include "backend/chunk.h"
#include "common.h"
#include "component/component_test.h"
#include "global.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static Chunk chunk;
static int chunk_code_offset;
static int chunk_constant_instruction_index;

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

static int count_non_negative_int_digits(int const non_negative_int) {
  assert(non_negative_int >= 0);

  if (non_negative_int == 0) return 1;

  return log10(non_negative_int) + 1;
}

static CompilerStatus compile(char const *const source_code) {
  assert(source_code != NULL);

  chunk_reset(&chunk);
  chunk_code_offset = 0;
  chunk_constant_instruction_index = 0;
  component_test_clear_binary_stream_resource_content(g_static_analysis_error_stream);

  return compiler_compile(source_code, &chunk);
}

#define COMPILE_ASSERT_SUCCESS(source_code) assert_int_equal(compile(source_code), COMPILER_SUCCESS)
#define COMPILE_ASSERT_FAILURE(source_code) assert_int_equal(compile(source_code), COMPILER_FAILURE)
#define COMPILE_ASSERT_UNEXPECTED_EOF(source_code) assert_int_equal(compile(source_code), COMPILER_UNEXPECTED_EOF)

static void assert_static_analysis_error(
  char const *const error_type, int const line, int const column, char const *const expected_error_message
) {
  assert(error_type != NULL);
  assert(line >= 0);
  assert(column >= 0);
  assert(expected_error_message != NULL);

  int const line_digit_count = count_non_negative_int_digits(line);
  int const column_digit_count = count_non_negative_int_digits(column);

  char const *const separator_1 = COMMON_MS __FILE__ COMMON_PS;
  char const *const separator_2 = COMMON_PS;
  char const *const separator_3 = COMMON_MS;
  char const *const ending = "\n";

  size_t const expected_static_analysis_error_length = strlen(error_type) + strlen(separator_1) + line_digit_count +
                                                       strlen(separator_2) + column_digit_count + strlen(separator_3) +
                                                       strlen(expected_error_message) + strlen(ending);

  char *const expected_static_analysis_error = malloc(expected_static_analysis_error_length);
  if (expected_static_analysis_error == NULL) ERROR_MEMORY_ERRNO();

  if (sprintf(
        expected_static_analysis_error, "%s%s%d%s%d%s%s%s", error_type, separator_1, line, separator_2, column,
        separator_3, expected_error_message, ending
      ) < 0)
    ERROR_IO_ERRNO();

  component_test_assert_binary_stream_resource_content(g_static_analysis_error_stream, expected_static_analysis_error);

  free(expected_static_analysis_error);
}

#define ASSERT_LEXICAL_ERROR(...) assert_static_analysis_error("[LEXICAL_ERROR]", __VA_ARGS__)
#define ASSERT_SYNTAX_ERROR(...) assert_static_analysis_error("[SYNTAX_ERROR]", __VA_ARGS__)
#define ASSERT_SEMANTIC_ERROR(...) assert_static_analysis_error("[SEMANTIC_ERROR]", __VA_ARGS__)

#define NEXT_CHUNK_CODE_BYTE() chunk.code.data[chunk_code_offset++]

#define ASSERT_INSTRUCTION_LINE(expected_line) \
  assert_int_equal(chunk_get_instruction_line(&chunk, chunk_code_offset), expected_line)

#define ASSERT_OPCODE(expected_opcode) assert_int_equal(NEXT_CHUNK_CODE_BYTE(), expected_opcode)
#define ASSERT_OPCODES(...) COMPONENT_TEST_APPLY_TO_EACH_ARG(ASSERT_OPCODE, ChunkOpCode, __VA_ARGS__)

static void assert_chunk_constant(int32_t const constant_index, Value const expected_constant) {
  assert_int_equal(chunk_constant_instruction_index, constant_index);
  component_test_assert_value_equality(chunk.constants.data[constant_index], expected_constant);
  chunk_constant_instruction_index++;
}

static void assert_OP_CONSTANT_instruction(Value const expected_constant) {
  ASSERT_OPCODE(CHUNK_OP_CONSTANT);
  uint32_t const constant_index = NEXT_CHUNK_CODE_BYTE();
  assert_chunk_constant(constant_index, expected_constant);
}

static void assert_OP_CONSTANT_2B_instruction(Value const expected_constant) {
  ASSERT_OPCODE(CHUNK_OP_CONSTANT_2B);
  uint8_t const constant_index_LSB = NEXT_CHUNK_CODE_BYTE();
  uint8_t const constant_index_MSB = NEXT_CHUNK_CODE_BYTE();
  uint32_t const constant_index = memory_concatenate_bytes(2, constant_index_MSB, constant_index_LSB);
  assert_chunk_constant(constant_index, expected_constant);
}

static void assert_constant_instruction(Value const expected_constant) {
  if (chunk_constant_instruction_index > UCHAR_MAX) assert_OP_CONSTANT_2B_instruction(expected_constant);
  else assert_OP_CONSTANT_instruction(expected_constant);
}
#define ASSERT_CONSTANT_INSTRUCTIONS(...) \
  COMPONENT_TEST_APPLY_TO_EACH_ARG(assert_constant_instruction, Value, __VA_ARGS__)

static ChunkOpCode map_binary_operator_to_its_opcode(char const *const operator) {
  assert(operator!= NULL);

  if (0 == strcmp(operator, "+")) return CHUNK_OP_ADD;
  if (0 == strcmp(operator, "-")) return CHUNK_OP_SUBTRACT;
  if (0 == strcmp(operator, "*")) return CHUNK_OP_MULTIPLY;
  if (0 == strcmp(operator, "/")) return CHUNK_OP_DIVIDE;
  if (0 == strcmp(operator, "%")) return CHUNK_OP_MODULO;
  if (0 == strcmp(operator, "==")) return CHUNK_OP_EQUAL;
  if (0 == strcmp(operator, "!=")) return CHUNK_OP_NOT_EQUAL;
  if (0 == strcmp(operator, "<")) return CHUNK_OP_LESS;
  if (0 == strcmp(operator, "<=")) return CHUNK_OP_LESS_EQUAL;
  if (0 == strcmp(operator, ">")) return CHUNK_OP_GREATER;
  if (0 == strcmp(operator, ">=")) return CHUNK_OP_GREATER_EQUAL;

  ERROR_INTERNAL("Unknown binary operator '%s'", operator);
}

#define ASSERT_BINARY_OPERATOR_SYNTAX(operator)                                      \
  do {                                                                               \
    ChunkOpCode const operator_opcode = map_binary_operator_to_its_opcode(operator); \
    int const operator_length = strlen(operator);                                    \
                                                                                     \
    COMPILE_ASSERT_FAILURE(operator);                                                \
    ASSERT_SYNTAX_ERROR(1, 1, "Expected expression at '" operator"'");               \
                                                                                     \
    COMPILE_ASSERT_UNEXPECTED_EOF("1 " operator);                                    \
    ASSERT_SYNTAX_ERROR(1, 3 + operator_length, "Expected expression");              \
                                                                                     \
    COMPILE_ASSERT_SUCCESS("1 " operator" 2;");                                      \
    ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));        \
    ASSERT_OPCODES(operator_opcode, CHUNK_OP_POP, CHUNK_OP_RETURN);                  \
  } while (0)

#define ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE(operator)                         \
  do {                                                                               \
    ChunkOpCode const operator_opcode = map_binary_operator_to_its_opcode(operator); \
    COMPILE_ASSERT_SUCCESS("1 " operator" 2 " operator" 3;");                        \
    ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));        \
    ASSERT_OPCODE(operator_opcode);                                                  \
    assert_constant_instruction(VALUE_MAKE_NUMBER(3));                               \
    ASSERT_OPCODES(operator_opcode, CHUNK_OP_POP, CHUNK_OP_RETURN);                  \
  } while (0)

#define ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE(operator_a, operator_b)         \
  do {                                                                                   \
    ChunkOpCode const operator_a_opcode = map_binary_operator_to_its_opcode(operator_a); \
    ChunkOpCode const operator_b_opcode = map_binary_operator_to_its_opcode(operator_b); \
                                                                                         \
    COMPILE_ASSERT_SUCCESS("1 " operator_a " 2 " operator_b " 3;");                      \
    ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));            \
    ASSERT_OPCODE(operator_a_opcode);                                                    \
    assert_constant_instruction(VALUE_MAKE_NUMBER(3));                                   \
    ASSERT_OPCODES(operator_b_opcode, CHUNK_OP_POP, CHUNK_OP_RETURN);                    \
                                                                                         \
    COMPILE_ASSERT_SUCCESS("1 " operator_b " 2 " operator_a " 3;");                      \
    ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));            \
    ASSERT_OPCODE(operator_b_opcode);                                                    \
    assert_constant_instruction(VALUE_MAKE_NUMBER(3));                                   \
    ASSERT_OPCODES(operator_a_opcode, CHUNK_OP_POP, CHUNK_OP_RETURN);                    \
  } while (0)

#define ASSERT_BINARY_OPERATOR_A_HAS_HIGHER_PRECEDENCE(operator_a, operator_b)                      \
  do {                                                                                              \
    ChunkOpCode const operator_a_opcode = map_binary_operator_to_its_opcode(operator_a);            \
    ChunkOpCode const operator_b_opcode = map_binary_operator_to_its_opcode(operator_b);            \
                                                                                                    \
    COMPILE_ASSERT_SUCCESS("1 " operator_b " 2 " operator_a " 3;");                                 \
    ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2), VALUE_MAKE_NUMBER(3)); \
    ASSERT_OPCODES(operator_a_opcode, operator_b_opcode, CHUNK_OP_POP, CHUNK_OP_RETURN);            \
  } while (0)

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  g_source_file_path = __FILE__;
  g_static_analysis_error_stream = tmpfile(); // assert_static_analysis_error expects this stream to be binary
  if (g_static_analysis_error_stream == NULL) ERROR_IO_ERRNO();

  chunk_init(&chunk);

  return 0;
}

static int teardown_test_group_env(void **const _) {
  if (fclose(g_static_analysis_error_stream)) ERROR_IO_ERRNO();

  chunk_destroy(&chunk);

  return 0;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*
static_assert(CHUNK_OP_OPCODE_COUNT == 21, "Exhaustive OpCode handling");

static void test_lexical_error_reporting(void **const _) {
  COMPILE_ASSERT_FAILURE("\"abc");
  ASSERT_LEXICAL_ERROR(1, 1, "Unterminated string literal");

  COMPILE_ASSERT_FAILURE("@");
  ASSERT_LEXICAL_ERROR(1, 1, "Unexpected character");
}

static void test_line_tracking(void **const _) {
  COMPILE_ASSERT_SUCCESS("nil;");
  ASSERT_INSTRUCTION_LINE(1);

  COMPILE_ASSERT_SUCCESS("\nnil;");
  ASSERT_INSTRUCTION_LINE(2);

  COMPILE_ASSERT_SUCCESS("\n\nnil;");
  ASSERT_INSTRUCTION_LINE(3);
}

static void test_expr_stmt_lacking_semicolon_terminator(void **const _) {
  COMPILE_ASSERT_UNEXPECTED_EOF("nil");
  ASSERT_SYNTAX_ERROR(1, 4, "Expected ';' terminating expression statement");
}

static void test_nil_literal(void **const _) {
  COMPILE_ASSERT_SUCCESS("nil;");
  ASSERT_OPCODES(CHUNK_OP_NIL, CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_bool_literal(void **const _) {
  COMPILE_ASSERT_SUCCESS("false;");
  ASSERT_OPCODES(CHUNK_OP_FALSE, CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("true;");
  ASSERT_OPCODES(CHUNK_OP_TRUE, CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_numeric_literal(void **const _) {
  COMPILE_ASSERT_SUCCESS("55;");
  assert_constant_instruction(VALUE_MAKE_NUMBER(55));
  ASSERT_OPCODES(CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("-55;");
  assert_constant_instruction(VALUE_MAKE_NUMBER(55));
  ASSERT_OPCODES(CHUNK_OP_NEGATE, CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("10.25;");
  assert_constant_instruction(VALUE_MAKE_NUMBER(10.25));
  ASSERT_OPCODES(CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("-10.25;");
  assert_constant_instruction(VALUE_MAKE_NUMBER(10.25));
  ASSERT_OPCODES(CHUNK_OP_NEGATE, CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_OP_CONSTANT_2B_being_generated(void **const _) {
  char source_code[(MEMORY_BYTE_STATE_COUNT + 1) * 2 + 1];
  for (size_t i = 0; i < MEMORY_BYTE_STATE_COUNT * 2; i += 2) {
    source_code[i] = '1';
    source_code[i + 1] = ';';
  }
  source_code[sizeof(source_code) - 3] = '2';
  source_code[sizeof(source_code) - 2] = ';';
  source_code[sizeof(source_code) - 1] = '\0';

  COMPILE_ASSERT_SUCCESS(source_code);
  for (size_t i = 0; i < MEMORY_BYTE_STATE_COUNT; i++) {
    assert_OP_CONSTANT_instruction(VALUE_MAKE_NUMBER(1));
    ASSERT_OPCODE(CHUNK_OP_POP);
  }
  assert_OP_CONSTANT_2B_instruction(VALUE_MAKE_NUMBER(2));
  ASSERT_OPCODES(CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_arithmetic_operators(void **const _) {
  ASSERT_BINARY_OPERATOR_SYNTAX("+");
  ASSERT_BINARY_OPERATOR_SYNTAX("*");
  ASSERT_BINARY_OPERATOR_SYNTAX("/");
  ASSERT_BINARY_OPERATOR_SYNTAX("%");

  // '-' is a unary/binary operator; hence it cannot use binary-operator-specific assertions
  COMPILE_ASSERT_UNEXPECTED_EOF("-");
  ASSERT_SYNTAX_ERROR(1, 2, "Expected expression");

  COMPILE_ASSERT_UNEXPECTED_EOF("2 -");
  ASSERT_SYNTAX_ERROR(1, 4, "Expected expression");

  COMPILE_ASSERT_SUCCESS("1 - 2;");
  ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));
  ASSERT_OPCODES(CHUNK_OP_SUBTRACT, CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_arithmetic_operator_associativity(void **const _) {
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("+");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("-");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("*");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("/");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("%");
}

static void test_arithmetic_operator_precedence(void **const _) {
  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE("+", "-");

  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE("*", "/");
  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE("/", "%");

  ASSERT_BINARY_OPERATOR_A_HAS_HIGHER_PRECEDENCE("*", "+");
}

static void test_grouping_expr(void **const _) {
  COMPILE_ASSERT_FAILURE(")");
  ASSERT_SYNTAX_ERROR(1, 1, "Expected expression at ')'");

  COMPILE_ASSERT_UNEXPECTED_EOF("(");
  ASSERT_SYNTAX_ERROR(1, 2, "Expected expression");

  COMPILE_ASSERT_SUCCESS("(1);");
  assert_constant_instruction(VALUE_MAKE_NUMBER(1));
  ASSERT_OPCODES(CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("(1 + 2);");
  ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));
  ASSERT_OPCODES(CHUNK_OP_ADD, CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("(1 + 2) * 3;");
  ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));
  ASSERT_OPCODE(CHUNK_OP_ADD);
  assert_constant_instruction(VALUE_MAKE_NUMBER(3));
  ASSERT_OPCODES(CHUNK_OP_MULTIPLY, CHUNK_OP_POP, CHUNK_OP_RETURN);

  COMPILE_ASSERT_SUCCESS("(1 + 2) * (3 / 4);");
  ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(1), VALUE_MAKE_NUMBER(2));
  ASSERT_OPCODE(CHUNK_OP_ADD);
  ASSERT_CONSTANT_INSTRUCTIONS(VALUE_MAKE_NUMBER(3), VALUE_MAKE_NUMBER(4));
  ASSERT_OPCODES(CHUNK_OP_DIVIDE, CHUNK_OP_MULTIPLY, CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_logical_operators(void **const _) {
  COMPILE_ASSERT_UNEXPECTED_EOF("!");
  ASSERT_SYNTAX_ERROR(1, 2, "Expected expression");

  COMPILE_ASSERT_SUCCESS("!true;");
  ASSERT_OPCODES(CHUNK_OP_TRUE, CHUNK_OP_NOT, CHUNK_OP_POP, CHUNK_OP_RETURN);
}

static void test_relational_operators(void **const _) {
  ASSERT_BINARY_OPERATOR_SYNTAX("==");
  ASSERT_BINARY_OPERATOR_SYNTAX("!=");
  ASSERT_BINARY_OPERATOR_SYNTAX("<");
  ASSERT_BINARY_OPERATOR_SYNTAX("<=");
  ASSERT_BINARY_OPERATOR_SYNTAX("<");
  ASSERT_BINARY_OPERATOR_SYNTAX("<=");
}

static void test_relational_operator_associativity(void **const _) {
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("==");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("!=");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("<");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE("<=");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE(">");
  ASSERT_BINARY_OPERATOR_IS_LEFT_ASSOCIATIVE(">=");
}

static void test_relational_operator_precedence(void **const _) {
  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE("==", "!=");

  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE("<", "<=");
  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE("<=", ">");
  ASSERT_BINARY_OPERATORS_HAVE_THE_SAME_PRECEDENCE(">", ">=");

  ASSERT_BINARY_OPERATOR_A_HAS_HIGHER_PRECEDENCE(">", "==");
}

static void test_print_stmt(void **const _) {
  COMPILE_ASSERT_UNEXPECTED_EOF("print");
  ASSERT_SYNTAX_ERROR(1, 6, "Expected expression");

  COMPILE_ASSERT_UNEXPECTED_EOF("print 5");
  ASSERT_SYNTAX_ERROR(1, 8, "Expected ';' terminating print statement");

  COMPILE_ASSERT_SUCCESS("print 5;");
  assert_constant_instruction(VALUE_MAKE_NUMBER(5));
  ASSERT_OPCODES(CHUNK_OP_PRINT, CHUNK_OP_RETURN);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(test_lexical_error_reporting),
    cmocka_unit_test(test_line_tracking),
    cmocka_unit_test(test_expr_stmt_lacking_semicolon_terminator),
    cmocka_unit_test(test_nil_literal),
    cmocka_unit_test(test_bool_literal),
    cmocka_unit_test(test_numeric_literal),
    cmocka_unit_test(test_OP_CONSTANT_2B_being_generated),
    cmocka_unit_test(test_arithmetic_operators),
    cmocka_unit_test(test_arithmetic_operator_associativity),

    cmocka_unit_test(test_arithmetic_operator_precedence),
    cmocka_unit_test(test_grouping_expr),
    cmocka_unit_test(test_logical_operators),
    cmocka_unit_test(test_relational_operators),
    cmocka_unit_test(test_relational_operator_associativity),
    cmocka_unit_test(test_relational_operator_precedence),
    cmocka_unit_test(test_print_stmt),
  };

  return cmocka_run_group_tests(tests, setup_test_group_env, teardown_test_group_env);
}
