#ifndef VALUE_H
#define VALUE_H

#include "utils/darray.h"

#include <stdbool.h>
#include <stdint.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// CLA value type.
typedef enum {
  VALUE_NIL,
  VALUE_BOOL,
  VALUE_NUMBER,
  VALUE_TYPE_COUNT,
} ValueType;

/// CLA value.
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } payload;
} Value;

/// Dynamic array used for storing CLA values.
typedef DARRAY_TYPE(Value) ValueList;

// *---------------------------------------------*
// *             OBJECT DECLARATIONS             *
// *---------------------------------------------*

extern char const *const value_type_to_string_table[];

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void value_list_init(ValueList *value_list);
void value_list_append(ValueList *value_list, Value value);
void value_list_destroy(ValueList *value_list);
void value_print(Value value);
bool value_equals(Value value_a, Value value_b);

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/// Make CLA nil value.
/// @return Made CLA nil value.
inline Value value_make_nil(void) {
  return (Value){
    VALUE_NIL,
    {0},
  };
}

/// Make CLA bool value from `boolean`.
/// @return Made CLA bool value.
inline Value value_make_bool(bool const boolean) {
  return (Value){
    VALUE_BOOL,
    {.boolean = boolean},
  };
}

/// Make CLA number value from `number`.
/// @return Made CLA number value.
inline Value value_make_number(double const number) {
  return (Value){
    VALUE_NUMBER,
    {.number = number},
  };
}

/// Determine whether CLA `value` is of bool type.
/// @return true if it is, false otherwise.
inline bool value_is_bool(Value const value) {
  return value.type == VALUE_BOOL;
}

/// Determine whether CLA `value` is of nil type.
/// @return true if it is, false otherwise.
inline bool value_is_nil(Value const value) {
  return value.type == VALUE_NIL;
}

/// Determine whether CLA `value` is of number type.
/// @return true if it is, false otherwise.
inline bool value_is_number(Value const value) {
  return value.type == VALUE_NUMBER;
}

/// Determine whether CLA `value` is falsy.
/// @return true if it is, false otherwise.
inline bool value_is_falsy(Value const value) {
  return value_is_nil(value) || (value_is_bool(value) && value.payload.boolean == false);
}

#endif // VALUE_H
