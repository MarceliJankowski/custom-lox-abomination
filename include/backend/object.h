#ifndef OBJECT_H
#define OBJECT_H

#include "utils/memory.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

/// Make `ctype` CLA Object of `object_kind`.
/// @param ctype Concrete C type of Object to be made.
/// @param object_kind ObjectKind value corresponding to `ctype`.
/// @result Pointer to made Object.
#define OBJECT_MAKE(ctype, object_kind) ((ctype *)object_make(sizeof(ctype), (object_kind)))

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// CLA object kind.
typedef enum {
  OBJECT_STRING,
  OBJECT_KIND_COUNT,
} ObjectKind;

/// CLA garbage-collected value.
typedef struct Object Object;
struct Object {
  Object *next;
  ObjectKind kind;
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

Object *object_make(size_t size, ObjectKind kind);
ObjectString *object_make_owning_string(char const *content, int content_length);
ObjectString *object_make_non_owning_string(char const *content, int content_length);
char const *object_get_kind_string(Object const *object);
void object_print(Object const *object);
bool object_equals(Object const *object_a, Object const *object_b);

#endif // OBJECT_H
