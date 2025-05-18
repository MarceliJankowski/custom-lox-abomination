#ifndef INTERPRETER_H
#define INTERPRETER_H

typedef enum {
  INTERPRETER_SUCCESS,
  INTERPRETER_COMPILER_FAILURE,
  INTERPRETER_COMPILER_UNEXPECTED_EOF,
  INTERPRETER_VM_FAILURE,
  INTERPRETER_STATUS_COUNT,
} InterpreterStatus;

void interpreter_init(void);
void interpreter_destroy(void);
InterpreterStatus interpreter_interpret(char const *source_code);

#endif // INTERPRETER_H
