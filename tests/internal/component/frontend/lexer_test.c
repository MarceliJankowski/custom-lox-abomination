#include "frontend/lexer.h"

#include "component/component_test.h"

#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define SCAN_ASSERT_EOF() scan_assert(LEXER_TOKEN_EOF, "EOF")
#define SCAN_ASSERT_ALL_EOF(expected_line, expected_column) \
  scan_assert_all(LEXER_TOKEN_EOF, "EOF", expected_line, expected_column)

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

static LexerToken scan_assert(LexerTokenType const expected_type, char const *const expected_lexeme) {
  LexerToken const token = lexer_scan();
  size_t const expected_lexeme_length = strlen(expected_lexeme);

  assert_int_equal(token.type, expected_type);
  assert_int_equal(token.lexeme_length, expected_lexeme_length);
  assert_memory_equal(token.lexeme, expected_lexeme, expected_lexeme_length);
  return token;
}

static inline void assert_position(LexerToken const token, int const expected_line, int const expected_column) {
  assert_int_equal(token.line, expected_line);
  assert_int_equal(token.column, expected_column);
}

static LexerToken scan_assert_all(
  LexerTokenType const expected_type, char const *const expected_lexeme, int const expected_line,
  int const expected_column
) {
  LexerToken const token = scan_assert(expected_type, expected_lexeme);
  assert_position(token, expected_line, expected_column);
  return token;
}

static inline void init_scan_assert(char const *const lexeme, LexerTokenType const expected_type) {
  lexer_init(lexeme);
  scan_assert(expected_type, lexeme);
  SCAN_ASSERT_EOF();
}

static inline void init_SCAN_ASSERT_EOF(char const *const lexeme) {
  lexer_init(lexeme);
  SCAN_ASSERT_EOF();
}

static inline void init_scan_assert_error(char const *const source_code, char const *const error_lexeme) {
  lexer_init(source_code);
  scan_assert(LEXER_TOKEN_ERROR, error_lexeme);
  SCAN_ASSERT_EOF();
}

// *---------------------------------------------*
// *                 TEST CASES                  *
// *---------------------------------------------*

static void test_eof_token(void **const _) {
  lexer_init("");

  // continually returns EOF token upon reaching NUL byte
  for (int i = 0; i < 3; i++) SCAN_ASSERT_EOF();
}

static void test_whitespace(void **const _) {
  // skips whitespace
  init_SCAN_ASSERT_EOF(" ");
  init_SCAN_ASSERT_EOF("\t");
  init_SCAN_ASSERT_EOF("\r");
  init_SCAN_ASSERT_EOF("\n");
  init_SCAN_ASSERT_EOF(" \t \r \n ");
}

static void test_unexpected_char(void **const _) {
  init_scan_assert_error("`", "Unexpected character");
  init_scan_assert_error("~", "Unexpected character");
  init_scan_assert_error("@", "Unexpected character");
  init_scan_assert_error("$", "Unexpected character");
  init_scan_assert_error("^", "Unexpected character");
  init_scan_assert_error("&", "Unexpected character");
  init_scan_assert_error("[", "Unexpected character");
  init_scan_assert_error("]", "Unexpected character");
  init_scan_assert_error("|", "Unexpected character");
  init_scan_assert_error("\\", "Unexpected character");
  init_scan_assert_error("'", "Unexpected character");
}

static void test_position_tracking(void **const _) {
  // line
  lexer_init(""), assert_int_equal(lexer_scan().line, 1);
  lexer_init("\n"), assert_int_equal(lexer_scan().line, 2);
  lexer_init("\r\n"), assert_int_equal(lexer_scan().line, 2);
  lexer_init("\n\n"), assert_int_equal(lexer_scan().line, 3);

  // column
  lexer_init(""), assert_int_equal(lexer_scan().column, 1);
  lexer_init(" "), assert_int_equal(lexer_scan().column, 2);
  lexer_init("  "), assert_int_equal(lexer_scan().column, 3);
  lexer_init("\t"), assert_int_equal(lexer_scan().column, 2);
  lexer_init("\r"), assert_int_equal(lexer_scan().column, 2);

  // line and column
  lexer_init("   \n"), assert_position(lexer_scan(), 2, 1);
  lexer_init("   \n   "), assert_position(lexer_scan(), 2, 4);
  lexer_init("1 \n 2"), assert_position(lexer_scan(), 1, 1), assert_position(lexer_scan(), 2, 2);
}

static void test_string_literal(void **const _) {
  init_scan_assert("\"abc\"", LEXER_TOKEN_STRING);
  init_scan_assert_error("\"abc", "Unterminated string literal");

  // string literal spanning multiple lines
  init_scan_assert("\"abc\ndef\"", LEXER_TOKEN_STRING);
  init_scan_assert_error("\"abc\ndef", "Unterminated string literal");
}

