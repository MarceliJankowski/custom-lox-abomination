#include "frontend/lexer.h"

#include "backend/vm.h"
#include "component/component_test.h"

#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define SCAN_ASSERT_EOF_KIND() scan_assert_kind(LEXER_TOKEN_EOF)
#define SCAN_ASSERT_EOF(expected_line, expected_column) \
  scan_assert_basic(LEXER_TOKEN_EOF, expected_line, expected_column, "EOF")

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

static void reset_lexer(char const *const source_code) {
  vm_reset();
  lexer_init(source_code);
}

static void scan_assert_kind(LexerTokenKind const expected_kind) {
  LexerToken const token = lexer_scan();

  assert_int_equal(token.kind, expected_kind);

  lexer_token_free(token);
}

static void scan_assert_basic(
  LexerTokenKind const expected_kind, int32_t const expected_line, int const expected_column,
  char const *const expected_lexeme
) {
  LexerToken const token = lexer_scan();

  assert_int_equal(token.kind, expected_kind);
  assert_int_equal(token.line, expected_line);
  assert_int_equal(token.column, expected_column);

  size_t const expected_lexeme_length = strlen(expected_lexeme);
  assert_memory_equal(token.as.basic.lexeme, expected_lexeme, expected_lexeme_length);
  assert_int_equal(token.as.basic.lexeme_length, expected_lexeme_length);

  lexer_token_free(token);
}

static void scan_assert_error(
  int32_t const expected_line, int const expected_column, char const *const expected_message
) {
  LexerToken const token = lexer_scan();

  assert_int_equal(token.kind, LEXER_TOKEN_ERROR);
  assert_int_equal(token.line, expected_line);
  assert_int_equal(token.column, expected_column);
  assert_string_equal(token.as.error.message, expected_message);

  lexer_token_free(token);
}

static inline void reset_scan_assert_basic(
  char const *const source_code, int32_t const expected_line, int const expected_column,
  LexerTokenKind const expected_kind
) {
  reset_lexer(source_code);
  scan_assert_basic(expected_kind, expected_line, expected_column, source_code);
  SCAN_ASSERT_EOF_KIND();
}

static inline void reset_scan_assert_error(
  char const *const source_code, int32_t const expected_line, int const expected_column,
  char const *const expected_message
) {
  reset_lexer(source_code);
  scan_assert_error(expected_line, expected_column, expected_message);
}

// *---------------------------------------------*
// *                  FIXTURES                   *
// *---------------------------------------------*

static int setup_test_group_env(void **const _) {
  vm_init();

  return 0;
}

