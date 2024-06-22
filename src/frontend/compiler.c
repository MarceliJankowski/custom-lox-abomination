#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "frontend/compiler.h"
#include "frontend/lexer.h"
#include "global.h"
#include "util/debug.h"
#include "util/error.h"

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
static void compiler_groupping_expr(void);
static void compiler_numeric_literal(void);

// *---------------------------------------------*
// *               STATIC OBJECTS                *
// *---------------------------------------------*

static_assert(TOKEN_TYPE_COUNT - TOKEN_INDICATOR_COUNT == 41, "Exhaustive TokenType handling");
static ParseRule const parse_rules[] = {
  // literals
  [TOKEN_STRING] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_NUMBER] = {compiler_numeric_literal, NULL, PRECEDENCE_NONE},
  [TOKEN_IDENTIFIER] = {NULL, NULL, PRECEDENCE_NONE},

  // single-character tokens
  [TOKEN_PLUS] = {NULL, compiler_binary_expr, PRECEDENCE_TERM},
  [TOKEN_MINUS] = {compiler_unary_expr, compiler_binary_expr, PRECEDENCE_TERM},
  [TOKEN_STAR] = {NULL, compiler_binary_expr, PRECEDENCE_FACTOR},
  [TOKEN_SLASH] = {NULL, compiler_binary_expr, PRECEDENCE_FACTOR},
  [TOKEN_PERCENT] = {NULL, compiler_binary_expr, PRECEDENCE_FACTOR},
  [TOKEN_BANG] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_LESS] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_GREATER] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_DOT] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_COMMA] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_COLON] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_SEMICOLON] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_QUESTION] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_OPEN_PAREN] = {compiler_groupping_expr, NULL, PRECEDENCE_NONE},
  [TOKEN_CLOSE_PAREN] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_OPEN_CURLY_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_CLOSE_CURLY_BRACE] = {NULL, NULL, PRECEDENCE_NONE},

  // multi-character tokens
  [TOKEN_BANG_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_LESS_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},

  // reserved identifiers (keywords)
  [TOKEN_TRUE] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_FALSE] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_VAR] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_NIL] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_AND] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_OR] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_FUN] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_RETURN] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_IF] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_ELSE] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_WHILE] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_FOR] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_CLASS] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_SUPER] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_THIS] = {NULL, NULL, PRECEDENCE_NONE},
  [TOKEN_PRINT] = {NULL, NULL, PRECEDENCE_NONE},
};

static Chunk *current_chunk; // TEMP

static struct {
  Token previous, current;
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
static void compiler_error_at(ErrorType const error_type, Token const *const token, char const *const message) {
  assert(token != NULL);
  assert(message != NULL);

  if (parser.state != PARSER_OK) return;

  // record error
  parser.state = token->type == TOKEN_EOF ? PARSER_UNEXPECTED_EOF : PARSER_PANIC;
  parser.had_error = true;

  // print error
  static_assert(ERROR_TYPE_COUNT == 3, "Exhaustive ErrorType handling");
  switch (error_type) {
    case ERROR_LEXICAL: {
      fprintf(g_static_err_stream, "[LEXICAL_ERROR]");
      break;
    }
    case ERROR_SYNTAX: {
      fprintf(g_static_err_stream, "[SYNTAX_ERROR]");
      break;
    }
    case ERROR_SEMANTIC: {
      fprintf(g_static_err_stream, "[SEMANTIC_ERROR]");
      break;
    }
    default: INTERNAL_ERROR("Unknown error_type '%d'", error_type);
  }
  fprintf(
    g_static_err_stream, M_S FILE_LINE_COLUMN_FORMAT M_S "%s", g_source_file, token->line, token->column, message
  );
  if (token->type == TOKEN_ERROR || token->type == TOKEN_EOF) fprintf(g_static_err_stream, "\n");
  else fprintf(g_static_err_stream, " at '%.*s'\n", token->lexeme_length, token->lexeme);
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
    if (parser.current.type != TOKEN_ERROR) break;
    compiler_error_at_current(ERROR_LEXICAL, parser.current.lexeme);
  }
}

/**@desc advance compiler if parser.current.type matches `type`, report error with `message` otherwise*/
static inline void compiler_consume(TokenType const type, char const *const message) {
  if (parser.current.type != type) compiler_error_at_current(ERROR_SYNTAX, message);
  compiler_advance();
}

/**@desc generate `opcode` bytecode instruction and append it to current_chunk*/
static inline void compiler_emit_instruction(OpCode const opcode) {
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
  TokenType const operator_type = parser.previous.type;
  ParseRule const *const parse_rule = parse_rules + operator_type;
  compiler_precedence_expr(parse_rule->precedence + 1);

  switch (operator_type) {
    case TOKEN_PLUS: {
      compiler_emit_instruction(OP_ADD);
      break;
    }
    case TOKEN_MINUS: {
      compiler_emit_instruction(OP_SUBTRACT);
      break;
    }
    case TOKEN_STAR: {
      compiler_emit_instruction(OP_MULTIPLY);
      break;
    }
    case TOKEN_SLASH: {
      compiler_emit_instruction(OP_DIVIDE);
      break;
    }
    case TOKEN_PERCENT: {
      compiler_emit_instruction(OP_MODULO);
      break;
    }
    default: INTERNAL_ERROR("Unknown binary operator type '%d'", operator_type);
  }
}

/**@desc compile unary expression*/
static void compiler_unary_expr(void) {
  TokenType const operator_type = parser.previous.type;
  compiler_precedence_expr(PRECEDENCE_UNARY);

  switch (operator_type) {
    case TOKEN_MINUS: {
      compiler_emit_instruction(OP_NEGATE);
      break;
    }
    default: INTERNAL_ERROR("Unknown unary operator type '%d'", operator_type);
  }
}

/**@desc compile '(...)' groupping expression*/
static void compiler_groupping_expr(void) {
  compiler_expr();
  compiler_consume(TOKEN_CLOSE_PAREN, "Missing ')' closing groupping expression");
}

/**@desc compile numeric literal*/
static void compiler_numeric_literal(void) {
  errno = 0;
  double const value = strtod(parser.previous.lexeme, NULL);
  if (errno != 0) {
    MEMORY_ERROR(
      FILE_LINE_COLUMN_FORMAT M_S "Out-of-range numeric literal '%.*s'", g_source_file, parser.previous.line,
      parser.previous.column, parser.previous.lexeme_length, parser.previous.lexeme
    );
  }
  compiler_emit_constant_instruction(value);
}

/**@desc compile `source_code` into bytecode instructions and append them to `chunk`
@return true if compilation succeeded, false otherwise*/
CompilationStatus compiler_compile(char const *const source_code, Chunk *const chunk) {
  assert(source_code != NULL);
  assert(chunk != NULL);

  // reset compiler
  lexer_init(source_code);
  parser.state = PARSER_OK;
  parser.had_error = false;
  current_chunk = chunk; // TEMP

  // compile source_code
  compiler_advance();
  compiler_expr();
  compiler_consume(TOKEN_EOF, "Expected EOF");
  compiler_emit_instruction(OP_RETURN); // TEMP

#ifdef DEBUG_COMPILER
  if (!parser.had_error) debug_disassemble_chunk(chunk, "DEBUG_COMPILER");
#endif

  if (!parser.had_error) return COMPILATION_SUCCESS;
  if (parser.state == PARSER_UNEXPECTED_EOF) return COMPILATION_UNEXPECTED_EOF;
  return COMPILATION_FAILURE;
}
