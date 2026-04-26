#include "frontend/compiler.h"

#include "backend/entity.h"
#include "frontend/lexer.h"
#include "global.h"
#include "utils/debug.h"
#include "utils/error.h"
#include "utils/io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

typedef enum { PARSER_OK, PARSER_PANIC, PARSER_UNEXPECTED_EOF } ParserState;

typedef enum { ERROR_LEXICAL, ERROR_SYNTAX, ERROR_SEMANTIC, ERROR_KIND_COUNT } ErrorKind;

/// Token precedence.
typedef enum {
  PRECEDENCE_NONE,
  PRECEDENCE_ASSIGNMENT, // right-associative
  PRECEDENCE_OR,
  PRECEDENCE_AND,
  PRECEDENCE_EQUALITY,
  PRECEDENCE_COMPARISON,
  PRECEDENCE_CONCATENATION, // right-associative
  PRECEDENCE_TERM,
  PRECEDENCE_FACTOR,
  PRECEDENCE_UNARY,
  PRECEDENCE_CALL,
  PRECEDENCE_PRIMARY
} Precedence;

/// Function handling (compiling) specified token kind.
typedef void(TokenHandlerFn)(void);

typedef struct {
  TokenHandlerFn *nud; // null-denotation (requires no-context)
  TokenHandlerFn *led; // left-denotation (requires left-context subexpression)
  Precedence precedence;
} ParseRule;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

static TokenHandlerFn compile_left_associative_binary_expr;
static TokenHandlerFn compile_right_associative_binary_expr;
static TokenHandlerFn compile_unary_expr;
static TokenHandlerFn compile_grouping_expr;
static TokenHandlerFn compile_numeric_literal;
static TokenHandlerFn compile_string_literal;
static TokenHandlerFn compile_invariable_literal;

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static_assert(LEXER_TOKEN_KIND_COUNT - LEXER_TOKEN_INDICATOR_COUNT == 42, "Exhaustive LexerTokenKind handling");
static ParseRule const parse_rules[] = {
  // literals
  [LEXER_TOKEN_NIL] = {compile_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_TRUE] = {compile_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_FALSE] = {compile_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_NUMBER] = {compile_numeric_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_STRING] = {compile_string_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_IDENTIFIER] = {NULL, NULL, PRECEDENCE_NONE},

  // single-character tokens
  [LEXER_TOKEN_PLUS] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_TERM},
  [LEXER_TOKEN_MINUS] = {compile_unary_expr, compile_left_associative_binary_expr, PRECEDENCE_TERM},
  [LEXER_TOKEN_STAR] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_SLASH] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_PERCENT] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_BANG] = {compile_unary_expr, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_LESS] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_GREATER] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_DOT] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_COMMA] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_COLON] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_SEMICOLON] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_QUESTION] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_OPEN_PAREN] = {compile_grouping_expr, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_CLOSE_PAREN] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_OPEN_CURLY_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_CLOSE_CURLY_BRACE] = {NULL, NULL, PRECEDENCE_NONE},

  // multi-character tokens
  [LEXER_TOKEN_DOT_DOT] = {NULL, compile_right_associative_binary_expr, PRECEDENCE_CONCATENATION},
  [LEXER_TOKEN_EQUAL_EQUAL] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_EQUALITY},
  [LEXER_TOKEN_BANG_EQUAL] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_EQUALITY},
  [LEXER_TOKEN_LESS_EQUAL] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_GREATER_EQUAL] = {NULL, compile_left_associative_binary_expr, PRECEDENCE_COMPARISON},

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
  bool has_previous, has_current;
} parser;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Retrieve currently compiled chunk.
static inline Chunk *get_current_chunk(void) {
  return current_chunk;
}

/// Handle `error_kind` error at `token` with `message`.
static void compiler_error_at(ErrorKind const error_kind, LexerToken const *const token, char const *const message) {
  assert(token != NULL);
  assert(message != NULL);

  if (parser.state != PARSER_OK) return;

  // record error
  parser.state = token->kind == LEXER_TOKEN_EOF ? PARSER_UNEXPECTED_EOF : PARSER_PANIC;
  parser.had_error = true;

  // print error
  static_assert(ERROR_KIND_COUNT == 3, "Exhaustive ErrorKind handling");
  switch (error_kind) {
    case ERROR_LEXICAL: {
      io_fprintf(g_static_analysis_error_stream, "[LEXICAL_ERROR]");
      break;
    }
    case ERROR_SYNTAX: {
      io_fprintf(g_static_analysis_error_stream, "[SYNTAX_ERROR]");
      break;
    }
    case ERROR_SEMANTIC: {
      io_fprintf(g_static_analysis_error_stream, "[SEMANTIC_ERROR]");
      break;
    }
    default: ERROR_INTERNAL("Unknown error_kind '%d'", error_kind);
  }
  io_fprintf(
    g_static_analysis_error_stream, COMMON_MS COMMON_FILE_LINE_COLUMN_FORMAT COMMON_MS "%s", g_source_file_path,
    token->line, token->column, message
  );
  if (token->kind == LEXER_TOKEN_ERROR || token->kind == LEXER_TOKEN_EOF) {
    io_fprintf(g_static_analysis_error_stream, "\n");
  } else {
    io_fprintf(g_static_analysis_error_stream, " at '%.*s'\n", token->as.basic.lexeme_length, token->as.basic.lexeme);
  }
}