static int teardown_test_group_env(void **const _) {
  vm_destroy();

  return 0;
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void test_eof_token(void **const _) {
  reset_lexer("");

  // continually returns EOF token upon reaching NUL byte
  for (int i = 0; i < 3; i++) SCAN_ASSERT_EOF_KIND();
}

static void test_whitespace(void **const _) {
  // skips whitespace
  reset_lexer(" "), SCAN_ASSERT_EOF_KIND();
  reset_lexer("\t"), SCAN_ASSERT_EOF_KIND();
  reset_lexer("\r"), SCAN_ASSERT_EOF_KIND();
  reset_lexer("\n"), SCAN_ASSERT_EOF_KIND();
  reset_lexer(" \t \r \n "), SCAN_ASSERT_EOF_KIND();
}

static void test_token_position_tracking(void **const _) {
  reset_lexer(""), SCAN_ASSERT_EOF(1, 1);
  reset_lexer(" "), SCAN_ASSERT_EOF(1, 2);
  reset_lexer("\n"), SCAN_ASSERT_EOF(2, 1);
  reset_lexer("\t"), SCAN_ASSERT_EOF(1, 2);
  reset_lexer("\r"), SCAN_ASSERT_EOF(1, 2);
  reset_lexer("\r\n"), SCAN_ASSERT_EOF(2, 1);
  reset_lexer("\n "), SCAN_ASSERT_EOF(2, 2);
  reset_lexer("\n\n"), SCAN_ASSERT_EOF(3, 1);
  reset_lexer("   \n"), SCAN_ASSERT_EOF(2, 1);
  reset_lexer("    \n    "), SCAN_ASSERT_EOF(2, 5);
}

static void test_unexpected_char(void **const _) {
  reset_scan_assert_error("`", 1, 1, "Unexpected character '`'");
  reset_scan_assert_error("~", 1, 1, "Unexpected character '~'");
  reset_scan_assert_error("@", 1, 1, "Unexpected character '@'");
  reset_scan_assert_error("$", 1, 1, "Unexpected character '$'");
  reset_scan_assert_error("^", 1, 1, "Unexpected character '^'");
  reset_scan_assert_error("&", 1, 1, "Unexpected character '&'");
  reset_scan_assert_error("[", 1, 1, "Unexpected character '['");
  reset_scan_assert_error("]", 1, 1, "Unexpected character ']'");
  reset_scan_assert_error("|", 1, 1, "Unexpected character '|'");
  reset_scan_assert_error("\\", 1, 1, "Unexpected character '\\'");
  reset_scan_assert_error("'", 1, 1, "Unexpected character '''");
}

static void test_string_literal(void **const _) {
  { // valid
    reset_scan_assert_basic("\"\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"abc\"", 1, 1, LEXER_TOKEN_STRING);

    // escape sequences
    reset_scan_assert_basic("\"\\\\\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\\"\\\"\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\a\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\b\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\f\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\n\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\r\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\t\"", 1, 1, LEXER_TOKEN_STRING);
    reset_scan_assert_basic("\"\\v\"", 1, 1, LEXER_TOKEN_STRING);
  }

  { // invalid
    reset_scan_assert_error("\"\\i\"", 1, 1, "Invalid escape sequence '\\i'");

    // unterminated
    reset_scan_assert_error("\"\\\"", 1, 1, "Unterminated string literal at '\"\\\"'");
    reset_scan_assert_error("\"abc", 1, 1, "Unterminated string literal at '\"abc'");

    // multiline
    reset_scan_assert_error("\"abc\ndef", 1, 1, "Unterminated string literal at '\"abc'");
    reset_scan_assert_error("\"abc\ndef\"", 1, 1, "Unterminated string literal at '\"abc'");
  }
}

static void test_numeric_literal(void **const _) {
  reset_scan_assert_basic("55", 1, 1, LEXER_TOKEN_NUMBER);
  reset_scan_assert_basic("10.25", 1, 1, LEXER_TOKEN_NUMBER);

  reset_lexer("-55");
  scan_assert_basic(LEXER_TOKEN_MINUS, 1, 1, "-");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 2, "55");
  SCAN_ASSERT_EOF(1, 4);

  reset_lexer("-10.25");
  scan_assert_basic(LEXER_TOKEN_MINUS, 1, 1, "-");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 2, "10.25");
  SCAN_ASSERT_EOF(1, 7);

  reset_lexer("4.");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 1, "4");
  scan_assert_basic(LEXER_TOKEN_DOT, 1, 2, ".");
  SCAN_ASSERT_EOF(1, 3);

  reset_lexer(".5");
  scan_assert_basic(LEXER_TOKEN_DOT, 1, 1, ".");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 2, "5");
  SCAN_ASSERT_EOF(1, 3);
}

static void test_identifier_literal(void **const _) {
  reset_scan_assert_basic("_", 1, 1, LEXER_TOKEN_IDENTIFIER);
  reset_scan_assert_basic("_name", 1, 1, LEXER_TOKEN_IDENTIFIER);
  reset_scan_assert_basic("name_123", 1, 1, LEXER_TOKEN_IDENTIFIER);
  reset_scan_assert_basic("name123", 1, 1, LEXER_TOKEN_IDENTIFIER);
  reset_scan_assert_basic(
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_", 1, 1, LEXER_TOKEN_IDENTIFIER
  );
}

