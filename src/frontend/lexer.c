#include "frontend/lexer.h"

#include "utils/character.h"
#include "utils/debug.h"
#include "utils/io.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static struct {
  char const *char_cursor;
  char const *lexeme;
  int32_t line;
  int column, lexeme_start_line, lexeme_start_column;
} lexer;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Determine whether `character` can be a part of identifier literal.
/// @return true if it can, false otherwise.
static inline bool can_constitute_identifier_literal(char const character) {
  return character_is_alphanumeric(character) || character == '_';
}

/// Determine whether `character` can begin identifier literal.
/// @return true if it can, false otherwise.
static inline bool can_begin_identifier_literal(char const character) {
  return can_constitute_identifier_literal(character) && !character_is_digit(character);
}

/// Determine whether lexer reached source code end.
/// @return true if it did, false otherwise.
static inline bool lexer_reached_end(void) {
  return *lexer.char_cursor == '\0';
}

/// Get current lexeme length, up to (excluding) lexer.char_cursor character.
/// @return Current lexeme length.
static inline ptrdiff_t lexer_get_lexeme_length(void) {
  return lexer.char_cursor - lexer.lexeme;
}

/// Advance lexer.char_cursor to next source code character.
/// @return Previous (advanced past) character.
static inline char lexer_advance(void) {
  lexer.column++;
  return *lexer.char_cursor++;
}

/// Determine whether `expected_char` matches lexer.char_cursor and call lexer_advance if it does.
/// @return true if it does, false otherwise.
static inline bool lexer_match(char const expected_char) {
  if (*lexer.char_cursor != expected_char) return false;
  lexer_advance();
  return true;
}

/// Peek at lexer.char_cursor character without advancing it.
/// @return lexer.char_cursor character.
static inline char lexer_peek(void) {
  return *lexer.char_cursor;
}

/// Peek at next lexer.char_cursor character without advancing char_cursor.
/// @return Upcoming source character or NUL if source end was reached.
static inline char lexer_peek_next(void) {
  if (lexer_reached_end()) return '\0';
  return lexer.char_cursor[1];
}

/// Make basic `token_kind` token.
/// @return Made basic token.
static LexerToken lexer_make_basic_token(LexerTokenKind const token_kind) {
  LexerToken const token = {
    .kind = token_kind,
    .line = lexer.lexeme_start_line,
    .column = lexer.lexeme_start_column,
    .as.basic = {
      .lexeme = lexer.lexeme,
      .lexeme_length = lexer_get_lexeme_length(),
    }
  };

#ifdef DEBUG_LEXER
  debug_token(&token);
#endif

  return token;
}

/// Make error token with message created from `format` and `...`.
/// @param format String containing error message format.
/// @param ... Variadic arguments for `format`.
/// @return Made error token.
static LexerToken lexer_make_error_token(char const *const format, ...) {
  assert(format != NULL);

  char *message;
  {
    va_list format_args, format_args_copy;
    va_start(format_args, format);
    va_copy(format_args_copy, format_args);

    char dummy_buffer[1]; // required by snprintf spec
    int const message_length = vsnprintf(dummy_buffer, 0, format, format_args);
    if (message_length < 0) ERROR_IO_ERRNO();

    size_t const message_size = message_length + 1; // account for NUL terminator
    message = malloc(message_size);
    if (message == NULL) ERROR_MEMORY_ERRNO();
    int const bytes_printed = vsnprintf(message, message_size, format, format_args_copy);
    if (bytes_printed < 0 || bytes_printed != message_length) ERROR_IO_ERRNO();

    va_end(format_args);
    va_end(format_args_copy);
  }

  LexerToken const error_token = {
    .kind = LEXER_TOKEN_ERROR,
    .line = lexer.lexeme_start_line,
    .column = lexer.lexeme_start_column,
    .as.error.message = message,
  };

#ifdef DEBUG_LEXER
  debug_token(&error_token);
#endif

  return error_token;
}

/// Make EOF token.
/// @return EOF token.
static LexerToken lexer_make_eof_token(void) {
  LexerToken const eof_token = {
    .kind = LEXER_TOKEN_EOF,
    .line = lexer.lexeme_start_line,
    .column = lexer.lexeme_start_column,
    .as.basic = {
      .lexeme = "EOF",
      .lexeme_length = 3,
    }
  };

#ifdef DEBUG_LEXER
  debug_token(&eof_token);
#endif

  return eof_token;
}

