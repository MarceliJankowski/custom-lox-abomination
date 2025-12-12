#include "frontend/compiler.h"

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

typedef enum { ERROR_LEXICAL, ERROR_SYNTAX, ERROR_SEMANTIC, ERROR_TYPE_COUNT } ErrorType;

/// Token precedence.
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

/// Function handling (compiling) specified TokenType.
typedef void(TokenHandlerFn)(void);

typedef struct {
  TokenHandlerFn *nud; // null-denotation (requires no-context)
  TokenHandlerFn *led; // left-denotation (requires left-context subexpression)
  Precedence precedence;
} ParseRule;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

static TokenHandlerFn compile_binary_expr;
static TokenHandlerFn compile_unary_expr;
static TokenHandlerFn compile_grouping_expr;
static TokenHandlerFn compile_numeric_literal;
static TokenHandlerFn compile_invariable_literal;

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static_assert(LEXER_TOKEN_TYPE_COUNT - LEXER_TOKEN_INDICATOR_COUNT == 41, "Exhaustive TokenType handling");
static ParseRule const parse_rules[] = {
  // literals
  [LEXER_TOKEN_NIL] = {compile_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_TRUE] = {compile_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_FALSE] = {compile_invariable_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_NUMBER] = {compile_numeric_literal, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_STRING] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_IDENTIFIER] = {NULL, NULL, PRECEDENCE_NONE},

  // single-character tokens
  [LEXER_TOKEN_PLUS] = {NULL, compile_binary_expr, PRECEDENCE_TERM},
  [LEXER_TOKEN_MINUS] = {compile_unary_expr, compile_binary_expr, PRECEDENCE_TERM},
  [LEXER_TOKEN_STAR] = {NULL, compile_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_SLASH] = {NULL, compile_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_PERCENT] = {NULL, compile_binary_expr, PRECEDENCE_FACTOR},
  [LEXER_TOKEN_BANG] = {compile_unary_expr, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [LEXER_TOKEN_LESS] = {NULL, compile_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_GREATER] = {NULL, compile_binary_expr, PRECEDENCE_COMPARISON},
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
  [LEXER_TOKEN_EQUAL_EQUAL] = {NULL, compile_binary_expr, PRECEDENCE_EQUALITY},
  [LEXER_TOKEN_BANG_EQUAL] = {NULL, compile_binary_expr, PRECEDENCE_EQUALITY},
  [LEXER_TOKEN_LESS_EQUAL] = {NULL, compile_binary_expr, PRECEDENCE_COMPARISON},
  [LEXER_TOKEN_GREATER_EQUAL] = {NULL, compile_binary_expr, PRECEDENCE_COMPARISON},

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
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Retrieve currently compiled chunk.
static inline Chunk *get_current_chunk(void) {
  return current_chunk;
}

/// Handle `error_type` error at `token` with `message`.
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
    default: ERROR_INTERNAL("Unknown error_type '%d'", error_type);
  }
  io_fprintf(
    g_static_analysis_error_stream, COMMON_MS COMMON_FILE_LINE_COLUMN_FORMAT COMMON_MS "%s", g_source_file_path,
    token->line, token->column, message
  );
  if (token->type == LEXER_TOKEN_ERROR || token->type == LEXER_TOKEN_EOF) {
    io_fprintf(g_static_analysis_error_stream, "\n");
  } else io_fprintf(g_static_analysis_error_stream, " at '%.*s'\n", token->lexeme_length, token->lexeme);
}

/// Handle `error_type` error at parser.previous token with `message`.
static inline void compiler_error_at_previous(ErrorType const error_type, char const *const message) {
  compiler_error_at(error_type, &parser.previous, message);
}

/// Handle `error_type` error at parser.current token with `message`.
static inline void compiler_error_at_current(ErrorType const error_type, char const *const message) {
  compiler_error_at(error_type, &parser.current, message);
}

/// Update parser.previous, advance parser.current, and handle any error tokens.
static void compiler_advance(void) {
  parser.previous = parser.current;

  for (;;) {
    parser.current = lexer_scan();
    if (parser.current.type != LEXER_TOKEN_ERROR) break;
    compiler_error_at_current(ERROR_LEXICAL, parser.current.lexeme);
  }
}

/// Advance compiler if parser.current.type matches `type`, report error with `message` otherwise.
static inline void compiler_consume(LexerTokenType const type, char const *const message) {
  if (parser.current.type != type) compiler_error_at_current(ERROR_SYNTAX, message);
  compiler_advance();
}

/// Advance compiler if parser.current.type matches `type`.
/// @return true if it does, false otherwise.
static inline bool compiler_match(LexerTokenType const type) {
  if (parser.current.type != type) return false;
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

/// Compile expression.
static void compile_expr(void) {
  compile_precedence_expr(PRECEDENCE_ASSIGNMENT);
}

/// Compile binary expression.
static void compile_binary_expr(void) {
  LexerTokenType const operator_type = parser.previous.type;
  compile_precedence_expr(parse_rules[operator_type].precedence + 1);

  switch (operator_type) {
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
    default: ERROR_INTERNAL("Unknown binary operator type '%d'", operator_type);
  }
}

/// Compile unary expression.
static void compile_unary_expr(void) {
  LexerTokenType const operator_type = parser.previous.type;
  compile_precedence_expr(PRECEDENCE_UNARY);

  switch (operator_type) {
    case LEXER_TOKEN_MINUS: {
      emit_instruction(CHUNK_OP_NEGATE);
      break;
    }
    case LEXER_TOKEN_BANG: {
      emit_instruction(CHUNK_OP_NOT);
      break;
    }
    default: ERROR_INTERNAL("Unknown unary operator type '%d'", operator_type);
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
  double const value = strtod(parser.previous.lexeme, NULL);
  if (errno != 0) {
    ERROR_MEMORY(
      COMMON_FILE_LINE_COLUMN_FORMAT COMMON_MS "Out-of-range numeric literal '%.*s'", g_source_file_path,
      parser.previous.line, parser.previous.column, parser.previous.lexeme_length, parser.previous.lexeme
    );
  }
  emit_constant_instruction(value_make_number(value));
}

/// Compile invariable literal (one with fixed lexeme).
static void compile_invariable_literal(void) {
  LexerTokenType const literal_type = parser.previous.type;

  switch (literal_type) {
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
    default: ERROR_INTERNAL("Unknown invariable literal type '%d'", literal_type);
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

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Compile `source_code` into bytecode instructions and append them to `chunk`.
/// @return Compiler status indicating compilation result.
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
  while (!compiler_match(LEXER_TOKEN_EOF)) compile_stmt();

  emit_instruction(CHUNK_OP_RETURN); // TEMP

#ifdef DEBUG_COMPILER
  if (!parser.had_error) debug_disassemble_chunk(chunk, "DEBUG_COMPILER");
#endif

  if (!parser.had_error) return COMPILER_SUCCESS;
  if (parser.state == PARSER_UNEXPECTED_EOF) return COMPILER_UNEXPECTED_EOF;
  return COMPILER_FAILURE;
}