/// Handle `error_kind` error at parser.previous token with `message`.
static inline void compiler_error_at_previous(ErrorKind const error_kind, char const *const message) {
  compiler_error_at(error_kind, &parser.previous, message);
}

/// Handle `error_kind` error at parser.current token with `message`.
static inline void compiler_error_at_current(ErrorKind const error_kind, char const *const message) {
  compiler_error_at(error_kind, &parser.current, message);
}

/// Update parser.previous, advance parser.current, and handle any error tokens.
static void compiler_advance(void) {
  if (parser.has_previous) lexer_token_free(parser.previous);
  if (parser.has_current) {
    parser.previous = parser.current;
    parser.has_previous = true;
  }

  for (;;) {
    parser.current = lexer_scan();
    if (parser.current.kind != LEXER_TOKEN_ERROR) break;

    // handle and discard error token; TokenHandlerFn never sees error tokens
    compiler_error_at_current(ERROR_LEXICAL, parser.current.as.error.message);
    lexer_token_free(parser.current);
  }
}

/// Advance compiler if parser.current.kind matches `kind`, report error with `message` otherwise.
static inline void compiler_consume(LexerTokenKind const kind, char const *const message) {
  if (parser.current.kind != kind) compiler_error_at_current(ERROR_SYNTAX, message);
  compiler_advance();
}

/// Advance compiler if parser.current.kind matches `kind`.
/// @return true if it does, false otherwise.
static inline bool compiler_match(LexerTokenKind const kind) {
  if (parser.current.kind != kind) return false;
  compiler_advance();
  return true;
}

/// Generate `opcode` bytecode instruction and append it to current_chunk.
static inline void emit_instruction(ChunkOpCode const opcode) {
  chunk_append_instruction(get_current_chunk(), opcode, parser.previous.line);
}

/// Generate bytecode constant instruction and append it to current_chunk.
static inline void emit_constant_instruction(Value const value) {
  chunk_append_constant_instruction(get_current_chunk(), value, parser.previous.line);
}

/// Compile `precedence` level expression.
static void compile_precedence_expr(Precedence const precedence) {
  compiler_advance();

  // compile head token subexpression
  if (parse_rules[parser.previous.kind].nud == NULL) {
    compiler_error_at_previous(ERROR_SYNTAX, "Expected expression");
    return;
  }
  parse_rules[parser.previous.kind].nud();

  // compile tail tokens subexpression
  while (parse_rules[parser.current.kind].precedence >= precedence) {
    compiler_advance();
    parse_rules[parser.previous.kind].led();
  }
}

/// Compile expression.
static void compile_expr(void) {
  compile_precedence_expr(PRECEDENCE_ASSIGNMENT);
}

/// Compile left-associaive binary expression.
static void compile_left_associative_binary_expr(void) {
  LexerTokenKind const operator_kind = parser.previous.kind;
  compile_precedence_expr(parse_rules[operator_kind].precedence + 1);

  switch (operator_kind) {
    case LEXER_TOKEN_PLUS: {
      emit_instruction(CHUNK_OP_ADD);
      break;
    }
    case LEXER_TOKEN_MINUS: {
      emit_instruction(CHUNK_OP_SUBTRACT);
      break;
    }
    case LEXER_TOKEN_STAR: {
      emit_instruction(CHUNK_OP_MULTIPLY);
      break;
    }
    case LEXER_TOKEN_SLASH: {
      emit_instruction(CHUNK_OP_DIVIDE);
      break;
    }
    case LEXER_TOKEN_PERCENT: {
      emit_instruction(CHUNK_OP_MODULO);
      break;
    }
    case LEXER_TOKEN_EQUAL_EQUAL: {
      emit_instruction(CHUNK_OP_EQUAL);
      break;
    }
    case LEXER_TOKEN_BANG_EQUAL: {
      emit_instruction(CHUNK_OP_NOT_EQUAL);
      break;
    }
    case LEXER_TOKEN_LESS: {
      emit_instruction(CHUNK_OP_LESS);
      break;
    }
    case LEXER_TOKEN_LESS_EQUAL: {
      emit_instruction(CHUNK_OP_LESS_EQUAL);
      break;
    }
    case LEXER_TOKEN_GREATER: {
      emit_instruction(CHUNK_OP_GREATER);
      break;
    }
    case LEXER_TOKEN_GREATER_EQUAL: {
      emit_instruction(CHUNK_OP_GREATER_EQUAL);
      break;
    }
    default: ERROR_INTERNAL("Unknown binary operator kind '%d'", operator_kind);
  }
}