static void test_numeric_literal(void **const _) {
  init_scan_assert("55", LEXER_TOKEN_NUMBER);
  init_scan_assert("10.25", LEXER_TOKEN_NUMBER);

  lexer_init("-55");
  scan_assert(LEXER_TOKEN_MINUS, "-");
  scan_assert(LEXER_TOKEN_NUMBER, "55");
  SCAN_ASSERT_EOF();

  lexer_init("-10.25");
  scan_assert(LEXER_TOKEN_MINUS, "-");
  scan_assert(LEXER_TOKEN_NUMBER, "10.25");
  SCAN_ASSERT_EOF();

  lexer_init("4.");
  scan_assert(LEXER_TOKEN_NUMBER, "4");
  scan_assert(LEXER_TOKEN_DOT, ".");
  SCAN_ASSERT_EOF();

  lexer_init(".5");
  scan_assert(LEXER_TOKEN_DOT, ".");
  scan_assert(LEXER_TOKEN_NUMBER, "5");
  SCAN_ASSERT_EOF();
}

static void test_identifier_literal(void **const _) {
  init_scan_assert("_", LEXER_TOKEN_IDENTIFIER);
  init_scan_assert("_name", LEXER_TOKEN_IDENTIFIER);
  init_scan_assert("name_123", LEXER_TOKEN_IDENTIFIER);
  init_scan_assert("name123", LEXER_TOKEN_IDENTIFIER);
  init_scan_assert("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_", LEXER_TOKEN_IDENTIFIER);
}

static void test_single_char_tokens(void **const _) {
  static_assert(LEXER_TOKEN_SINGLE_CHAR_COUNT == 18, "Exhaustive single-character token handling");
  init_scan_assert("+", LEXER_TOKEN_PLUS);
  init_scan_assert("-", LEXER_TOKEN_MINUS);
  init_scan_assert("*", LEXER_TOKEN_STAR);
  init_scan_assert("/", LEXER_TOKEN_SLASH);
  init_scan_assert("%", LEXER_TOKEN_PERCENT);
  init_scan_assert("!", LEXER_TOKEN_BANG);
  init_scan_assert("<", LEXER_TOKEN_LESS);
  init_scan_assert("=", LEXER_TOKEN_EQUAL);
  init_scan_assert(">", LEXER_TOKEN_GREATER);
  init_scan_assert(".", LEXER_TOKEN_DOT);
  init_scan_assert(",", LEXER_TOKEN_COMMA);
  init_scan_assert(":", LEXER_TOKEN_COLON);
  init_scan_assert(";", LEXER_TOKEN_SEMICOLON);
  init_scan_assert("?", LEXER_TOKEN_QUESTION);
  init_scan_assert("(", LEXER_TOKEN_OPEN_PAREN);
  init_scan_assert(")", LEXER_TOKEN_CLOSE_PAREN);
  init_scan_assert("{", LEXER_TOKEN_OPEN_CURLY_BRACE);
  init_scan_assert("}", LEXER_TOKEN_CLOSE_CURLY_BRACE);
}

static void test_multi_char_tokens(void **const _) {
  static_assert(LEXER_TOKEN_MULTI_CHAR_COUNT == 4, "Exhaustive multi-character token handling");
  init_scan_assert("!=", LEXER_TOKEN_BANG_EQUAL);
  init_scan_assert("<=", LEXER_TOKEN_LESS_EQUAL);
  init_scan_assert("==", LEXER_TOKEN_EQUAL_EQUAL);
  init_scan_assert(">=", LEXER_TOKEN_GREATER_EQUAL);
}

static void test_keyword_tokens(void **const _) {
  static_assert(LEXER_TOKEN_KEYWORD_COUNT == 16, "Exhaustive keyword token handling");
  init_scan_assert("true", LEXER_TOKEN_TRUE);
  init_scan_assert("false", LEXER_TOKEN_FALSE);
  init_scan_assert("var", LEXER_TOKEN_VAR);
  init_scan_assert("nil", LEXER_TOKEN_NIL);
  init_scan_assert("and", LEXER_TOKEN_AND);
  init_scan_assert("or", LEXER_TOKEN_OR);
  init_scan_assert("fun", LEXER_TOKEN_FUN);
  init_scan_assert("return", LEXER_TOKEN_RETURN);
  init_scan_assert("if", LEXER_TOKEN_IF);
  init_scan_assert("else", LEXER_TOKEN_ELSE);
  init_scan_assert("while", LEXER_TOKEN_WHILE);
  init_scan_assert("for", LEXER_TOKEN_FOR);
  init_scan_assert("class", LEXER_TOKEN_CLASS);
  init_scan_assert("super", LEXER_TOKEN_SUPER);
  init_scan_assert("this", LEXER_TOKEN_THIS);
  init_scan_assert("print", LEXER_TOKEN_PRINT);
}

static void test_comment(void **const _) {
  init_SCAN_ASSERT_EOF("# comment");
  init_SCAN_ASSERT_EOF("# comment... # continues...");
  lexer_init("# comment spans single line\n +"), scan_assert(LEXER_TOKEN_PLUS, "+");
}

