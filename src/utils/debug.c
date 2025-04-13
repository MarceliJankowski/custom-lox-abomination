#include "utils/debug.h"

#include "backend/value.h"
#include "common.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/**@desc print lexical `token`*/
void debug_token(LexerToken const *const token) {
  printf(COMMON_FILE_LINE_COLUMN_FORMAT " ", g_source_file, token->line, token->column);

  static_assert(LEXER_TOKEN_TYPE_COUNT == 43, "Exhaustive LexerTokenType handling");
  switch (token->type) {
    // indicators
    case LEXER_TOKEN_ERROR: IO_PRINTF_BREAK("LEXER_TOKEN_ERROR");
    case LEXER_TOKEN_EOF: IO_PRINTF_BREAK("LEXER_TOKEN_EOF");

    // literals
    case LEXER_TOKEN_STRING: IO_PRINTF_BREAK("LEXER_TOKEN_STRING");
    case LEXER_TOKEN_NUMBER: IO_PRINTF_BREAK("LEXER_TOKEN_NUMBER");
    case LEXER_TOKEN_IDENTIFIER: IO_PRINTF_BREAK("LEXER_TOKEN_IDENTIFIER");

    // single-character tokens
    case LEXER_TOKEN_PLUS: IO_PRINTF_BREAK("LEXER_TOKEN_PLUS");
    case LEXER_TOKEN_MINUS: IO_PRINTF_BREAK("LEXER_TOKEN_MINUS");
    case LEXER_TOKEN_STAR: IO_PRINTF_BREAK("LEXER_TOKEN_STAR");
    case LEXER_TOKEN_SLASH: IO_PRINTF_BREAK("LEXER_TOKEN_SLASH");
    case LEXER_TOKEN_PERCENT: IO_PRINTF_BREAK("LEXER_TOKEN_PERCENT");
    case LEXER_TOKEN_BANG: IO_PRINTF_BREAK("LEXER_TOKEN_BANG");
    case LEXER_TOKEN_LESS: IO_PRINTF_BREAK("LEXER_TOKEN_LESS");
    case LEXER_TOKEN_EQUAL: IO_PRINTF_BREAK("LEXER_TOKEN_EQUAL");
    case LEXER_TOKEN_GREATER: IO_PRINTF_BREAK("LEXER_TOKEN_GREATER");
    case LEXER_TOKEN_DOT: IO_PRINTF_BREAK("LEXER_TOKEN_DOT");
    case LEXER_TOKEN_COMMA: IO_PRINTF_BREAK("LEXER_TOKEN_COMMA");
    case LEXER_TOKEN_COLON: IO_PRINTF_BREAK("LEXER_TOKEN_COLON");
    case LEXER_TOKEN_SEMICOLON: IO_PRINTF_BREAK("LEXER_TOKEN_SEMICOLON");
    case LEXER_TOKEN_QUESTION: IO_PRINTF_BREAK("LEXER_TOKEN_QUESTION");
    case LEXER_TOKEN_OPEN_PAREN: IO_PRINTF_BREAK("LEXER_TOKEN_OPEN_PAREN");
    case LEXER_TOKEN_CLOSE_PAREN: IO_PRINTF_BREAK("LEXER_TOKEN_CLOSE_PAREN");
    case LEXER_TOKEN_OPEN_CURLY_BRACE: IO_PRINTF_BREAK("LEXER_TOKEN_OPEN_CURLY_BRACE");
    case LEXER_TOKEN_CLOSE_CURLY_BRACE: IO_PRINTF_BREAK("LEXER_TOKEN_CLOSE_CURLY_BRACE");

    // multi-character tokens
    case LEXER_TOKEN_BANG_EQUAL: IO_PRINTF_BREAK("LEXER_TOKEN_BANG_EQUAL");
    case LEXER_TOKEN_LESS_EQUAL: IO_PRINTF_BREAK("LEXER_TOKEN_LESS_EQUAL");
    case LEXER_TOKEN_EQUAL_EQUAL: IO_PRINTF_BREAK("LEXER_TOKEN_EQUAL_EQUAL");
    case LEXER_TOKEN_GREATER_EQUAL: IO_PRINTF_BREAK("LEXER_TOKEN_GREATER_EQUAL");

    // reserved identifiers (keywords)
    case LEXER_TOKEN_TRUE: IO_PRINTF_BREAK("LEXER_TOKEN_TRUE");
    case LEXER_TOKEN_FALSE: IO_PRINTF_BREAK("LEXER_TOKEN_FALSE");
    case LEXER_TOKEN_VAR: IO_PRINTF_BREAK("LEXER_TOKEN_VAR");
    case LEXER_TOKEN_NIL: IO_PRINTF_BREAK("LEXER_TOKEN_NIL");
    case LEXER_TOKEN_AND: IO_PRINTF_BREAK("LEXER_TOKEN_AND");
    case LEXER_TOKEN_OR: IO_PRINTF_BREAK("LEXER_TOKEN_OR");
    case LEXER_TOKEN_FUN: IO_PRINTF_BREAK("LEXER_TOKEN_FUN");
    case LEXER_TOKEN_RETURN: IO_PRINTF_BREAK("LEXER_TOKEN_RETURN");
    case LEXER_TOKEN_IF: IO_PRINTF_BREAK("LEXER_TOKEN_IF");
    case LEXER_TOKEN_ELSE: IO_PRINTF_BREAK("LEXER_TOKEN_ELSE");
    case LEXER_TOKEN_WHILE: IO_PRINTF_BREAK("LEXER_TOKEN_WHILE");
    case LEXER_TOKEN_FOR: IO_PRINTF_BREAK("LEXER_TOKEN_FOR");
    case LEXER_TOKEN_CLASS: IO_PRINTF_BREAK("LEXER_TOKEN_CLASS");
    case LEXER_TOKEN_SUPER: IO_PRINTF_BREAK("LEXER_TOKEN_SUPER");
    case LEXER_TOKEN_THIS: IO_PRINTF_BREAK("LEXER_TOKEN_THIS");
    case LEXER_TOKEN_PRINT: IO_PRINTF_BREAK("LEXER_TOKEN_PRINT");

    default: ERROR_INTERNAL("Unknown lexer token type '%d'", token->type);
  }

  printf(" '%.*s'\n", token->lexeme_length, token->lexeme);
}

