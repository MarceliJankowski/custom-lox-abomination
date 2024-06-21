#ifndef LEXER_H
#define LEXER_H

#include <assert.h>
#include <limits.h>
#include <stdint.h>

typedef enum {
  // indicators
  TOKEN_ERROR,
  TOKEN_EOF,
  TOKEN_INDICATOR_END, // assertion utility

  // literals
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_LITERAL_END, // assertion utility

  // single-character tokens
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_PERCENT,
  TOKEN_BANG,
  TOKEN_LESS,
  TOKEN_EQUAL,
  TOKEN_GREATER,
  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_QUESTION,
  TOKEN_OPEN_PAREN,
  TOKEN_CLOSE_PAREN,
  TOKEN_OPEN_CURLY_BRACE,
  TOKEN_CLOSE_CURLY_BRACE,
  TOKEN_SINGLE_CHAR_END, // assertion utility

  // multi-character tokens
  TOKEN_BANG_EQUAL,
  TOKEN_LESS_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER_EQUAL,
  TOKEN_MULTI_CHAR_END, // assertion utility

  // reserved identifiers (keywords)
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_VAR,
  TOKEN_NIL,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_FUN,
  TOKEN_RETURN,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FOR,
  TOKEN_CLASS,
  TOKEN_SUPER,
  TOKEN_THIS,
  TOKEN_PRINT,
  TOKEN_KEYWORD_END, // assertion utility

  // assertion utilities
  TOKEN_TYPE_COUNT = TOKEN_KEYWORD_END - 4, // -4 to account for assertion utility members
  TOKEN_INDICATOR_COUNT = TOKEN_INDICATOR_END,
  TOKEN_SINGLE_CHAR_COUNT = TOKEN_SINGLE_CHAR_END - TOKEN_LITERAL_END - 1,
  TOKEN_MULTI_CHAR_COUNT = TOKEN_MULTI_CHAR_END - TOKEN_SINGLE_CHAR_END - 1,
  TOKEN_KEYWORD_COUNT = TOKEN_KEYWORD_END - TOKEN_MULTI_CHAR_END - 1
} TokenType;

static_assert(TOKEN_TYPE_COUNT <= UCHAR_MAX, "Too many TokenTypes defined; Token.type can't fit all of them");

/**@desc lexeme bundled up with metadata about itself; smallest meaningful language unit*/
typedef struct {
  char const *lexeme;
  int32_t line;
  int column, lexeme_length;
  uint8_t type;
} Token;

void lexer_init(char const *source_code);
Token lexer_scan(void);

#endif // LEXER_H
