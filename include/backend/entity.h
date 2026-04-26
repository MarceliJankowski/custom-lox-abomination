#ifndef ENTITY_H
#define ENTITY_H

#include "utils/memory.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

/// Make `ctype` CLA Entity of `entity_kind`.
/// @param ctype Concrete C type of Entity to be made.
/// @param entity_kind EntityKind value corresponding to `ctype`.
/// @result Pointer to made Entity.
#define ENTITY_MAKE(ctype, entity_kind) ((ctype *)entity_make(sizeof(ctype), (entity_kind)))

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// CLA entity kind.
typedef enum {
  ENTITY_STRING,
  ENTITY_KIND_COUNT,
} EntityKind;

/// CLA garbage-collected value.
typedef struct Entity Entity;
struct Entity {
  Entity *next;
  EntityKind kind;
};

/// CLA string entity.
typedef struct {
  Entity entity;
  char *content;
  uint32_t hash;
  int content_length;
} EntityString;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*
// Entities are passed by pointers to prevent object slicing

Entity *entity_make(size_t size, EntityKind kind);
EntityString *entity_string_adopt(char const *content, int content_length);
EntityString *entity_string_copy(char const *content, int content_length);
char const *entity_get_kind_string(Entity const *entity);
void entity_print(Entity const *entity);
bool entity_equals(Entity const *entity_a, Entity const *entity_b);

#endif // ENTITY_H