static void test_single_char_tokens(void **const _) {
  static_assert(LEXER_TOKEN_SINGLE_CHAR_COUNT == 18, "Exhaustive single-character token handling");
  reset_scan_assert_basic("+", 1, 1, LEXER_TOKEN_PLUS);
  reset_scan_assert_basic("-", 1, 1, LEXER_TOKEN_MINUS);
  reset_scan_assert_basic("*", 1, 1, LEXER_TOKEN_STAR);
  reset_scan_assert_basic("/", 1, 1, LEXER_TOKEN_SLASH);
  reset_scan_assert_basic("%", 1, 1, LEXER_TOKEN_PERCENT);
  reset_scan_assert_basic("!", 1, 1, LEXER_TOKEN_BANG);
  reset_scan_assert_basic("<", 1, 1, LEXER_TOKEN_LESS);
  reset_scan_assert_basic("=", 1, 1, LEXER_TOKEN_EQUAL);
  reset_scan_assert_basic(">", 1, 1, LEXER_TOKEN_GREATER);
  reset_scan_assert_basic(".", 1, 1, LEXER_TOKEN_DOT);
  reset_scan_assert_basic(",", 1, 1, LEXER_TOKEN_COMMA);
  reset_scan_assert_basic(":", 1, 1, LEXER_TOKEN_COLON);
  reset_scan_assert_basic(";", 1, 1, LEXER_TOKEN_SEMICOLON);
  reset_scan_assert_basic("?", 1, 1, LEXER_TOKEN_QUESTION);
  reset_scan_assert_basic("(", 1, 1, LEXER_TOKEN_OPEN_PAREN);
  reset_scan_assert_basic(")", 1, 1, LEXER_TOKEN_CLOSE_PAREN);
  reset_scan_assert_basic("{", 1, 1, LEXER_TOKEN_OPEN_CURLY_BRACE);
  reset_scan_assert_basic("}", 1, 1, LEXER_TOKEN_CLOSE_CURLY_BRACE);
}

static void test_multi_char_tokens(void **const _) {
  static_assert(LEXER_TOKEN_MULTI_CHAR_COUNT == 5, "Exhaustive multi-character token handling");
  reset_scan_assert_basic("!=", 1, 1, LEXER_TOKEN_BANG_EQUAL);
  reset_scan_assert_basic("<=", 1, 1, LEXER_TOKEN_LESS_EQUAL);
  reset_scan_assert_basic("==", 1, 1, LEXER_TOKEN_EQUAL_EQUAL);
  reset_scan_assert_basic(">=", 1, 1, LEXER_TOKEN_GREATER_EQUAL);
  reset_scan_assert_basic("..", 1, 1, LEXER_TOKEN_DOT_DOT);
}

static void test_keyword_tokens(void **const _) {
  static_assert(LEXER_TOKEN_KEYWORD_COUNT == 16, "Exhaustive keyword token handling");
  reset_scan_assert_basic("true", 1, 1, LEXER_TOKEN_TRUE);
  reset_scan_assert_basic("false", 1, 1, LEXER_TOKEN_FALSE);
  reset_scan_assert_basic("var", 1, 1, LEXER_TOKEN_VAR);
  reset_scan_assert_basic("nil", 1, 1, LEXER_TOKEN_NIL);
  reset_scan_assert_basic("and", 1, 1, LEXER_TOKEN_AND);
  reset_scan_assert_basic("or", 1, 1, LEXER_TOKEN_OR);
  reset_scan_assert_basic("fun", 1, 1, LEXER_TOKEN_FUN);
  reset_scan_assert_basic("return", 1, 1, LEXER_TOKEN_RETURN);
  reset_scan_assert_basic("if", 1, 1, LEXER_TOKEN_IF);
  reset_scan_assert_basic("else", 1, 1, LEXER_TOKEN_ELSE);
  reset_scan_assert_basic("while", 1, 1, LEXER_TOKEN_WHILE);
  reset_scan_assert_basic("for", 1, 1, LEXER_TOKEN_FOR);
  reset_scan_assert_basic("class", 1, 1, LEXER_TOKEN_CLASS);
  reset_scan_assert_basic("super", 1, 1, LEXER_TOKEN_SUPER);
  reset_scan_assert_basic("this", 1, 1, LEXER_TOKEN_THIS);
  reset_scan_assert_basic("print", 1, 1, LEXER_TOKEN_PRINT);
}

static void test_comment(void **const _) {
  reset_lexer("# comment"), SCAN_ASSERT_EOF(1, 10);
  reset_lexer("# comment... # continues..."), SCAN_ASSERT_EOF(1, 28);
  reset_lexer("# comment spans single line\n +"), scan_assert_basic(LEXER_TOKEN_PLUS, 2, 2, "+");
}

static void test_input_source_code_1(void **const _) {
  reset_lexer("(-1 + 2) * 3 - -4");

  scan_assert_basic(LEXER_TOKEN_OPEN_PAREN, 1, 1, "(");
  scan_assert_basic(LEXER_TOKEN_MINUS, 1, 2, "-");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 3, "1");
  scan_assert_basic(LEXER_TOKEN_PLUS, 1, 5, "+");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 7, "2");
  scan_assert_basic(LEXER_TOKEN_CLOSE_PAREN, 1, 8, ")");
  scan_assert_basic(LEXER_TOKEN_STAR, 1, 10, "*");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 12, "3");
  scan_assert_basic(LEXER_TOKEN_MINUS, 1, 14, "-");
  scan_assert_basic(LEXER_TOKEN_MINUS, 1, 16, "-");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 17, "4");
  SCAN_ASSERT_EOF(1, 18);
}