static void test_input_source_code_1(void **const _) {
  lexer_init("(-1 + 2) * 3 - -4");

  scan_assert_all(LEXER_TOKEN_OPEN_PAREN, "(", 1, 1);
  scan_assert_all(LEXER_TOKEN_MINUS, "-", 1, 2);
  scan_assert_all(LEXER_TOKEN_NUMBER, "1", 1, 3);
  scan_assert_all(LEXER_TOKEN_PLUS, "+", 1, 5);
  scan_assert_all(LEXER_TOKEN_NUMBER, "2", 1, 7);
  scan_assert_all(LEXER_TOKEN_CLOSE_PAREN, ")", 1, 8);
  scan_assert_all(LEXER_TOKEN_STAR, "*", 1, 10);
  scan_assert_all(LEXER_TOKEN_NUMBER, "3", 1, 12);
  scan_assert_all(LEXER_TOKEN_MINUS, "-", 1, 14);
  scan_assert_all(LEXER_TOKEN_MINUS, "-", 1, 16);
  scan_assert_all(LEXER_TOKEN_NUMBER, "4", 1, 17);
  SCAN_ASSERT_ALL_EOF(1, 18);
}

static void test_input_source_code_2(void **const _) {
  lexer_init(
    "var x = 5;\n"
    "var y = 10;\n"
    "print x + y;"
  );

  scan_assert_all(LEXER_TOKEN_VAR, "var", 1, 1);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "x", 1, 5);
  scan_assert_all(LEXER_TOKEN_EQUAL, "=", 1, 7);
  scan_assert_all(LEXER_TOKEN_NUMBER, "5", 1, 9);
  scan_assert_all(LEXER_TOKEN_SEMICOLON, ";", 1, 10);
  scan_assert_all(LEXER_TOKEN_VAR, "var", 2, 1);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "y", 2, 5);
  scan_assert_all(LEXER_TOKEN_EQUAL, "=", 2, 7);
  scan_assert_all(LEXER_TOKEN_NUMBER, "10", 2, 9);
  scan_assert_all(LEXER_TOKEN_SEMICOLON, ";", 2, 11);
  scan_assert_all(LEXER_TOKEN_PRINT, "print", 3, 1);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "x", 3, 7);
  scan_assert_all(LEXER_TOKEN_PLUS, "+", 3, 9);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "y", 3, 11);
  scan_assert_all(LEXER_TOKEN_SEMICOLON, ";", 3, 12);
  SCAN_ASSERT_ALL_EOF(3, 13);
}

static void test_input_source_code_3(void **const _) {
  lexer_init(
    "fun add(a, b) {\n"
    "  return a + b;\n"
    "}\n"
    "print add(2.5, 7.5);"
  );

  scan_assert_all(LEXER_TOKEN_FUN, "fun", 1, 1);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "add", 1, 5);
  scan_assert_all(LEXER_TOKEN_OPEN_PAREN, "(", 1, 8);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "a", 1, 9);
  scan_assert_all(LEXER_TOKEN_COMMA, ",", 1, 10);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "b", 1, 12);
  scan_assert_all(LEXER_TOKEN_CLOSE_PAREN, ")", 1, 13);
  scan_assert_all(LEXER_TOKEN_OPEN_CURLY_BRACE, "{", 1, 15);
  scan_assert_all(LEXER_TOKEN_RETURN, "return", 2, 3);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "a", 2, 10);
  scan_assert_all(LEXER_TOKEN_PLUS, "+", 2, 12);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "b", 2, 14);
  scan_assert_all(LEXER_TOKEN_SEMICOLON, ";", 2, 15);
  scan_assert_all(LEXER_TOKEN_CLOSE_CURLY_BRACE, "}", 3, 1);
  scan_assert_all(LEXER_TOKEN_PRINT, "print", 4, 1);
  scan_assert_all(LEXER_TOKEN_IDENTIFIER, "add", 4, 7);
  scan_assert_all(LEXER_TOKEN_OPEN_PAREN, "(", 4, 10);
  scan_assert_all(LEXER_TOKEN_NUMBER, "2.5", 4, 11);
  scan_assert_all(LEXER_TOKEN_COMMA, ",", 4, 14);
  scan_assert_all(LEXER_TOKEN_NUMBER, "7.5", 4, 16);
  scan_assert_all(LEXER_TOKEN_CLOSE_PAREN, ")", 4, 19);
  scan_assert_all(LEXER_TOKEN_SEMICOLON, ";", 4, 20);
  SCAN_ASSERT_ALL_EOF(4, 21);
}

int main(void) {
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(test_eof_token),
    cmocka_unit_test(test_whitespace),
    cmocka_unit_test(test_unexpected_char),
    cmocka_unit_test(test_position_tracking),
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

  return cmocka_run_group_tests(tests, NULL, NULL);
}
