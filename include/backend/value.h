#ifndef VALUE_H
#define VALUE_H

#include "backend/entity.h"
#include "utils/darray.h"

#include <stdbool.h>
#include <stdint.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// CLA value kind.
typedef enum {
  VALUE_NIL,
  VALUE_BOOL,
  VALUE_NUMBER,
  VALUE_ENTITY,
  VALUE_KIND_COUNT,
} ValueKind;

/// CLA value.
typedef struct {
  ValueKind kind;
  union {
    bool boolean;
    double number;
    Entity *entity;
  } as;
} Value;

/// Dynamic array used for storing CLA values.
typedef DARRAY_TYPE(Value) ValueList;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

char const *value_get_kind_string(Value value);
void value_list_init(ValueList *value_list);
void value_list_append(ValueList *value_list, Value value);
void value_list_destroy(ValueList *value_list);
void value_print(Value value);
bool value_equals(Value value_a, Value value_b);
EntityString *value_to_string_entity(Value value);

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

/// Make CLA entity value from `entity`.
/// @return Made CLA entity value.
inline Value value_make_entity(Entity *const entity) {
  return (Value){
    VALUE_ENTITY,
    {.entity = entity},
  };
}

/// Determine whether CLA `value` is of bool kind.
/// @return true if it is, false otherwise.
inline bool value_is_bool(Value const value) {
  return value.kind == VALUE_BOOL;
}

/// Determine whether CLA `value` is of nil kind.
/// @return true if it is, false otherwise.
inline bool value_is_nil(Value const value) {
  return value.kind == VALUE_NIL;
}

/// Determine whether CLA `value` is of number kind.
/// @return true if it is, false otherwise.
inline bool value_is_number(Value const value) {
  return value.kind == VALUE_NUMBER;
}

/// Determine whether CLA `value` is of string kind.
/// @return true if it is, false otherwise.
inline bool value_is_string(Value const value) {
  return value.kind == VALUE_ENTITY && value.as.entity->kind == ENTITY_STRING;
}

/// Determine whether CLA `value` is falsy.
/// @return true if it is, false otherwise.
inline bool value_is_falsy(Value const value) {
  return value_is_nil(value) || (value_is_bool(value) && value.as.boolean == false);
}

#endif // VALUE_H
