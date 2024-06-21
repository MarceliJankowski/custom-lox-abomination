#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "backend/value.h"
#include "common.h"
#include "global.h"
#include "util/debug.h"
#include "util/error.h"
#include "util/memory.h"

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

#define PUTS_BREAK(string) \
  puts(string);            \
  break

#define PRINTF_BREAK(...) \
  printf(__VA_ARGS__);    \
  break

// *---------------------------------------------*
// *               DEBUG FUNCTIONS               *
// *---------------------------------------------*

/**@desc print lexical `token`*/
void debug_token(Token const *const token) {
  printf(FILE_LINE_COLUMN_FORMAT " ", g_source_file, token->line, token->column);

  static_assert(TOKEN_TYPE_COUNT == 43, "Exhaustive TokenType handling");
  switch (token->type) {
    // indicators
    case TOKEN_ERROR: PRINTF_BREAK("TOKEN_ERROR");
    case TOKEN_EOF: PRINTF_BREAK("TOKEN_EOF");

    // literals
    case TOKEN_STRING: PRINTF_BREAK("TOKEN_STRING");
    case TOKEN_NUMBER: PRINTF_BREAK("TOKEN_NUMBER");
    case TOKEN_IDENTIFIER: PRINTF_BREAK("TOKEN_IDENTIFIER");

    // single-character tokens
    case TOKEN_PLUS: PRINTF_BREAK("TOKEN_PLUS");
    case TOKEN_MINUS: PRINTF_BREAK("TOKEN_MINUS");
    case TOKEN_STAR: PRINTF_BREAK("TOKEN_STAR");
    case TOKEN_SLASH: PRINTF_BREAK("TOKEN_SLASH");
    case TOKEN_PERCENT: PRINTF_BREAK("TOKEN_PERCENT");
    case TOKEN_BANG: PRINTF_BREAK("TOKEN_BANG");
    case TOKEN_LESS: PRINTF_BREAK("TOKEN_LESS");
    case TOKEN_EQUAL: PRINTF_BREAK("TOKEN_EQUAL");
    case TOKEN_GREATER: PRINTF_BREAK("TOKEN_GREATER");
    case TOKEN_DOT: PRINTF_BREAK("TOKEN_DOT");
    case TOKEN_COMMA: PRINTF_BREAK("TOKEN_COMMA");
    case TOKEN_COLON: PRINTF_BREAK("TOKEN_COLON");
    case TOKEN_SEMICOLON: PRINTF_BREAK("TOKEN_SEMICOLON");
    case TOKEN_QUESTION: PRINTF_BREAK("TOKEN_QUESTION");
    case TOKEN_OPEN_PAREN: PRINTF_BREAK("TOKEN_OPEN_PAREN");
    case TOKEN_CLOSE_PAREN: PRINTF_BREAK("TOKEN_CLOSE_PAREN");
    case TOKEN_OPEN_CURLY_BRACE: PRINTF_BREAK("TOKEN_OPEN_CURLY_BRACE");
    case TOKEN_CLOSE_CURLY_BRACE: PRINTF_BREAK("TOKEN_CLOSE_CURLY_BRACE");

    // multi-character tokens
    case TOKEN_BANG_EQUAL: PRINTF_BREAK("TOKEN_BANG_EQUAL");
    case TOKEN_LESS_EQUAL: PRINTF_BREAK("TOKEN_LESS_EQUAL");
    case TOKEN_EQUAL_EQUAL: PRINTF_BREAK("TOKEN_EQUAL_EQUAL");
    case TOKEN_GREATER_EQUAL: PRINTF_BREAK("TOKEN_GREATER_EQUAL");

    // reserved identifiers (keywords)
    case TOKEN_TRUE: PRINTF_BREAK("TOKEN_TRUE");
    case TOKEN_FALSE: PRINTF_BREAK("TOKEN_FALSE");
    case TOKEN_VAR: PRINTF_BREAK("TOKEN_VAR");
    case TOKEN_NIL: PRINTF_BREAK("TOKEN_NIL");
    case TOKEN_AND: PRINTF_BREAK("TOKEN_AND");
    case TOKEN_OR: PRINTF_BREAK("TOKEN_OR");
    case TOKEN_FUN: PRINTF_BREAK("TOKEN_FUN");
    case TOKEN_RETURN: PRINTF_BREAK("TOKEN_RETURN");
    case TOKEN_IF: PRINTF_BREAK("TOKEN_IF");
    case TOKEN_ELSE: PRINTF_BREAK("TOKEN_ELSE");
    case TOKEN_WHILE: PRINTF_BREAK("TOKEN_WHILE");
    case TOKEN_FOR: PRINTF_BREAK("TOKEN_FOR");
    case TOKEN_CLASS: PRINTF_BREAK("TOKEN_CLASS");
    case TOKEN_SUPER: PRINTF_BREAK("TOKEN_SUPER");
    case TOKEN_THIS: PRINTF_BREAK("TOKEN_THIS");
    case TOKEN_PRINT: PRINTF_BREAK("TOKEN_PRINT");

    default: INTERNAL_ERROR("Unknown token type '%d'", token->type);
  }

  printf(" '%.*s'\n", token->lexeme_length, token->lexeme);
}

