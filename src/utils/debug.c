#include "utils/debug.h"

#include "backend/value.h"
#include "common.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/memory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Print simple instruction (one without operands) encoded by `opcode` and located at `offset`.
/// @return Offset to next instruction.
static inline int32_t debug_simple_instruction(uint8_t const opcode, int32_t const offset) {
#define PUTS_BREAK(string) \
  io_puts(string);         \
  break

  static_assert(CHUNK_OP_SIMPLE_OPCODE_COUNT == 20, "Exhaustive simple chunk opcode handling");
  switch (opcode) {
    case CHUNK_OP_RETURN: PUTS_BREAK("OP_RETURN");
    case CHUNK_OP_PRINT: PUTS_BREAK("OP_PRINT");
    case CHUNK_OP_POP: PUTS_BREAK("OP_POP");
    case CHUNK_OP_NEGATE: PUTS_BREAK("OP_NEGATE");
    case CHUNK_OP_ADD: PUTS_BREAK("OP_ADD");
    case CHUNK_OP_SUBTRACT: PUTS_BREAK("OP_SUBTRACT");
    case CHUNK_OP_MULTIPLY: PUTS_BREAK("OP_MULTIPLY");
    case CHUNK_OP_DIVIDE: PUTS_BREAK("OP_DIVIDE");
    case CHUNK_OP_MODULO: PUTS_BREAK("OP_MODULO");
    case CHUNK_OP_NOT: PUTS_BREAK("OP_NOT");
    case CHUNK_OP_NIL: PUTS_BREAK("OP_NIL");
    case CHUNK_OP_TRUE: PUTS_BREAK("OP_TRUE");
    case CHUNK_OP_FALSE: PUTS_BREAK("OP_FALSE");
    case CHUNK_OP_EQUAL: PUTS_BREAK("OP_EQUAL");
    case CHUNK_OP_NOT_EQUAL: PUTS_BREAK("OP_NOT_EQUAL");
    case CHUNK_OP_LESS: PUTS_BREAK("OP_LESS");
    case CHUNK_OP_LESS_EQUAL: PUTS_BREAK("OP_LESS_EQUAL");
    case CHUNK_OP_GREATER: PUTS_BREAK("OP_GREATER");
    case CHUNK_OP_GREATER_EQUAL: PUTS_BREAK("OP_GREATER_EQUAL");
    case CHUNK_OP_CONCATENATE: PUTS_BREAK("OP_CONCATENATE");

    default: ERROR_INTERNAL("Unknown chunk simple instruction opcode '%d'", opcode);
  }

  return offset + 1;

#undef PUTS_BREAK
}

