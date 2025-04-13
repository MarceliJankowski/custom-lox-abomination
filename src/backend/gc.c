#include "backend/gc.h"

#include "utils/error.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/**@desc manage `object` memory with garbage collector.
Memory of objects tracked by garbage collector must be managed exclusively by this function (from the get-go).
There are four available operations:
- allocate new object (`object` == NULL, `old_size` == 0, `new_size` > 0);
- expand `object` (`object` != NULL, `old_size` > 0, `new_size` > `old_size`);
- shrink `object` (`object` != NULL, `old_size` > 0, `new_size` < `old_size`);
- deallocate `object` (`object` != NULL || `object` == NULL, `old_size` >= 0, `new_size` == 0);
Deallocation of non-existing objects is supported for convenience's sake.
@return pointer to (re)allocated `object` or NULL if deallocation was performed*/
void *gc_manage_memory(void *const object, size_t const old_size, size_t const new_size) {
  (void)old_size; // TEMP

  assert(
    !(object == NULL && old_size > 0 && new_size != 0) && "Invalid operation; can't allocate object with existing size"
  );
  assert(
    !(object != NULL && old_size == new_size && new_size != 0) &&
    "Invalid operation; can't determine whether to shrink or expand object"
  );
  assert(
    !(object != NULL && old_size == 0 && new_size != 0) &&
    "Invalid operation; existing object of size 0 can only be deallocated"
  );

  if (new_size == 0) {
    free(object);
    return NULL;
  }

  void *const reallocated_object = realloc(object, new_size);
  if (reallocated_object == NULL) ERROR_MEMORY("%s", strerror(errno));

  return reallocated_object;
}