/// Compile right-associative binary expression.
static void compile_right_associative_binary_expr(void) {
  LexerTokenKind const operator_kind = parser.previous.kind;
  compile_precedence_expr(parse_rules[operator_kind].precedence);

  switch (operator_kind) {
    case LEXER_TOKEN_DOT_DOT: {
      emit_instruction(CHUNK_OP_CONCATENATE);
      break;
    }
    default: ERROR_INTERNAL("Unknown binary operator kind '%d'", operator_kind);
  }
}

/// Compile unary expression.
static void compile_unary_expr(void) {
  LexerTokenKind const operator_kind = parser.previous.kind;
  compile_precedence_expr(PRECEDENCE_UNARY);

  switch (operator_kind) {
    case LEXER_TOKEN_MINUS: {
      emit_instruction(CHUNK_OP_NEGATE);
      break;
    }
    case LEXER_TOKEN_BANG: {
      emit_instruction(CHUNK_OP_NOT);
      break;
    }
    default: ERROR_INTERNAL("Unknown unary operator kind '%d'", operator_kind);
  }
}

/// Compile '(...)' grouping expression.
static void compile_grouping_expr(void) {
  compile_expr();
  compiler_consume(LEXER_TOKEN_CLOSE_PAREN, "Expected ')' closing grouping expression");
}

/// Compile numeric literal.
static void compile_numeric_literal(void) {
  errno = 0;
  double const value = strtod(parser.previous.as.basic.lexeme, NULL);
  if (errno != 0) {
    ERROR_MEMORY(
      COMMON_FILE_LINE_COLUMN_FORMAT COMMON_MS "Out-of-range numeric literal '%.*s'", g_source_file_path,
      parser.previous.line, parser.previous.column, parser.previous.as.basic.lexeme_length,
      parser.previous.as.basic.lexeme
    );
  }
  emit_constant_instruction(value_make_number(value));
}

/// Compile string literal.
static void compile_string_literal(void) {
  int const content_length = parser.previous.as.string.content_length;
  char const *const content = parser.previous.as.string.content;
  EntityString *const string_entity = entity_string_adopt(content, content_length);

  emit_constant_instruction(value_make_entity((Entity *)string_entity));
}

/// Compile invariable literal (one with fixed lexeme).
static void compile_invariable_literal(void) {
  LexerTokenKind const literal_kind = parser.previous.kind;

  switch (literal_kind) {
    case LEXER_TOKEN_NIL: {
      emit_instruction(CHUNK_OP_NIL);
      break;
    }
    case LEXER_TOKEN_TRUE: {
      emit_instruction(CHUNK_OP_TRUE);
      break;
    }
    case LEXER_TOKEN_FALSE: {
      emit_instruction(CHUNK_OP_FALSE);
      break;
    }
    default: ERROR_INTERNAL("Unknown invariable literal kind '%d'", literal_kind);
  }
}

/// Compile expression statement.
static void compile_expr_stmt(void) {
  compile_expr();
  compiler_consume(LEXER_TOKEN_SEMICOLON, "Expected ';' terminating expression statement");
  emit_instruction(CHUNK_OP_POP); // discard expression result
}

/// Compile print statement.
static void compile_print_stmt(void) {
  compile_expr();
  compiler_consume(LEXER_TOKEN_SEMICOLON, "Expected ';' terminating print statement");
  emit_instruction(CHUNK_OP_PRINT);
}

/// Compile statement.
static void compile_stmt(void) {
  if (compiler_match(LEXER_TOKEN_PRINT)) compile_print_stmt();
  else compile_expr_stmt();
}

/// Initialize parser with `source_code`.
static void parser_init(char const *const source_code) {
  lexer_init(source_code);

  parser.state = PARSER_OK;
  parser.had_error = false;
  parser.has_current = false;
  parser.has_previous = false;

  compiler_advance();
  parser.has_current = true;
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Compile `source_code` into bytecode instructions and append them to `chunk`.
/// @pre VM is initialized (this requirement originates in lexer).
/// @return Compiler status indicating compilation result.
CompilerStatus compiler_compile(char const *const source_code, Chunk *const chunk) {
  assert(source_code != NULL);
  assert(chunk != NULL);

  // reset compiler
  current_chunk = chunk; // TEMP
  parser_init(source_code);

  // compile source_code
  while (!compiler_match(LEXER_TOKEN_EOF)) compile_stmt();

  emit_instruction(CHUNK_OP_RETURN); // TEMP

#ifdef DEBUG_COMPILER
  if (!parser.had_error) debug_disassemble_chunk(chunk, "DEBUG_COMPILER");
#endif

  // clean up
  if (parser.has_current) lexer_token_free(parser.current);
  if (parser.has_previous) lexer_token_free(parser.previous);

  if (!parser.had_error) return COMPILER_SUCCESS;
  if (parser.state == PARSER_UNEXPECTED_EOF) return COMPILER_UNEXPECTED_EOF;
  return COMPILER_FAILURE;
}