static void test_input_source_code_2(void **const _) {
  reset_lexer(
    "var x = 5;\n"
    "var y = 10;\n"
    "print x + y;"
  );

  scan_assert_basic(LEXER_TOKEN_VAR, 1, 1, "var");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 1, 5, "x");
  scan_assert_basic(LEXER_TOKEN_EQUAL, 1, 7, "=");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 1, 9, "5");
  scan_assert_basic(LEXER_TOKEN_SEMICOLON, 1, 10, ";");
  scan_assert_basic(LEXER_TOKEN_VAR, 2, 1, "var");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 2, 5, "y");
  scan_assert_basic(LEXER_TOKEN_EQUAL, 2, 7, "=");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 2, 9, "10");
  scan_assert_basic(LEXER_TOKEN_SEMICOLON, 2, 11, ";");
  scan_assert_basic(LEXER_TOKEN_PRINT, 3, 1, "print");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 3, 7, "x");
  scan_assert_basic(LEXER_TOKEN_PLUS, 3, 9, "+");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 3, 11, "y");
  scan_assert_basic(LEXER_TOKEN_SEMICOLON, 3, 12, ";");
  SCAN_ASSERT_EOF(3, 13);
}

static void test_input_source_code_3(void **const _) {
  reset_lexer(
    "fun add(a, b) {\n"
    "  return a + b;\n"
    "}\n"
    "print add(2.5, 7.5);"
  );

  scan_assert_basic(LEXER_TOKEN_FUN, 1, 1, "fun");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 1, 5, "add");
  scan_assert_basic(LEXER_TOKEN_OPEN_PAREN, 1, 8, "(");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 1, 9, "a");
  scan_assert_basic(LEXER_TOKEN_COMMA, 1, 10, ",");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 1, 12, "b");
  scan_assert_basic(LEXER_TOKEN_CLOSE_PAREN, 1, 13, ")");
  scan_assert_basic(LEXER_TOKEN_OPEN_CURLY_BRACE, 1, 15, "{");
  scan_assert_basic(LEXER_TOKEN_RETURN, 2, 3, "return");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 2, 10, "a");
  scan_assert_basic(LEXER_TOKEN_PLUS, 2, 12, "+");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 2, 14, "b");
  scan_assert_basic(LEXER_TOKEN_SEMICOLON, 2, 15, ";");
  scan_assert_basic(LEXER_TOKEN_CLOSE_CURLY_BRACE, 3, 1, "}");
  scan_assert_basic(LEXER_TOKEN_PRINT, 4, 1, "print");
  scan_assert_basic(LEXER_TOKEN_IDENTIFIER, 4, 7, "add");
  scan_assert_basic(LEXER_TOKEN_OPEN_PAREN, 4, 10, "(");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 4, 11, "2.5");
  scan_assert_basic(LEXER_TOKEN_COMMA, 4, 14, ",");
  scan_assert_basic(LEXER_TOKEN_NUMBER, 4, 16, "7.5");
  scan_assert_basic(LEXER_TOKEN_CLOSE_PAREN, 4, 19, ")");
  scan_assert_basic(LEXER_TOKEN_SEMICOLON, 4, 20, ";");
  SCAN_ASSERT_EOF(4, 21);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(test_eof_token),
    cmocka_unit_test(test_whitespace),
    cmocka_unit_test(test_token_position_tracking),
    cmocka_unit_test(test_unexpected_char),
    cmocka_unit_test(test_string_literal),
    cmocka_unit_test(test_numeric_literal),
    cmocka_unit_test(test_identifier_literal),
    cmocka_unit_test(test_single_char_tokens),
    cmocka_unit_test(test_multi_char_tokens),
    cmocka_unit_test(test_keyword_tokens),
    cmocka_unit_test(test_comment),
    cmocka_unit_test(test_input_source_code_1),
    cmocka_unit_test(test_input_source_code_2),
    cmocka_unit_test(test_input_source_code_3),
  };

  return cmocka_run_group_tests(tests, setup_test_group_env, teardown_test_group_env);
}
