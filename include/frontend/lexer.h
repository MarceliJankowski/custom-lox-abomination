#ifndef LEXER_H
#define LEXER_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef enum {
  // indicators
  LEXER_TOKEN_ERROR,
  LEXER_TOKEN_EOF,
  LEXER_TOKEN_INDICATOR_END, // assertion utility

  // literals
  LEXER_TOKEN_STRING,
  LEXER_TOKEN_NUMBER,
  LEXER_TOKEN_IDENTIFIER,
  LEXER_TOKEN_LITERAL_END, // assertion utility

  // single-character tokens
  LEXER_TOKEN_PLUS,
  LEXER_TOKEN_MINUS,
  LEXER_TOKEN_STAR,
  LEXER_TOKEN_SLASH,
  LEXER_TOKEN_PERCENT,
  LEXER_TOKEN_BANG,
  LEXER_TOKEN_LESS,
  LEXER_TOKEN_EQUAL,
  LEXER_TOKEN_GREATER,
  LEXER_TOKEN_DOT,
  LEXER_TOKEN_COMMA,
  LEXER_TOKEN_COLON,
  LEXER_TOKEN_SEMICOLON,
  LEXER_TOKEN_QUESTION,
  LEXER_TOKEN_OPEN_PAREN,
  LEXER_TOKEN_CLOSE_PAREN,
  LEXER_TOKEN_OPEN_CURLY_BRACE,
  LEXER_TOKEN_CLOSE_CURLY_BRACE,
  LEXER_TOKEN_SINGLE_CHAR_END, // assertion utility

  // multi-character tokens
  LEXER_TOKEN_DOT_DOT,
  LEXER_TOKEN_EQUAL_EQUAL,
  LEXER_TOKEN_BANG_EQUAL,
  LEXER_TOKEN_LESS_EQUAL,
  LEXER_TOKEN_GREATER_EQUAL,
  LEXER_TOKEN_MULTI_CHAR_END, // assertion utility

  // reserved identifiers (keywords)
  LEXER_TOKEN_TRUE,
  LEXER_TOKEN_FALSE,
  LEXER_TOKEN_VAR,
  LEXER_TOKEN_NIL,
  LEXER_TOKEN_AND,
  LEXER_TOKEN_OR,
  LEXER_TOKEN_FUN,
  LEXER_TOKEN_RETURN,
  LEXER_TOKEN_IF,
  LEXER_TOKEN_ELSE,
  LEXER_TOKEN_WHILE,
  LEXER_TOKEN_FOR,
  LEXER_TOKEN_CLASS,
  LEXER_TOKEN_SUPER,
  LEXER_TOKEN_THIS,
  LEXER_TOKEN_PRINT,
  LEXER_TOKEN_KEYWORD_END, // assertion utility

  // assertion utilities
  LEXER_TOKEN_KIND_COUNT = LEXER_TOKEN_KEYWORD_END - 4, // -4 to account for assertion utility members
  LEXER_TOKEN_INDICATOR_COUNT = LEXER_TOKEN_INDICATOR_END,
  LEXER_TOKEN_SINGLE_CHAR_COUNT = LEXER_TOKEN_SINGLE_CHAR_END - LEXER_TOKEN_LITERAL_END - 1,
  LEXER_TOKEN_MULTI_CHAR_COUNT = LEXER_TOKEN_MULTI_CHAR_END - LEXER_TOKEN_SINGLE_CHAR_END - 1,
  LEXER_TOKEN_KEYWORD_COUNT = LEXER_TOKEN_KEYWORD_END - LEXER_TOKEN_MULTI_CHAR_END - 1
} LexerTokenKind;

static_assert(
  LEXER_TOKEN_KIND_COUNT <= UCHAR_MAX, "Too many LexerTokenKinds defined; LexerToken.kind can't fit all of them"
);

/// Smallest meaningful language unit.
/// In its `.as.basic` form it is a lexeme bundled up with metadata about itself.
/// Other forms exist for specialized token kinds.
typedef struct {
  uint8_t kind;
  int column;
  int32_t line;
  union {
    struct {
      int lexeme_length;
      char const *lexeme; // character sequence
    } basic;
    struct {
      char *message; // string
    } error;
  } as;
} LexerToken;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void lexer_init(char const *source_code);
LexerToken lexer_scan(void);
void lexer_token_free(LexerToken const token);

#endif // LEXER_H
