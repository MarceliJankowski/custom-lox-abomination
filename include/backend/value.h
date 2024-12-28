#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include <stdint.h>

/**@desc CLA value type*/
typedef enum {
  VALUE_NIL,
  VALUE_BOOL,
  VALUE_NUMBER,
  VALUE_TYPE_COUNT,
} ValueType;

/**@desc CLA value*/
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } payload;
} Value;

/**@desc dynamic array used for storing CLA values*/
typedef struct {
  Value *values;
  int32_t capacity, count;
} ValueArray;

#define NIL_VALUE() ((Value){VALUE_NIL, {.number = 0}})
#define BOOL_VALUE(value) ((Value){VALUE_BOOL, {.boolean = value}})
#define NUMBER_VALUE(value) ((Value){VALUE_NUMBER, {.number = value}})

#define IS_BOOL_VALUE(value) ((value).type == VALUE_BOOL)
#define IS_NIL_VALUE(value) ((value).type == VALUE_NIL)
#define IS_NUMBER_VALUE(value) ((value).type == VALUE_NUMBER)

extern char const *const value_type_to_string_table[];

void value_array_init(ValueArray *array);
void value_array_append(ValueArray *array, Value value);
void value_array_free(ValueArray *array);
void value_print(Value value);

#endif // VALUE_H
