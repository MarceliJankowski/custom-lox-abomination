#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "frontend/lexer.h"

static struct {
  char const *char_cursor;
  char const *lexeme;
  long line;
} lexer;

/**@desc initialize lexer with `source_code`*/
void lexer_init(char const *const source_code) {
  assert(source_code != NULL);

  lexer.lexeme = source_code;
  lexer.char_cursor = source_code;
  lexer.line = 1;
}

/**@desc determine whether given `character` is a digit*/
static inline bool is_digit(char const character) {
  return character >= '0' && character <= '9';
}

/**@desc determine whether given `character` can begin identifier literal*/
static inline bool can_begin_identifier_literal(char const character) {
  return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

/**@desc determine whether given `character` can be a part of identifier literal*/
static inline bool can_constitute_identifier_literal(char const character) {
  return can_begin_identifier_literal(character) || is_digit(character);
}

/**@desc determine whether lexer reached source code end*/
static inline bool lexer_reached_end(void) {
  return *lexer.char_cursor == '\0';
}

/**@desc advance lexer.char_cursor to next source code character
@return previous (advanced past) character*/
static inline char lexer_advance(void) {
  return *lexer.char_cursor++;
}

/**@desc determine whether `expected_char` matches lexer.char_cursor and advance char_cursor if it does*/
static inline bool lexer_match(char const expected_char) {
  if (*lexer.char_cursor != expected_char) return false;
  lexer.char_cursor++;
  return true;
}

/**@desc peek at lexer.char_cursor character without advancing it
@return lexer.char_cursor character*/
static inline char lexer_peek(void) {
  return *lexer.char_cursor;
}

/**@desc peek at next lexer.char_cursor character without advancing char_cursor
@return upcoming source character or NUL if source end was reached*/
static inline char lexer_peek_next(void) {
  if (lexer_reached_end()) return '\0';
  return lexer.char_cursor[1];
}

/**@desc make `token_type` token
@return constructed token*/
static Token lexer_make_token(TokenType const token_type) {
  Token const token = {
    .type = token_type,
    .line = lexer.line,
    .lexeme = lexer.lexeme,
    .lexeme_length = lexer.char_cursor - lexer.lexeme,
  };

  return token;
}

/**@desc make error token with `message` lexeme
@return constructed error token*/
static Token lexer_make_error_token(char const *const message) {
  assert(message != NULL);

  Token const error_token = {
    .type = TOKEN_ERROR,
    .line = lexer.line,
    .lexeme = message,
    .lexeme_length = strlen(message),
  };

  return error_token;
}

/**@desc make EOF token
@return EOF token*/
static Token lexer_make_eof_token(void) {
  Token const eof_token = {
    .type = TOKEN_EOF,
    .line = lexer.line,
    .lexeme = "EOF",
    .lexeme_length = 3,
  };

  return eof_token;
}

/**@desc tokenize string literal
@return string literal token*/
static Token lexer_tokenize_string_literal(void) {
  // advance until closing quote
  while (lexer_peek() != '"') {
    if (lexer_reached_end()) return lexer_make_error_token("Unterminated string literal");
    if (lexer_advance() == '\n') lexer.line++;
  }

  lexer_advance(); // advance past closing quote

  return lexer_make_token(TOKEN_STRING);
}

/**@desc tokenize numeric literal
@return numeric literal token*/
static Token lexer_tokenize_numeric_literal(void) {
  // advance past integer part
  while (is_digit(lexer_peek())) lexer_advance();

  if (lexer_peek() == '.' && is_digit(lexer_peek_next())) {
    lexer_advance(); // advance past decimal point

    // advance past fractional part
    while (is_digit(lexer_peek())) lexer_advance();
  }

  return lexer_make_token(TOKEN_NUMBER);
}

/**@desc make either identifier or `keyword_type` (reserved identifier) token
@return `keyword_type` token if lexer.lexeme offsetted by `keyword_beginning_length` matches `keyword_rest`,
identifier token otherwise*/
static Token lexer_make_identifier_token(
  int const keyword_beginning_length, int const keyword_rest_length, const char *const keyword_rest,
  TokenType const keyword_type
) {
  assert(keyword_rest != NULL);
  assert(keyword_beginning_length > 0);
  assert(keyword_rest_length > 0);

  if (lexer.char_cursor - lexer.lexeme != keyword_beginning_length + keyword_rest_length) goto identifier;
  if (memcmp(lexer.lexeme + keyword_beginning_length, keyword_rest, keyword_rest_length)) goto identifier;

  // keyword
  return lexer_make_token(keyword_type);

identifier:
  return lexer_make_token(TOKEN_IDENTIFIER);
}

/**@desc tokenize identifier literal; handles both regular and reserved identifiers (keywords)
@return regular or reserved identifier token*/
static Token lexer_tokenize_identifier_literal(void) {
  // advance past identifier literal
  while (can_constitute_identifier_literal(lexer_peek())) lexer_advance();

  // determine if it's a regular or reserved identifier (algorithm mimics trie data structure)
  static_assert(TOKEN_KEYWORD_COUNT == 16, "Exhaustive keyword token handling");
  switch (*lexer.lexeme) {
    case 'a': return lexer_make_identifier_token(1, 2, "nd", TOKEN_AND);
    case 'c': return lexer_make_identifier_token(1, 4, "lass", TOKEN_CLASS);
    case 'e': return lexer_make_identifier_token(1, 3, "lse", TOKEN_ELSE);
    case 'f': {
      if (lexer.char_cursor - lexer.lexeme <= 1) break;
      switch (lexer.lexeme[1]) {
        case 'a': return lexer_make_identifier_token(2, 3, "lse", TOKEN_FALSE);
        case 'o': return lexer_make_identifier_token(2, 1, "r", TOKEN_FOR);
        case 'u': return lexer_make_identifier_token(2, 1, "n", TOKEN_FUN);
      }
      break;
    }
    case 'i': return lexer_make_identifier_token(1, 1, "f", TOKEN_IF);
    case 'n': return lexer_make_identifier_token(1, 2, "il", TOKEN_NIL);
    case 'o': return lexer_make_identifier_token(1, 1, "r", TOKEN_OR);
    case 'p': return lexer_make_identifier_token(1, 4, "rint", TOKEN_PRINT);
    case 'r': return lexer_make_identifier_token(1, 5, "eturn", TOKEN_RETURN);
    case 's': return lexer_make_identifier_token(1, 4, "uper", TOKEN_SUPER);
    case 't': {
      if (lexer.char_cursor - lexer.lexeme <= 1) break;
      switch (lexer.lexeme[1]) {
        case 'h': return lexer_make_identifier_token(2, 2, "is", TOKEN_THIS);
        case 'r': return lexer_make_identifier_token(2, 2, "ue", TOKEN_TRUE);
      }
      break;
    }
    case 'v': return lexer_make_identifier_token(1, 2, "ar", TOKEN_VAR);
    case 'w': return lexer_make_identifier_token(1, 4, "hile", TOKEN_WHILE);
  }

  // regular identifier
  return lexer_make_token(TOKEN_IDENTIFIER);
}

/**@desc advance past all whitespace characters*/
static void lexer_skip_whitespace(void) {
  for (;;) {
    switch (lexer_peek()) {
      case ' ':
      case '\t':
      case '\r': {
        lexer_advance();
        break;
      }
      case '\n': {
        lexer_advance();
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

/**@desc scan lexer source code for next lexeme, bundle it up with metadata, and produce new token
@return produced token*/
Token lexer_scan(void) {
  lexer_skip_whitespace();
  if (lexer_reached_end()) return lexer_make_eof_token();

  lexer.lexeme = lexer.char_cursor; // reset lexeme
  char const previous_char = lexer_advance();

  // literals
  if (previous_char == '"') return lexer_tokenize_string_literal();
  if (is_digit(previous_char)) return lexer_tokenize_numeric_literal();
  if (can_begin_identifier_literal(previous_char)) return lexer_tokenize_identifier_literal();

  static_assert(TOKEN_SINGLE_CHAR_COUNT == 18, "Exhaustive single-character token handling");
  static_assert(TOKEN_MULTI_CHAR_COUNT == 4, "Exhaustive multi-character token handling");
  switch (previous_char) {
    // single-character tokens
    case '+': return lexer_make_token(TOKEN_PLUS);
    case '-': return lexer_make_token(TOKEN_MINUS);
    case '*': return lexer_make_token(TOKEN_STAR);
    case '/': return lexer_make_token(TOKEN_SLASH);
    case '%': return lexer_make_token(TOKEN_PERCENT);
    case '(': return lexer_make_token(TOKEN_OPEN_PAREN);
    case ')': return lexer_make_token(TOKEN_CLOSE_PAREN);
    case '{': return lexer_make_token(TOKEN_OPEN_CURLY_BRACE);
    case '}': return lexer_make_token(TOKEN_CLOSE_CURLY_BRACE);
    case '.': return lexer_make_token(TOKEN_DOT);
    case ',': return lexer_make_token(TOKEN_COMMA);
    case '?': return lexer_make_token(TOKEN_QUESTION);
    case ':': return lexer_make_token(TOKEN_COLON);
    case ';': return lexer_make_token(TOKEN_SEMICOLON);

    // single-character or possibly multi-character tokens
    case '=': return lexer_make_token(lexer_match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '!': return lexer_make_token(lexer_match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '>': return lexer_make_token(lexer_match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '<': return lexer_make_token(lexer_match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);

    default: return lexer_make_error_token("Unexpected character");
  }
}
