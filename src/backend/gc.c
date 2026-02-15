#include "backend/gc.h"

#include "backend/object.h"
#include "backend/vm.h"
#include "utils/memory.h"

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void *gc_allocate(size_t new_size);
void *gc_reallocate(void *object, size_t old_size, size_t new_size);
void *gc_deallocate(void *object, size_t old_size);

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Deallocate garbage-collected CLA `object`.
static void gc_deallocate_cla_object(Object *const object) {
  assert(object != NULL);

  static_assert(OBJECT_TYPE_COUNT == 2, "Exhaustive ObjectType handling");
  switch (object->type) {
    case OBJECT_STRING: {
      int const content_length = ((ObjectString *)object)->length;
      gc_deallocate(object, sizeof(ObjectString) + content_length);
      break;
    }
    case OBJECT_STRING_LITERAL: {
      gc_deallocate(object, sizeof(ObjectStringLiteral));
      break;
    }

    default: ERROR_INTERNAL("Unknown ObjectType '%d'", object->type);
  }
}

// *---------------------------------------------*
// *        EXTERNAL-LINKAGE FUNCTIONS           *
// *---------------------------------------------*

/// Garbage collecting MemoryManagerFn implementation.
/// @note Memory of objects tracked by garbage collector must be managed exclusively by this function (from the get-go).
/// @see MemoryManagerFn for further documentation.
void *gc_memory_manage(void *const object, size_t const old_size, size_t const new_size) {
  // TODO: implement garbage collecting

  return memory_manage(object, old_size, new_size);
}

/// Deallocate garbage-collected CLA Objects belonging to VM.
void gc_deallocate_vm_gc_objects(void) {
  for (Object *current_object = vm.gc_objects; current_object != NULL;) {
    Object *const next_object = current_object->next;

    gc_deallocate_cla_object(current_object);

    current_object = next_object;
  }
}
