#include "frontend/compiler.h"

#include "frontend/lexer.h"
#include "global.h"
#include "utils/debug.h"
#include "utils/error.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef enum { PARSER_OK, PARSER_PANIC, PARSER_UNEXPECTED_EOF } ParserState;

typedef enum { ERROR_LEXICAL, ERROR_SYNTAX, ERROR_SEMANTIC, ERROR_TYPE_COUNT } ErrorType;

/**@desc token precedence*/
typedef enum {
  PRECEDENCE_NONE,
  PRECEDENCE_ASSIGNMENT, // right-associative
  PRECEDENCE_OR,
  PRECEDENCE_AND,
  PRECEDENCE_EQUALITY,
  PRECEDENCE_COMPARISON,
  PRECEDENCE_TERM,
  PRECEDENCE_FACTOR,
  PRECEDENCE_UNARY,
  PRECEDENCE_CALL,
  PRECEDENCE_PRIMARY
} Precedence;

/**@desc function handling (compiling) specified TokenType*/
typedef void (*TokenHandler)(void);

typedef struct {
  TokenHandler nud; // null-denotation (requires no-context)
  TokenHandler led; // left-denotation (requires left-context subexpression)
  Precedence precedence;
} ParseRule;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

static void compiler_binary_expr(void);
static void compiler_unary_expr(void);
static void compiler_grouping_expr(void);
static void compiler_numeric_literal(void);
static void compiler_invariable_literal(void);

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static_assert(LEXER_TOKEN_TYPE_COUNT - LEXER_TOKEN_INDICATOR_COUNT == 41, "Exhaustive TokenType handling");
static ParseRule const parse_rules[] = {
  // literals
  [LEXER_TOKEN_NIL] = {compiler_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_TRUE] = {compiler_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_FALSE] = {compiler_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_NUMBER] = {compiler_numeric_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_STRING] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_IDENTIFIER] = {NULL, NULL, PRECEDENCE_NONE},

  // single-character tokens
  [LEXER_TOKEN_PLUS] = {NULL, compiler_binary_expr, PRECEDENCE_TERM},
  [LEXER_TOKEN_MINUS] = {compiler_unary_expr, compiler_binary_expr, PRECEDENCE_TERM},
  [LEXER_TOKEN_STAR] = {NULL, compiler_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_SLASH] = {NULL, compiler_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_PERCENT] = {NULL, compiler_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_BANG] = {compiler_unary_expr, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_LESS] = {NULL, compiler_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_GREATER] = {NULL, compiler_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_DOT] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_COMMA] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_COLON] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_SEMICOLON] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_QUESTION] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_OPEN_PAREN] = {compiler_grouping_expr, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_CLOSE_PAREN] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_OPEN_CURLY_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_CLOSE_CURLY_BRACE] = {NULL, NULL, PRECEDENCE_NONE},

  // multi-character tokens
  [LEXER_TOKEN_EQUAL_EQUAL] = {NULL, compiler_binary_expr, PRECEDENCE_EQUALITY},
  [LEXER_TOKEN_BANG_EQUAL] = {NULL, compiler_binary_expr, PRECEDENCE_EQUALITY},
  [LEXER_TOKEN_LESS_EQUAL] = {NULL, compiler_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_GREATER_EQUAL] = {NULL, compiler_binary_expr, PRECEDENCE_COMPARISON},

  // reserved identifiers (keywords)
  [LEXER_TOKEN_VAR] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_AND] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_OR] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_FUN] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_RETURN] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_IF] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_ELSE] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_WHILE] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_FOR] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_CLASS] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_SUPER] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_THIS] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_PRINT] = {NULL, NULL, PRECEDENCE_NONE},
};

static Chunk *current_chunk; // TEMP

static struct {
  LexerToken previous, current;
  ParserState state;
  bool had_error;
} parser;

// *---------------------------------------------*
// *                  UTILITIES                  *
// *---------------------------------------------*

/**@desc retrieve currently compiled chunk*/
static inline Chunk *compiler_current_chunk(void) {
  return current_chunk;
}