/// Tokenize string literal.
/// @return String literal token.
static LexerToken lexer_tokenize_string_literal(void) {
  // advance until closing quote
  while (lexer_peek() != '"') {
    if (lexer_reached_end() || lexer_advance() == '\n') return lexer_make_error_token("Unterminated string literal");
  };

  lexer_advance(); // advance past closing quote

  return lexer_make_basic_token(LEXER_TOKEN_STRING);
}

/// Tokenize numeric literal.
/// @return Numeric literal token.
static LexerToken lexer_tokenize_numeric_literal(void) {
  // advance past integer part
  while (character_is_digit(lexer_peek())) lexer_advance();

  if (lexer_peek() == '.' && character_is_digit(lexer_peek_next())) {
    lexer_advance(); // advance past decimal point

    // advance past fractional part
    while (character_is_digit(lexer_peek())) lexer_advance();
  }

  return lexer_make_basic_token(LEXER_TOKEN_NUMBER);
}

/// Make either identifier or `keyword_kind` (reserved identifier) token.
/// @return `keyword_kind` token if lexer.lexeme offsetted by `keyword_beginning_length` matches `keyword_rest`,
/// identifier token otherwise.
static LexerToken lexer_make_identifier_token(
  int const keyword_beginning_length, int const keyword_rest_length, const char *const keyword_rest,
  LexerTokenKind const keyword_kind
) {
  assert(keyword_rest != NULL);
  assert(keyword_beginning_length > 0);
  assert(keyword_rest_length > 0);

  if (lexer_get_lexeme_length() != keyword_beginning_length + keyword_rest_length) goto handle_identifier;
  if (memcmp(lexer.lexeme + keyword_beginning_length, keyword_rest, keyword_rest_length)) goto handle_identifier;

  // keyword
  return lexer_make_basic_token(keyword_kind);

handle_identifier:
  return lexer_make_basic_token(LEXER_TOKEN_IDENTIFIER);
}

/// Tokenize identifier literal; handles both regular and reserved identifiers (keywords).
/// @return Regular or reserved identifier token.
static LexerToken lexer_tokenize_identifier_literal(void) {
  // advance past identifier literal
  while (can_constitute_identifier_literal(lexer_peek())) lexer_advance();

  // determine if it's a regular or reserved identifier (algorithm mimics trie data structure)
  static_assert(LEXER_TOKEN_KEYWORD_COUNT == 16, "Exhaustive keyword token handling");
  switch (*lexer.lexeme) {
    case 'a': return lexer_make_identifier_token(1, 2, "nd", LEXER_TOKEN_AND);
    case 'c': return lexer_make_identifier_token(1, 4, "lass", LEXER_TOKEN_CLASS);
    case 'e': return lexer_make_identifier_token(1, 3, "lse", LEXER_TOKEN_ELSE);
    case 'f': {
      if (lexer_get_lexeme_length() <= 1) break;
      switch (lexer.lexeme[1]) {
        case 'a': return lexer_make_identifier_token(2, 3, "lse", LEXER_TOKEN_FALSE);
        case 'o': return lexer_make_identifier_token(2, 1, "r", LEXER_TOKEN_FOR);
        case 'u': return lexer_make_identifier_token(2, 1, "n", LEXER_TOKEN_FUN);
      }
      break;
    }
    case 'i': return lexer_make_identifier_token(1, 1, "f", LEXER_TOKEN_IF);
    case 'n': return lexer_make_identifier_token(1, 2, "il", LEXER_TOKEN_NIL);
    case 'o': return lexer_make_identifier_token(1, 1, "r", LEXER_TOKEN_OR);
    case 'p': return lexer_make_identifier_token(1, 4, "rint", LEXER_TOKEN_PRINT);
    case 'r': return lexer_make_identifier_token(1, 5, "eturn", LEXER_TOKEN_RETURN);
    case 's': return lexer_make_identifier_token(1, 4, "uper", LEXER_TOKEN_SUPER);
    case 't': {
      if (lexer_get_lexeme_length() <= 1) break;
      switch (lexer.lexeme[1]) {
        case 'h': return lexer_make_identifier_token(2, 2, "is", LEXER_TOKEN_THIS);
        case 'r': return lexer_make_identifier_token(2, 2, "ue", LEXER_TOKEN_TRUE);
      }
      break;
    }
    case 'v': return lexer_make_identifier_token(1, 2, "ar", LEXER_TOKEN_VAR);
    case 'w': return lexer_make_identifier_token(1, 4, "hile", LEXER_TOKEN_WHILE);
  }

  // regular identifier
  return lexer_make_basic_token(LEXER_TOKEN_IDENTIFIER);
}

