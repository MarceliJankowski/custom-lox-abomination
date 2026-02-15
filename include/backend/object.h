#ifndef OBJECT_H
#define OBJECT_H

#include "utils/memory.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

/// Make `ctype` CLA Object of `object_type`.
/// @param ctype Concrete C type of Object to be made.
/// @param object_type ObjectType value corresponding to `ctype`.
/// @result Pointer to made Object.
#define OBJECT_MAKE(ctype, object_type) ((ctype *)object_make(sizeof(ctype), (object_type)))

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// CLA object type.
typedef enum {
  OBJECT_STRING,
  OBJECT_TYPE_COUNT,
} ObjectType;

/// CLA garbage-collected value.
typedef struct Object Object;
struct Object {
  Object *next;
  ObjectType type;
};

/// CLA string object.
typedef struct {
  Object object;
  int length;
  bool is_content_owner;
  char *content;
} ObjectString;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*
// Objects are passed by pointers to prevent object slicing

Object *object_make(size_t size, ObjectType type);
ObjectString *object_make_owning_string(char const *content, int content_length);
ObjectString *object_make_non_owning_string(char const *content, int content_length);
char const *object_get_type_string(Object const *object);
void object_print(Object const *object);
bool object_equals(Object const *object_a, Object const *object_b);

#endif // OBJECT_H
