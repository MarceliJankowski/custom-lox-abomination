#include "frontend/compiler.h"

#include "backend/chunk.h"
#include "common.h"
#include "component/component_test_utils.h"
#include "global.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <stdio.h>
#include <stdlib.h>

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static Chunk chunk;
static int chunk_code_offset;
static int chunk_constant_instruction_index;

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

static CompilationStatus compile(char const *const source_code) {
  assert(source_code != NULL);

  chunk_reset(&chunk);
  chunk_code_offset = 0;
  chunk_constant_instruction_index = 0;
  clear_binary_stream_resource_content(g_static_error_stream);

  return compiler_compile(source_code, &chunk);
}

#define compile_assert_success(source_code) assert_int_equal(compile(source_code), COMPILATION_SUCCESS)
#define compile_assert_failure(source_code) assert_int_equal(compile(source_code), COMPILATION_FAILURE)
#define compile_assert_unexpected_eof(source_code) assert_int_equal(compile(source_code), COMPILATION_UNEXPECTED_EOF)

#define assert_static_error(error_type, line, column, expected_error_message)                            \
  assert_binary_stream_resource_content(                                                                 \
    g_static_error_stream, error_type M_S __FILE__ P_S #line P_S #column M_S expected_error_message "\n" \
  )

#define assert_lexical_error(...) assert_static_error("[LEXICAL_ERROR]", __VA_ARGS__)
#define assert_syntax_error(...) assert_static_error("[SYNTAX_ERROR]", __VA_ARGS__)
#define assert_semantic_error(...) assert_static_error("[SEMANTIC_ERROR]", __VA_ARGS__)

#define next_chunk_code_byte() chunk.code[chunk_code_offset++]

#define assert_instruction_line(expected_line) \
  assert_int_equal(chunk_get_instruction_line(&chunk, chunk_code_offset), expected_line)

#define assert_opcode(expected_opcode) assert_int_equal(next_chunk_code_byte(), expected_opcode)
#define assert_opcodes(...) APPLY_TO_EACH_ARG(assert_opcode, OpCode, __VA_ARGS__)

static void assert_chunk_constant(int32_t const constant_index, Value const expected_constant) {
  assert_int_equal(chunk_constant_instruction_index, constant_index);
  assert_value_equality(chunk.constants.values[constant_index], expected_constant);
  chunk_constant_instruction_index++;
}

static void assert_OP_CONSTANT_instruction(Value const expected_constant) {
  assert_opcode(OP_CONSTANT);
  uint32_t const constant_index = next_chunk_code_byte();
  assert_chunk_constant(constant_index, expected_constant);
}

static void assert_OP_CONSTANT_2B_instruction(Value const expected_constant) {
  assert_opcode(OP_CONSTANT_2B);
  uint8_t const constant_index_LSB = next_chunk_code_byte();
  uint8_t const constant_index_MSB = next_chunk_code_byte();
  uint32_t const constant_index = concatenate_bytes(2, constant_index_MSB, constant_index_LSB);
  assert_chunk_constant(constant_index, expected_constant);
}

static void assert_constant_instruction(Value const expected_constant) {
  if (chunk_constant_instruction_index > UCHAR_MAX) assert_OP_CONSTANT_2B_instruction(expected_constant);
  else assert_OP_CONSTANT_instruction(expected_constant);
}
#define assert_constant_instructions(...) APPLY_TO_EACH_ARG(assert_constant_instruction, Value, __VA_ARGS__)

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  g_source_file = __FILE__;
  g_static_error_stream = tmpfile(); // assert_static_error expects this stream to be binary
  if (g_static_error_stream == NULL) IO_ERROR("%s", strerror(errno));

  chunk_init(&chunk);

  return 0;
}