/**@desc disassemble and print `chunk` annotated with `name`*/
void debug_disassemble_chunk(Chunk const *const chunk, char const *const name) {
  assert(chunk != NULL);
  assert(name != NULL);

  printf("\n== %s ==\n", name);
  for (int32_t offset = 0; offset < chunk->count;) offset = debug_disassemble_instruction(chunk, offset);
}

/**@desc print simple instruction (one without operands) encoded by `opcode` and located at `offset`
@return offset to next instruction*/
static inline int32_t debug_simple_instruction(uint8_t const opcode, int32_t const offset) {
  static_assert(CHUNK_OP_SIMPLE_OPCODE_COUNT == 19, "Exhaustive simple chunk opcode handling");
  switch (opcode) {
    case CHUNK_OP_RETURN: IO_PUTS_BREAK("CHUNK_OP_RETURN");
    case CHUNK_OP_PRINT: IO_PUTS_BREAK("CHUNK_OP_PRINT");
    case CHUNK_OP_POP: IO_PUTS_BREAK("CHUNK_OP_POP");
    case CHUNK_OP_NEGATE: IO_PUTS_BREAK("CHUNK_OP_NEGATE");
    case CHUNK_OP_ADD: IO_PUTS_BREAK("CHUNK_OP_ADD");
    case CHUNK_OP_SUBTRACT: IO_PUTS_BREAK("CHUNK_OP_SUBTRACT");
    case CHUNK_OP_MULTIPLY: IO_PUTS_BREAK("CHUNK_OP_MULTIPLY");
    case CHUNK_OP_DIVIDE: IO_PUTS_BREAK("CHUNK_OP_DIVIDE");
    case CHUNK_OP_MODULO: IO_PUTS_BREAK("CHUNK_OP_MODULO");
    case CHUNK_OP_NOT: IO_PUTS_BREAK("CHUNK_OP_NOT");
    case CHUNK_OP_NIL: IO_PUTS_BREAK("CHUNK_OP_NIL");
    case CHUNK_OP_TRUE: IO_PUTS_BREAK("CHUNK_OP_TRUE");
    case CHUNK_OP_FALSE: IO_PUTS_BREAK("CHUNK_OP_FALSE");
    case CHUNK_OP_EQUAL: IO_PUTS_BREAK("CHUNK_OP_EQUAL");
    case CHUNK_OP_NOT_EQUAL: IO_PUTS_BREAK("CHUNK_OP_NOT_EQUAL");
    case CHUNK_OP_LESS: IO_PUTS_BREAK("CHUNK_OP_LESS");
    case CHUNK_OP_LESS_EQUAL: IO_PUTS_BREAK("CHUNK_OP_LESS_EQUAL");
    case CHUNK_OP_GREATER: IO_PUTS_BREAK("CHUNK_OP_GREATER");
    case CHUNK_OP_GREATER_EQUAL: IO_PUTS_BREAK("CHUNK_OP_GREATER_EQUAL");

    default: ERROR_INTERNAL("Unknown chunk simple instruction opcode '%d'", opcode);
  }

  return offset + 1;
}