/// Advance past all whitespace characters.
static void lexer_skip_whitespace(void) {
  for (;;) {
    switch (lexer_peek()) {
      case ' ':
      case '\f':
      case '\t':
      case '\v':
      case '\r': {
        lexer_advance();
        break;
      }
      case '\n': {
        lexer_advance();
        lexer.column = 1;
        lexer.line++;
        break;
      }
      case '#': {
        do lexer_advance();
        while (lexer_peek() != '\n' && !lexer_reached_end());
        break;
      }
      default: return;
    }
  }
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Initialize lexer with `source_code`.
void lexer_init(char const *const source_code) {
  assert(source_code != NULL);

#ifdef DEBUG_LEXER
  io_puts("== DEBUG_LEXER ==");
#endif

  lexer.char_cursor = source_code;
  lexer.lexeme = source_code;
  lexer.line = 1;
  lexer.column = 1;
}

/// Scan lexer source code for next lexeme, bundle it up with metadata, and produce new token.
/// @pre Lexer is initialized.
/// @return Produced token.
LexerToken lexer_scan(void) {
  lexer_skip_whitespace();

  // reset lexeme
  lexer.lexeme = lexer.char_cursor;
  lexer.lexeme_start_line = lexer.line;
  lexer.lexeme_start_column = lexer.column;

  if (lexer_reached_end()) return lexer_make_eof_token();

  char const previous_char = lexer_advance();

  // literals
  if (previous_char == '"') return lexer_tokenize_string_literal();
  if (character_is_digit(previous_char)) return lexer_tokenize_numeric_literal();
  if (can_begin_identifier_literal(previous_char)) return lexer_tokenize_identifier_literal();

  static_assert(LEXER_TOKEN_SINGLE_CHAR_COUNT == 18, "Exhaustive single-character token handling");
  static_assert(LEXER_TOKEN_MULTI_CHAR_COUNT == 5, "Exhaustive multi-character token handling");
  switch (previous_char) {
    // single-character tokens
    case '+': return lexer_make_basic_token(LEXER_TOKEN_PLUS);
    case '-': return lexer_make_basic_token(LEXER_TOKEN_MINUS);
    case '*': return lexer_make_basic_token(LEXER_TOKEN_STAR);
    case '/': return lexer_make_basic_token(LEXER_TOKEN_SLASH);
    case '%': return lexer_make_basic_token(LEXER_TOKEN_PERCENT);
    case '(': return lexer_make_basic_token(LEXER_TOKEN_OPEN_PAREN);
    case ')': return lexer_make_basic_token(LEXER_TOKEN_CLOSE_PAREN);
    case '{': return lexer_make_basic_token(LEXER_TOKEN_OPEN_CURLY_BRACE);
    case '}': return lexer_make_basic_token(LEXER_TOKEN_CLOSE_CURLY_BRACE);
    case ',': return lexer_make_basic_token(LEXER_TOKEN_COMMA);
    case '?': return lexer_make_basic_token(LEXER_TOKEN_QUESTION);
    case ':': return lexer_make_basic_token(LEXER_TOKEN_COLON);
    case ';': return lexer_make_basic_token(LEXER_TOKEN_SEMICOLON);

    // single-character or possibly multi-character tokens
    case '.': return lexer_make_basic_token(lexer_match('.') ? LEXER_TOKEN_DOT_DOT : LEXER_TOKEN_DOT);
    case '=': return lexer_make_basic_token(lexer_match('=') ? LEXER_TOKEN_EQUAL_EQUAL : LEXER_TOKEN_EQUAL);
    case '!': return lexer_make_basic_token(lexer_match('=') ? LEXER_TOKEN_BANG_EQUAL : LEXER_TOKEN_BANG);
    case '>': return lexer_make_basic_token(lexer_match('=') ? LEXER_TOKEN_GREATER_EQUAL : LEXER_TOKEN_GREATER);
    case '<': return lexer_make_basic_token(lexer_match('=') ? LEXER_TOKEN_LESS_EQUAL : LEXER_TOKEN_LESS);

    default: return lexer_make_error_token("Unexpected character '%c'", previous_char);
  }
}

/// Release `token` resources.
void lexer_token_free(LexerToken const token) {
  switch (token.kind) {
    case LEXER_TOKEN_ERROR: {
      free(token.as.error.message);
      break;
    }
    default: break;
  }
}