static int teardown_test_group_env(void **const _) {
  if (fclose(g_static_error_stream)) IO_ERROR("%s", strerror(errno));

  chunk_free(&chunk);

  return 0;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*
static_assert(OP_OPCODE_COUNT == 14, "Exhaustive OpCode handling");

static void test_lexical_error_reporting(void **const _) {
  compile_assert_failure("\"abc");
  assert_lexical_error(1, 1, "Unterminated string literal");

  compile_assert_failure("@");
  assert_lexical_error(1, 1, "Unexpected character");
}

static void test_line_tracking(void **const _) {
  compile_assert_success("nil;");
  assert_instruction_line(1);

  compile_assert_success("\nnil;");
  assert_instruction_line(2);

  compile_assert_success("\n\nnil;");
  assert_instruction_line(3);
}

static void test_expr_stmt_lacking_semicolon_terminator(void **const _) {
  compile_assert_unexpected_eof("nil");
  assert_syntax_error(1, 4, "Expected ';' terminating expression statement");
}

static void test_nil_literal(void **const _) {
  compile_assert_success("nil;");
  assert_opcodes(OP_NIL, OP_POP, OP_RETURN);
}

static void test_bool_literal(void **const _) {
  compile_assert_success("false;");
  assert_opcodes(OP_FALSE, OP_POP, OP_RETURN);

  compile_assert_success("true;");
  assert_opcodes(OP_TRUE, OP_POP, OP_RETURN);
}

static void test_numeric_literal(void **const _) {
  compile_assert_success("55;");
  assert_constant_instruction(NUMBER_VALUE(55));
  assert_opcodes(OP_POP, OP_RETURN);

  compile_assert_success("-55;");
  assert_constant_instruction(NUMBER_VALUE(55));
  assert_opcodes(OP_NEGATE, OP_POP, OP_RETURN);

  compile_assert_success("10.25;");
  assert_constant_instruction(NUMBER_VALUE(10.25));
  assert_opcodes(OP_POP, OP_RETURN);

  compile_assert_success("-10.25;");
  assert_constant_instruction(NUMBER_VALUE(10.25));
  assert_opcodes(OP_NEGATE, OP_POP, OP_RETURN);
}

static void test_OP_CONSTANT_2B_being_generated(void **const _) {
  char source_code[(BYTE_STATE_COUNT + 1) * 2 + 1];
  for (size_t i = 0; i < BYTE_STATE_COUNT * 2; i += 2) {
    source_code[i] = '1';
    source_code[i + 1] = ';';
  }
  source_code[sizeof(source_code) - 3] = '2';
  source_code[sizeof(source_code) - 2] = ';';
  source_code[sizeof(source_code) - 1] = '\0';

  compile_assert_success(source_code);
  for (size_t i = 0; i < BYTE_STATE_COUNT; i++) {
    assert_OP_CONSTANT_instruction(NUMBER_VALUE(1));
    assert_opcode(OP_POP);
  }
  assert_OP_CONSTANT_2B_instruction(NUMBER_VALUE(2));
  assert_opcodes(OP_POP, OP_RETURN);
}

static void test_arithmetic_operators(void **const _) {
  compile_assert_failure("+");
  assert_syntax_error(1, 1, "Expected expression at '+'");
  compile_assert_failure("*");
  assert_syntax_error(1, 1, "Expected expression at '*'");
  compile_assert_failure("/");
  assert_syntax_error(1, 1, "Expected expression at '/'");
  compile_assert_failure("%");
  assert_syntax_error(1, 1, "Expected expression at '%'");

  compile_assert_unexpected_eof("-"); // '-' symbol also denotes unary negation operator, hence unexpected_eof
  assert_syntax_error(1, 2, "Expected expression");
  compile_assert_unexpected_eof("1 +");
  assert_syntax_error(1, 4, "Expected expression");
  compile_assert_unexpected_eof("2 -");
  assert_syntax_error(1, 4, "Expected expression");
  compile_assert_unexpected_eof("3 *");
  assert_syntax_error(1, 4, "Expected expression");
  compile_assert_unexpected_eof("4 /");
  assert_syntax_error(1, 4, "Expected expression");
  compile_assert_unexpected_eof("5 %");
  assert_syntax_error(1, 4, "Expected expression");

  compile_assert_success("1 + 2;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcodes(OP_ADD, OP_POP, OP_RETURN);

  compile_assert_success("1 - 2;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcodes(OP_SUBTRACT, OP_POP, OP_RETURN);

  compile_assert_success("1 * 2;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcodes(OP_MULTIPLY, OP_POP, OP_RETURN);

  compile_assert_success("1 / 2;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcodes(OP_DIVIDE, OP_POP, OP_RETURN);

  compile_assert_success("1 % 2;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcodes(OP_MODULO, OP_POP, OP_RETURN);
}

static void test_arithmetic_operator_associativity(void **const _) {
  compile_assert_success("1 + 2 + 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_ADD);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_ADD, OP_POP, OP_RETURN);

  compile_assert_success("1 - 2 - 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_SUBTRACT);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_SUBTRACT, OP_POP, OP_RETURN);

  compile_assert_success("1 * 2 * 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_MULTIPLY);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_MULTIPLY, OP_POP, OP_RETURN);

  compile_assert_success("1 / 2 / 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_DIVIDE);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_DIVIDE, OP_POP, OP_RETURN);

  compile_assert_success("1 % 2 % 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_MODULO);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_MODULO, OP_POP, OP_RETURN);
}

static void test_arithmetic_operator_precedence(void **const _) {
  compile_assert_success("1 + 2 - 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_ADD);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_SUBTRACT, OP_POP, OP_RETURN);

  compile_assert_success("1 - 2 + 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_SUBTRACT);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_ADD, OP_POP, OP_RETURN);

  compile_assert_success("1 * 2 / 3 % 4;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_MULTIPLY);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcode(OP_DIVIDE);
  assert_constant_instruction(NUMBER_VALUE(4));
  assert_opcodes(OP_MODULO, OP_POP, OP_RETURN);

  compile_assert_success("1 % 2 * 3 / 4;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_MODULO);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcode(OP_MULTIPLY);
  assert_constant_instruction(NUMBER_VALUE(4));
  assert_opcodes(OP_DIVIDE, OP_POP, OP_RETURN);

  compile_assert_success("1 / 2 % 3 * 4;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_DIVIDE);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcode(OP_MODULO);
  assert_constant_instruction(NUMBER_VALUE(4));
  assert_opcodes(OP_MULTIPLY, OP_POP, OP_RETURN);

  compile_assert_success("1 + 2 * 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2), NUMBER_VALUE(3));
  assert_opcodes(OP_MULTIPLY, OP_ADD, OP_POP, OP_RETURN);
}

static void test_grouping_expr(void **const _) {
  compile_assert_failure(")");
  assert_syntax_error(1, 1, "Expected expression at ')'");

  compile_assert_unexpected_eof("(");
  assert_syntax_error(1, 2, "Expected expression");

  compile_assert_success("(1);");
  assert_constant_instruction(NUMBER_VALUE(1));
  assert_opcodes(OP_POP, OP_RETURN);

  compile_assert_success("(1 + 2);");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcodes(OP_ADD, OP_POP, OP_RETURN);

  compile_assert_success("(1 + 2) * 3;");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_ADD);
  assert_constant_instruction(NUMBER_VALUE(3));
  assert_opcodes(OP_MULTIPLY, OP_POP, OP_RETURN);

  compile_assert_success("(1 + 2) * (3 / 4);");
  assert_constant_instructions(NUMBER_VALUE(1), NUMBER_VALUE(2));
  assert_opcode(OP_ADD);
  assert_constant_instructions(NUMBER_VALUE(3), NUMBER_VALUE(4));
  assert_opcodes(OP_DIVIDE, OP_MULTIPLY, OP_POP, OP_RETURN);
}

static void test_print_stmt(void **const _) {
  compile_assert_unexpected_eof("print");
  assert_syntax_error(1, 6, "Expected expression");

  compile_assert_unexpected_eof("print 5");
  assert_syntax_error(1, 8, "Expected ';' terminating print statement");

  compile_assert_success("print 5;");
  assert_constant_instruction(NUMBER_VALUE(5));
  assert_opcodes(OP_PRINT, OP_RETURN);
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
    cmocka_unit_test(test_print_stmt),
  };

  return cmocka_run_group_tests(tests, setup_test_group_env, teardown_test_group_env);
}