/**@desc handle `error_type` error at `token` with `message`*/
static void compiler_error_at(ErrorType const error_type, LexerToken const *const token, char const *const message) {
  assert(token != NULL);
  assert(message != NULL);

  if (parser.state != PARSER_OK) return;

  // record error
  parser.state = token->type == LEXER_TOKEN_EOF ? PARSER_UNEXPECTED_EOF : PARSER_PANIC;
  parser.had_error = true;

  // print error
  static_assert(ERROR_TYPE_COUNT == 3, "Exhaustive ErrorType handling");
  switch (error_type) {
    case ERROR_LEXICAL: {
      fprintf(g_static_error_stream, "[LEXICAL_ERROR]");
      break;
    }
    case ERROR_SYNTAX: {
      fprintf(g_static_error_stream, "[SYNTAX_ERROR]");
      break;
    }
    case ERROR_SEMANTIC: {
      fprintf(g_static_error_stream, "[SEMANTIC_ERROR]");
      break;
    }
    default: ERROR_INTERNAL("Unknown error_type '%d'", error_type);
  }
  fprintf(
    g_static_error_stream, COMMON_MS COMMON_FILE_LINE_COLUMN_FORMAT COMMON_MS "%s", g_source_file, token->line,
    token->column, message
  );
  if (token->type == LEXER_TOKEN_ERROR || token->type == LEXER_TOKEN_EOF) fprintf(g_static_error_stream, "\n");
  else fprintf(g_static_error_stream, " at '%.*s'\n", token->lexeme_length, token->lexeme);
}

/**@desc handle `error_type` error at parser.previous token with `message`*/
static inline void compiler_error_at_previous(ErrorType const error_type, char const *const message) {
  compiler_error_at(error_type, &parser.previous, message);
}

/**@desc handle `error_type` error at parser.current token with `message`*/
static inline void compiler_error_at_current(ErrorType const error_type, char const *const message) {
  compiler_error_at(error_type, &parser.current, message);
}

/**@desc update parser.previous, advance parser.current, and handle any error tokens*/
static void compiler_advance(void) {
  parser.previous = parser.current;

  for (;;) {
    parser.current = lexer_scan();
    if (parser.current.type != LEXER_TOKEN_ERROR) break;
    compiler_error_at_current(ERROR_LEXICAL, parser.current.lexeme);
  }
}

/**@desc advance compiler if parser.current.type matches `type`, report error with `message` otherwise*/
static inline void compiler_consume(LexerTokenType const type, char const *const message) {
  if (parser.current.type != type) compiler_error_at_current(ERROR_SYNTAX, message);
  compiler_advance();
}

/**@desc advance compiler if parser.current.type matches `type`
@return true if it does, false otherwise*/
static inline bool compiler_match(LexerTokenType const type) {
  if (parser.current.type != type) return false;
  compiler_advance();
  return true;
}

/**@desc generate `opcode` bytecode instruction and append it to current_chunk*/
static inline void compiler_emit_instruction(ChunkOpCode const opcode) {
  chunk_append_instruction(compiler_current_chunk(), opcode, parser.previous.line);
}

/**@desc generate bytecode constant instruction and append it to current_chunk*/
static inline void compiler_emit_constant_instruction(Value const value) {
  chunk_append_constant_instruction(compiler_current_chunk(), value, parser.previous.line);
}

// *---------------------------------------------*
// *              COMPILE FUNCTIONS              *
// *---------------------------------------------*

/**@desc compile `precedence` level expression*/
static void compiler_precedence_expr(Precedence const precedence) {
  compiler_advance();

  // compile head token subexpression
  if (parse_rules[parser.previous.type].nud == NULL) {
    compiler_error_at_previous(ERROR_SYNTAX, "Expected expression");
    return;
  }
  parse_rules[parser.previous.type].nud();

  // compile tail tokens subexpression
  while (parse_rules[parser.current.type].precedence >= precedence) {
    compiler_advance();
    parse_rules[parser.previous.type].led();
  }
}

/**@desc compile expression*/
static void compiler_expr(void) {
  compiler_precedence_expr(PRECEDENCE_ASSIGNMENT);
}

/**@desc compile binary expression*/
static void compiler_binary_expr(void) {
  LexerTokenType const operator_type = parser.previous.type;
  compiler_precedence_expr(parse_rules[operator_type].precedence + 1);

  switch (operator_type) {
    case LEXER_TOKEN_PLUS: {
      compiler_emit_instruction(CHUNK_OP_ADD);
      break;
    }
    case LEXER_TOKEN_MINUS: {
      compiler_emit_instruction(CHUNK_OP_SUBTRACT);
      break;
    }
    case LEXER_TOKEN_STAR: {
      compiler_emit_instruction(CHUNK_OP_MULTIPLY);
      break;
    }
    case LEXER_TOKEN_SLASH: {
      compiler_emit_instruction(CHUNK_OP_DIVIDE);
      break;
    }
    case LEXER_TOKEN_PERCENT: {
      compiler_emit_instruction(CHUNK_OP_MODULO);
      break;
    }
    case LEXER_TOKEN_EQUAL_EQUAL: {
      compiler_emit_instruction(CHUNK_OP_EQUAL);
      break;
    }
    case LEXER_TOKEN_BANG_EQUAL: {
      compiler_emit_instruction(CHUNK_OP_NOT_EQUAL);
      break;
    }
    case LEXER_TOKEN_LESS: {
      compiler_emit_instruction(CHUNK_OP_LESS);
      break;
    }
    case LEXER_TOKEN_LESS_EQUAL: {
      compiler_emit_instruction(CHUNK_OP_LESS_EQUAL);
      break;
    }
    case LEXER_TOKEN_GREATER: {
      compiler_emit_instruction(CHUNK_OP_GREATER);
      break;
    }
    case LEXER_TOKEN_GREATER_EQUAL: {
      compiler_emit_instruction(CHUNK_OP_GREATER_EQUAL);
      break;
    }
    default: ERROR_INTERNAL("Unknown binary operator type '%d'", operator_type);
  }
}

