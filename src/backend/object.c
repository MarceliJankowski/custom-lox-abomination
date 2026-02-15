#include "backend/object.h"

#include "backend/gc.h"
#include "backend/vm.h"
#include "utils/io.h"

// *---------------------------------------------*
// *        EXTERNAL-LINKAGE FUNCTIONS           *
// *---------------------------------------------*

/// Make CLA object of `size` and `type`.
/// @return Pointer to made Object.
Object *object_make(size_t const size, ObjectType const type) {
  Object *const object = gc_allocate(size);
  object->type = type;

  object->next = vm.gc_objects;
  vm.gc_objects = object;

  return object;
}

/// Make CLA string object from `content` of `content_length`.
/// @return Pointer to made string object.
ObjectString *object_make_string(char const *const content, int const content_length) {
  assert(content != NULL);
  assert(content_length >= 0);

  size_t const string_object_size = sizeof(ObjectString) + content_length;
  ObjectString *const string_object = (ObjectString *)object_make(string_object_size, OBJECT_STRING);
  string_object->length = content_length;
  memcpy(string_object->content, content, content_length);

  return string_object;
}

/// Make CLA string literal object from `content` of `content_length`.
/// @return Pointer to made string literal object.
ObjectStringLiteral *object_make_string_literal(char const *const content, int const content_length) {
  assert(content != NULL);
  assert(content_length >= 0);

  ObjectStringLiteral *const string_literal_object = OBJECT_MAKE(ObjectStringLiteral, OBJECT_STRING_LITERAL);
  string_literal_object->length = content_length;
  string_literal_object->content = content;

  return string_literal_object;
}

/// Get string with description of `object` type.
/// @return `object` type string.
char const *object_get_type_string(Object const *const object) {
  assert(object != NULL);

  static_assert(OBJECT_TYPE_COUNT == 2, "Exhaustive ObjectType handling");
  switch (object->type) {
    case OBJECT_STRING:
    case OBJECT_STRING_LITERAL: return "string";

    default: ERROR_INTERNAL("Unknown ObjectType '%d'", object->type);
  }
}

/// Print `object`.
void object_print(Object const *const object) {
  assert(object != NULL);

  static_assert(OBJECT_TYPE_COUNT == 2, "Exhaustive ObjectType handling");
  switch (object->type) {
    case OBJECT_STRING:
    case OBJECT_STRING_LITERAL: {
      ObjectString const *const string_object = (ObjectString *)object;
      io_fprintf(g_source_program_output_stream, "%.*s", (int)string_object->length, string_object->content);
      break;
    }

    default: ERROR_INTERNAL("Unknown ObjectType '%d'", object->type);
  }
}

/// Determine whether `object_a` equals `object_b`.
/// @return true if it does, false otherwise.
bool object_equals(Object const *const object_a, Object const *const object_b) {
  assert(object_a != NULL);
  assert(object_b != NULL);

  if (object_a->type != object_b->type) return false;

  static_assert(OBJECT_TYPE_COUNT == 2, "Exhaustive ObjectType handling");
  switch (object_a->type) {
    case OBJECT_STRING:
    case OBJECT_STRING_LITERAL: {
      ObjectString const *const string_object_a = (ObjectString *)object_a;
      ObjectString const *const string_object_b = (ObjectString *)object_b;

      if (string_object_a->length != string_object_b->length) return false;
      return memcmp(string_object_a->content, string_object_b->content, string_object_a->length) == 0;
    }

    default: ERROR_INTERNAL("Unknown ObjectType '%d'", object_a->type);
  }
}