/**@desc disassemble and print `chunk` annotated with `name`*/
void debug_disassemble_chunk(Chunk *const chunk, char const *const name) {
  assert(chunk != NULL);
  assert(name != NULL);

  printf("\n== %s ==\n", name);
  for (int32_t offset = 0; offset < chunk->count;) offset = debug_disassemble_instruction(chunk, offset);
}

/**@desc print simple instruction (one without operands) encoded by `opcode` and located at `offset`
@return offset to next instruction*/
static inline int32_t debug_simple_instruction(uint8_t const opcode, int32_t const offset) {
  static_assert(OP_SIMPLE_OPCODE_COUNT == 7, "Exhaustive simple opcode handling");
  switch (opcode) {
    case OP_RETURN: PUTS_BREAK("OP_RETURN");
    case OP_NEGATE: PUTS_BREAK("OP_NEGATE");
    case OP_ADD: PUTS_BREAK("OP_ADD");
    case OP_SUBTRACT: PUTS_BREAK("OP_SUBTRACT");
    case OP_MULTIPLY: PUTS_BREAK("OP_MULTIPLY");
    case OP_DIVIDE: PUTS_BREAK("OP_DIVIDE");
    case OP_MODULO: PUTS_BREAK("OP_MODULO");

    default: INTERNAL_ERROR("Unknown simple instruction opcode '%d'", opcode);
  }

  return offset + 1;
}

/**@desc print `chunk` constant instruction encoded by `opcode` and located at `offset`
@return offset to next instruction*/
static int32_t debug_constant_instruction(
  Chunk const *const chunk, uint8_t const opcode, int32_t const offset
) {
  assert(chunk != NULL);

  if (opcode == OP_CONSTANT) {
    uint8_t const constant = chunk->code[offset + 1];

    printf("OP_CONSTANT %d '", constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 2;
  }

  if (opcode == OP_CONSTANT_2B) {
    unsigned int const constant = concatenate_bytes(2, chunk->code[offset + 2], chunk->code[offset + 1]);

    printf("OP_CONSTANT_2B %d '", constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");

    return offset + 3;
  }

  INTERNAL_ERROR("Unknown constant instruction opcode '%d'", opcode);
}

/**@desc disassemble and print `chunk` instruction located at `offset`
@return offset to next instruction*/
int32_t debug_disassemble_instruction(Chunk *const chunk, int32_t const offset) {
  assert(chunk != NULL);
  assert(offset >= 0 && "Expected offset to be nonnegative");

  printf(FILE_LINE_FORMAT " ", g_source_file, chunk_get_instruction_line(chunk, offset));

  uint8_t const opcode = chunk->code[offset];

  static_assert(OP_OPCODE_COUNT == 9, "Exhaustive opcode handling");
  switch (opcode) {
    case OP_RETURN:
    case OP_NEGATE:
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_MODULO: {
      return debug_simple_instruction(opcode, offset);
    }
    case OP_CONSTANT:
    case OP_CONSTANT_2B: {
      return debug_constant_instruction(chunk, opcode, offset);
    }
    default: INTERNAL_ERROR("Unknown opcode '%d'", opcode);
  }
}
