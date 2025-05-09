#ifndef VALUE_H
#define VALUE_H

#include "utils/darray.h"

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
typedef DARRAY_TYPE(Value) ValueList;

#define VALUE_MAKE_NIL() ((Value){VALUE_NIL, {.number = 0}})
#define VALUE_MAKE_BOOL(value) ((Value){VALUE_BOOL, {.boolean = value}})
#define VALUE_MAKE_NUMBER(value) ((Value){VALUE_NUMBER, {.number = value}})

#define VALUE_IS_BOOL(value) ((value).type == VALUE_BOOL)
#define VALUE_IS_NIL(value) ((value).type == VALUE_NIL)
#define VALUE_IS_NUMBER(value) ((value).type == VALUE_NUMBER)

#define VALUE_IS_FALSY(value) VALUE_IS_NIL(value) || (VALUE_IS_BOOL(value) && !value.payload.boolean)

extern char const *const value_type_to_string_table[];

void value_list_init(ValueList *value_list);
void value_list_append(ValueList *value_list, Value value);
void value_list_free(ValueList *value_list);
void value_print(Value value);
bool value_equals(Value value_a, Value value_b);

#endif // VALUE_H
