#include "backend/gc.h"

#include "backend/entity.h"
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

/// Deallocate garbage-collected CLA `entity`.
/// @pre VM is initialized.
static void gc_deallocate_cla_entity(Entity *const entity) {
  assert(entity != NULL);

  static_assert(ENTITY_KIND_COUNT == 1, "Exhaustive EntityKind handling");
  switch (entity->kind) {
    case ENTITY_STRING: {
      EntityString const *const entity_string = (EntityString *)entity;

      if (entity_string->is_content_owner) gc_deallocate(entity_string->content, entity_string->content_length);
      gc_deallocate(entity, sizeof(*entity_string));
      break;
    }

    default: ERROR_INTERNAL("Unknown EntityKind '%d'", entity->kind);
  }
}

// *---------------------------------------------*
// *        EXTERNAL-LINKAGE FUNCTIONS           *
// *---------------------------------------------*

/// Garbage collecting MemoryManagerFn implementation.
/// @note Memory of objects tracked by garbage collector must be managed exclusively by this function (from the get-go).
/// @pre VM is initialized.
/// @see MemoryManagerFn for further documentation.
void *gc_memory_manage(void *const object, size_t const old_size, size_t const new_size) {
  // TODO: implement garbage collecting

  return memory_manage(object, old_size, new_size);
}

/// Deallocate garbage-collected CLA entitites belonging to VM.
/// @pre VM is initialized.
void gc_deallocate_vm_entitites(void) {
  for (Entity *current_entity = vm.entities; current_entity != NULL;) {
    Entity *const next_entity = current_entity->next;

    gc_deallocate_cla_entity(current_entity);

    current_entity = next_entity;
  }
}