/**@desc print `chunk` constant instruction encoded by `opcode` and located at `offset`
@return offset to next instruction*/
static int32_t debug_constant_instruction(Chunk const *const chunk, uint8_t const opcode, int32_t const offset) {
  assert(chunk != NULL);

  if (opcode == CHUNK_OP_CONSTANT) {
    uint8_t const constant_index = chunk->code[offset + 1];

    printf("CHUNK_OP_CONSTANT %d '", constant_index);
    value_print(chunk->constants.values[constant_index]);
    printf("'\n");

    return offset + 2;
  }

  if (opcode == CHUNK_OP_CONSTANT_2B) {
    unsigned int const constant_index = memory_concatenate_bytes(2, chunk->code[offset + 2], chunk->code[offset + 1]);

    printf("CHUNK_OP_CONSTANT_2B %d '", constant_index);
    value_print(chunk->constants.values[constant_index]);
    printf("'\n");

    return offset + 3;
  }

  ERROR_INTERNAL("Unknown chunk constant instruction opcode '%d'", opcode);
}

/**@desc disassemble and print `chunk` instruction located at `offset`
@return offset to next instruction*/
int32_t debug_disassemble_instruction(Chunk const *const chunk, int32_t const offset) {
  assert(chunk != NULL);
  assert(offset >= 0 && "Expected offset to be nonnegative");

  printf(COMMON_FILE_LINE_FORMAT " ", g_source_file, chunk_get_instruction_line(chunk, offset));

  uint8_t const opcode = chunk->code[offset];

  static_assert(CHUNK_OP_OPCODE_COUNT == 21, "Exhaustive chunk opcode handling");
  switch (opcode) {
    case CHUNK_OP_RETURN:
    case CHUNK_OP_PRINT:
    case CHUNK_OP_POP:
    case CHUNK_OP_NEGATE:
    case CHUNK_OP_ADD:
    case CHUNK_OP_SUBTRACT:
    case CHUNK_OP_MULTIPLY:
    case CHUNK_OP_DIVIDE:
    case CHUNK_OP_MODULO:
    case CHUNK_OP_NOT:
    case CHUNK_OP_NIL:
    case CHUNK_OP_TRUE:
    case CHUNK_OP_FALSE:
    case CHUNK_OP_EQUAL:
    case CHUNK_OP_NOT_EQUAL:
    case CHUNK_OP_LESS:
    case CHUNK_OP_LESS_EQUAL:
    case CHUNK_OP_GREATER:
    case CHUNK_OP_GREATER_EQUAL: {
      return debug_simple_instruction(opcode, offset);
    }
    case CHUNK_OP_CONSTANT:
    case CHUNK_OP_CONSTANT_2B: {
      return debug_constant_instruction(chunk, opcode, offset);
    }
    default: ERROR_INTERNAL("Unknown chunk opcode '%d'", opcode);
  }
}
