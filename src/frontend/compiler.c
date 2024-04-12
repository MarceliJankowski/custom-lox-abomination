#include <stdio.h>

#include "frontend/lexer.h"

/**@desc compile `source_code` to bytecode instructions `(NOT IMPLEMENTED)`*/
void compiler_compile(char const *const source_code) {
  lexer_init(source_code);

  for (Token token = lexer_scan(); token.type != TOKEN_EOF; token = lexer_scan()) {
    printf("%ld %02d '%.*s'\n", token.line, token.type, token.lexeme_length, token.lexeme);
  }
}
