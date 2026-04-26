#include "backend/entity.h"

#include "backend/gc.h"
#include "backend/vm.h"
#include "utils/io.h"

#include <string.h>

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Hash `content` of `content_length`.
/// @param content Pointer to character sequence, or NULL.
/// @param content_length Length of `content` (0 when `content` is NULL, otherwise positive).
/// @return Computed hash.
static inline uint32_t hash_string_entity_content(char const *const content, int const content_length) {
  assert(content_length >= 0);

  // TODO: look into replacing this hashing algorithm with a more suitable one
  uint32_t hash = 2166136261u;
  for (int i = 0; i < content_length; i++) {
    hash ^= (uint8_t)content[i];
    hash *= 16777619;
  }

  return hash;
}

/// Make CLA string entity.
/// @param content Pointer to GC character sequence, or NULL.
/// @param content_length Length of `content` (0 when `content` is NULL, otherwise positive).
/// @return Pointer to made string entity.
static inline EntityString *entity_make_string(char const *const content, int const content_length) {
  assert((content != NULL && content_length > 0) || (content == NULL && content_length == 0));

  EntityString *const string_entity = ENTITY_MAKE(EntityString, ENTITY_STRING);
  string_entity->content = (char *)content;
  string_entity->content_length = content_length;
  string_entity->hash = hash_string_entity_content(content, content_length);

  return string_entity;
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Make CLA entity of `size` and `kind`.
/// @return Pointer to made Entity.
Entity *entity_make(size_t const size, EntityKind const kind) {
  Entity *const entity = gc_allocate(size);
  entity->kind = kind;

  entity->next = vm.entities;
  vm.entities = entity;

  return entity;
}

/// Make CLA string entity by adopting GC `content` of `content_length`.
/// @param content Pointer to GC character sequence, or NULL.
/// @param content_length Length of `content` (0 when `content` is NULL, otherwise positive).
/// @return Pointer to made string entity.
EntityString *entity_string_adopt(char const *const content, int const content_length) {
  assert((content != NULL && content_length > 0) || (content == NULL && content_length == 0));

  return entity_make_string(content, content_length);
}

/// Make CLA string entity by copying `content` of `content_length`.
/// @param content Pointer to character sequence, or NULL.
/// @param content_length Length of `content` (0 when `content` is NULL, otherwise positive).
/// @return Pointer to made string entity.
EntityString *entity_string_copy(char const *const content, int const content_length) {
  assert((content != NULL && content_length > 0) || (content == NULL && content_length == 0));

  char *const gc_content = gc_allocate(content_length);
  memcpy(gc_content, content, content_length);

  return entity_make_string(gc_content, content_length);
}

/// Get string with description of `entity` kind.
/// @return `entity` kind string.
char const *entity_get_kind_string(Entity const *const entity) {
  assert(entity != NULL);

  static_assert(ENTITY_KIND_COUNT == 1, "Exhaustive EntityKind handling");
  switch (entity->kind) {
    case ENTITY_STRING: return "string";

    default: ERROR_INTERNAL("Unknown EntityKind '%d'", entity->kind);
  }
}

/// Print `entity`.
void entity_print(Entity const *const entity) {
  assert(entity != NULL);

  static_assert(ENTITY_KIND_COUNT == 1, "Exhaustive EntityKind handling");
  switch (entity->kind) {
    case ENTITY_STRING: {
      EntityString const *const string_entity = (EntityString *)entity;
      io_fprintf(g_source_program_output_stream, "%.*s", (int)string_entity->content_length, string_entity->content);
      break;
    }

    default: ERROR_INTERNAL("Unknown EntityKind '%d'", entity->kind);
  }
}

/// Determine whether `entity_a` equals `entity`.
/// @return true if it does, false otherwise.
bool entity_equals(Entity const *const entity_a, Entity const *const entity_b) {
  assert(entity_a != NULL);
  assert(entity_b != NULL);

  if (entity_a->kind != entity_b->kind) return false;

  static_assert(ENTITY_KIND_COUNT == 1, "Exhaustive EntityKind handling");
  switch (entity_a->kind) {
    case ENTITY_STRING: {
      EntityString const *const string_entity_a = (EntityString *)entity_a;
      EntityString const *const string_entity_b = (EntityString *)entity_b;

      if (string_entity_a->content_length != string_entity_b->content_length) return false;
      return memcmp(string_entity_a->content, string_entity_b->content, string_entity_a->content_length) == 0;
    }

    default: ERROR_INTERNAL("Unknown EntityKind '%d'", entity_a->kind);
  }
}