/**@desc compile unary expression*/
static void compiler_unary_expr(void) {
  LexerTokenType const operator_type = parser.previous.type;
  compiler_precedence_expr(PRECEDENCE_UNARY);

  switch (operator_type) {
    case LEXER_TOKEN_MINUS: {
      compiler_emit_instruction(CHUNK_OP_NEGATE);
      break;
    }
    case LEXER_TOKEN_BANG: {
      compiler_emit_instruction(CHUNK_OP_NOT);
      break;
    }
    default: ERROR_INTERNAL("Unknown unary operator type '%d'", operator_type);
  }
}

/**@desc compile '(...)' grouping expression*/
static void compiler_grouping_expr(void) {
  compiler_expr();
  compiler_consume(LEXER_TOKEN_CLOSE_PAREN, "Expected ')' closing grouping expression");
}

/**@desc compile numeric literal*/
static void compiler_numeric_literal(void) {
  errno = 0;
  double const value = strtod(parser.previous.lexeme, NULL);
  if (errno != 0) {
    ERROR_MEMORY(
      COMMON_FILE_LINE_COLUMN_FORMAT COMMON_MS "Out-of-range numeric literal '%.*s'", g_source_file,
      parser.previous.line, parser.previous.column, parser.previous.lexeme_length, parser.previous.lexeme
    );
  }
  compiler_emit_constant_instruction(VALUE_MAKE_NUMBER(value));
}

/**@desc compile invariable literal (one with fixed lexeme)*/
static void compiler_invariable_literal(void) {
  LexerTokenType const literal_type = parser.previous.type;

  switch (literal_type) {
    case LEXER_TOKEN_NIL: {
      compiler_emit_instruction(CHUNK_OP_NIL);
      break;
    }
    case LEXER_TOKEN_TRUE: {
      compiler_emit_instruction(CHUNK_OP_TRUE);
      break;
    }
    case LEXER_TOKEN_FALSE: {
      compiler_emit_instruction(CHUNK_OP_FALSE);
      break;
    }
    default: ERROR_INTERNAL("Unknown invariable literal type '%d'", literal_type);
  }
}

/**@desc compile expression statement*/
static void compiler_expr_stmt(void) {
  compiler_expr();
  compiler_consume(LEXER_TOKEN_SEMICOLON, "Expected ';' terminating expression statement");
  compiler_emit_instruction(CHUNK_OP_POP); // discard expression result
}

/**@desc compile print statement*/
static void compiler_print_stmt(void) {
  compiler_expr();
  compiler_consume(LEXER_TOKEN_SEMICOLON, "Expected ';' terminating print statement");
  compiler_emit_instruction(CHUNK_OP_PRINT);
}

/**@desc compile statement*/
static void compiler_stmt(void) {
  if (compiler_match(LEXER_TOKEN_PRINT)) compiler_print_stmt();
  else compiler_expr_stmt();
}

/**@desc compile `source_code` into bytecode instructions and append them to `chunk`
@return true if compilation succeeded, false otherwise*/
CompilerStatus compiler_compile(char const *const source_code, Chunk *const chunk) {
  assert(source_code != NULL);
  assert(chunk != NULL);

  // reset compiler
  parser.state = PARSER_OK;
  parser.had_error = false;
  current_chunk = chunk; // TEMP
  lexer_init(source_code);
  compiler_advance();

  // compile source_code
  while (!compiler_match(LEXER_TOKEN_EOF)) compiler_stmt();

  compiler_emit_instruction(CHUNK_OP_RETURN); // TEMP

#ifdef DEBUG_COMPILER
  if (!parser.had_error) debug_disassemble_chunk(chunk, "DEBUG_COMPILER");
#endif

  if (!parser.had_error) return COMPILER_SUCCESS;
  if (parser.state == PARSER_UNEXPECTED_EOF) return COMPILER_UNEXPECTED_EOF;
  return COMPILER_FAILURE;
}