/// Print `chunk` constant instruction encoded by `opcode` and located at `offset`.
/// @return Offset to next instruction.
static int32_t debug_constant_instruction(Chunk const *const chunk, uint8_t const opcode, int32_t const offset) {
  assert(chunk != NULL);

  if (opcode == CHUNK_OP_CONSTANT) {
    uint8_t const constant_index = chunk->code.data[offset + 1];

    io_printf("OP_CONSTANT %d '", constant_index);
    value_print(chunk->constants.data[constant_index]);
    io_printf("'\n");

    return offset + 2;
  }

  if (opcode == CHUNK_OP_CONSTANT_2B) {
    unsigned int const constant_index =
      memory_concatenate_bytes(2, chunk->code.data[offset + 2], chunk->code.data[offset + 1]);

    io_printf("OP_CONSTANT_2B %d '", constant_index);
    value_print(chunk->constants.data[constant_index]);
    io_printf("'\n");

    return offset + 3;
  }

  ERROR_INTERNAL("Unknown chunk constant instruction opcode '%d'", opcode);
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Print lexical `token`.
void debug_token(LexerToken const *const token) {
#define LOG_BASIC_TOKEN(token_postfix_string)                                                                  \
  io_printf("TOKEN_" token_postfix_string " '%.*s'\n", token->as.basic.lexeme_length, token->as.basic.lexeme); \
  break

  io_printf(COMMON_FILE_LINE_COLUMN_FORMAT " ", g_source_file_path, token->line, token->column);

  static_assert(LEXER_TOKEN_KIND_COUNT == 44, "Exhaustive LexerTokenKind handling");
  switch (token->kind) {
    // indicators
    case LEXER_TOKEN_EOF: LOG_BASIC_TOKEN("EOF");
    case LEXER_TOKEN_ERROR: {
      io_printf("TOKEN_ERROR '%s'\n", token->as.error.message);
      break;
    }

    // literals
    case LEXER_TOKEN_STRING: LOG_BASIC_TOKEN("STRING");
    case LEXER_TOKEN_IDENTIFIER: LOG_BASIC_TOKEN("IDENTIFIER");
    case LEXER_TOKEN_NUMBER: LOG_BASIC_TOKEN("NUMBER");

    // single-character tokens
    case LEXER_TOKEN_PLUS: LOG_BASIC_TOKEN("PLUS");
    case LEXER_TOKEN_MINUS: LOG_BASIC_TOKEN("MINUS");
    case LEXER_TOKEN_STAR: LOG_BASIC_TOKEN("STAR");
    case LEXER_TOKEN_SLASH: LOG_BASIC_TOKEN("SLASH");
    case LEXER_TOKEN_PERCENT: LOG_BASIC_TOKEN("PERCENT");
    case LEXER_TOKEN_BANG: LOG_BASIC_TOKEN("BANG");
    case LEXER_TOKEN_LESS: LOG_BASIC_TOKEN("LESS");
    case LEXER_TOKEN_EQUAL: LOG_BASIC_TOKEN("EQUAL");
    case LEXER_TOKEN_GREATER: LOG_BASIC_TOKEN("GREATER");
    case LEXER_TOKEN_DOT: LOG_BASIC_TOKEN("DOT");
    case LEXER_TOKEN_COMMA: LOG_BASIC_TOKEN("COMMA");
    case LEXER_TOKEN_COLON: LOG_BASIC_TOKEN("COLON");
    case LEXER_TOKEN_SEMICOLON: LOG_BASIC_TOKEN("SEMICOLON");
    case LEXER_TOKEN_QUESTION: LOG_BASIC_TOKEN("QUESTION");
    case LEXER_TOKEN_OPEN_PAREN: LOG_BASIC_TOKEN("OPEN_PAREN");
    case LEXER_TOKEN_CLOSE_PAREN: LOG_BASIC_TOKEN("CLOSE_PAREN");
    case LEXER_TOKEN_OPEN_CURLY_BRACE: LOG_BASIC_TOKEN("OPEN_CURLY_BRACE");
    case LEXER_TOKEN_CLOSE_CURLY_BRACE: LOG_BASIC_TOKEN("CLOSE_CURLY_BRACE");

    // multi-character tokens
    case LEXER_TOKEN_DOT_DOT: LOG_BASIC_TOKEN("DOT_DOT");
    case LEXER_TOKEN_BANG_EQUAL: LOG_BASIC_TOKEN("BANG_EQUAL");
    case LEXER_TOKEN_LESS_EQUAL: LOG_BASIC_TOKEN("LESS_EQUAL");
    case LEXER_TOKEN_EQUAL_EQUAL: LOG_BASIC_TOKEN("EQUAL_EQUAL");
    case LEXER_TOKEN_GREATER_EQUAL: LOG_BASIC_TOKEN("GREATER_EQUAL");

    // reserved identifiers (keywords)
    case LEXER_TOKEN_TRUE: LOG_BASIC_TOKEN("TRUE");
    case LEXER_TOKEN_FALSE: LOG_BASIC_TOKEN("FALSE");
    case LEXER_TOKEN_VAR: LOG_BASIC_TOKEN("VAR");
    case LEXER_TOKEN_NIL: LOG_BASIC_TOKEN("NIL");
    case LEXER_TOKEN_AND: LOG_BASIC_TOKEN("AND");
    case LEXER_TOKEN_OR: LOG_BASIC_TOKEN("OR");
    case LEXER_TOKEN_FUN: LOG_BASIC_TOKEN("FUN");
    case LEXER_TOKEN_RETURN: LOG_BASIC_TOKEN("RETURN");
    case LEXER_TOKEN_IF: LOG_BASIC_TOKEN("IF");
    case LEXER_TOKEN_ELSE: LOG_BASIC_TOKEN("ELSE");
    case LEXER_TOKEN_WHILE: LOG_BASIC_TOKEN("WHILE");
    case LEXER_TOKEN_FOR: LOG_BASIC_TOKEN("FOR");
    case LEXER_TOKEN_CLASS: LOG_BASIC_TOKEN("CLASS");
    case LEXER_TOKEN_SUPER: LOG_BASIC_TOKEN("SUPER");
    case LEXER_TOKEN_THIS: LOG_BASIC_TOKEN("THIS");
    case LEXER_TOKEN_PRINT: LOG_BASIC_TOKEN("PRINT");

    default: ERROR_INTERNAL("Unknown lexer token kind '%d'", token->kind);
  }

#undef LOG_BASIC_TOKEN
}

/// Disassemble and print `chunk` annotated with `name`.
void debug_disassemble_chunk(Chunk const *const chunk, char const *const name) {
  assert(chunk != NULL);
  assert(name != NULL);

  io_printf("\n== %s ==\n", name);
  for (size_t offset = 0; offset < chunk->code.count;) offset = debug_disassemble_instruction(chunk, offset);
}

/// Disassemble and print `chunk` instruction located at `offset`.
/// @return Offset to next instruction.
int32_t debug_disassemble_instruction(Chunk const *const chunk, int32_t const offset) {
  assert(chunk != NULL);
  assert(offset >= 0 && "Expected offset to be nonnegative");

  io_printf(COMMON_FILE_LINE_FORMAT " ", g_source_file_path, chunk_get_instruction_line(chunk, offset));

  uint8_t const opcode = chunk->code.data[offset];

  static_assert(CHUNK_OP_OPCODE_COUNT == 22, "Exhaustive chunk opcode handling");
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
    case CHUNK_OP_GREATER_EQUAL:
    case CHUNK_OP_CONCATENATE: {
      return debug_simple_instruction(opcode, offset);
    }
    case CHUNK_OP_CONSTANT:
    case CHUNK_OP_CONSTANT_2B: {
      return debug_constant_instruction(chunk, opcode, offset);
    }
    default: ERROR_INTERNAL("Unknown chunk opcode '%d'", opcode);
  }
}
