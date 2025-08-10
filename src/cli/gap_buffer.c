#include "cli/gap_buffer.h"

#include "utils/error.h"

#include <stdlib.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

size_t gap_buffer_get_cursor_position(GapBuffer const *gap_buffer);

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc grow `gap_buffer` to double its capacity*/
static void gap_buffer_grow(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  // update capacity
  size_t const old_capacity = gap_buffer->capacity;
  gap_buffer->capacity *= 2;

  // update gap end
  size_t const post_gap_content_length = old_capacity - gap_buffer->gap_end;
  size_t const old_gap_end = gap_buffer->gap_end;
  gap_buffer->gap_end = gap_buffer->capacity - post_gap_content_length;

  // grow buffer
  gap_buffer->buffer = realloc(gap_buffer->buffer, gap_buffer->capacity);
  if (gap_buffer->buffer == NULL) ERROR_MEMORY_ERRNO();

  // copy post-gap content past the new gap end
  memmove(gap_buffer->buffer + gap_buffer->gap_end, gap_buffer->buffer + old_gap_end, post_gap_content_length);
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc initialize `gap_buffer` with `initial_capacity` (must be positive)*/
void gap_buffer_init(GapBuffer *const gap_buffer, size_t const initial_capacity) {
  assert(gap_buffer != NULL);
  assert(initial_capacity > 0);

  gap_buffer->gap_start = 0;
  gap_buffer->gap_end = initial_capacity;
  gap_buffer->capacity = initial_capacity;
  gap_buffer->buffer = malloc(initial_capacity);
  if (gap_buffer->buffer == NULL) ERROR_MEMORY_ERRNO();
}

/**@desc free `gap_buffer` memory and set it to uninitialized state*/
void gap_buffer_destroy(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  free(gap_buffer->buffer);
  *gap_buffer = (GapBuffer){0};
}

/**@desc insert into `gap_buffer` a `character` at cursor position*/
void gap_buffer_insert(GapBuffer *const gap_buffer, char const character) {
  assert(gap_buffer != NULL);

  // grow if needed
  if (gap_buffer->gap_start == gap_buffer->gap_end) gap_buffer_grow(gap_buffer);

  // insert character at cursor position
  size_t const cursor_position = gap_buffer->gap_start;
  gap_buffer->buffer[cursor_position] = character;
  gap_buffer->gap_start++;
}

/**@desc delete character from `gap_buffer` left to cursor position (if such character exists)*/
void gap_buffer_delete_left(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  if (gap_buffer->gap_start > 0) gap_buffer->gap_start--;
}

/**@desc clear `gap_buffer` content while preserving its internal buffer*/
void gap_buffer_clear_content(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  gap_buffer->gap_start = 0;
  gap_buffer->gap_end = gap_buffer->capacity;
}

/**@desc get `gap_buffer` content length
@return content length in bytes; NUL terminator isn't accounted for*/
size_t gap_buffer_get_content_length(GapBuffer const *const gap_buffer) {
  assert(gap_buffer != NULL);

  size_t const pre_gap_content_length = gap_buffer->gap_start;
  size_t const post_gap_content_length = gap_buffer->capacity - gap_buffer->gap_end;
  size_t const content_length = pre_gap_content_length + post_gap_content_length;

  return content_length;
}

/**@desc get `gap_buffer` content
@return pointer to NUL terminated buffer with `gap_buffer` content*/
char *gap_buffer_get_content(GapBuffer const *const gap_buffer) {
  assert(gap_buffer != NULL);

  size_t const pre_gap_content_length = gap_buffer->gap_start;
  size_t const post_gap_content_length = gap_buffer->capacity - gap_buffer->gap_end;
  size_t const content_length = pre_gap_content_length + post_gap_content_length;

  // allocate content buffer
  char *content_buffer = malloc(content_length + 1); // account for NUL terminator
  if (!content_buffer) ERROR_MEMORY_ERRNO();

  // fill allocated content_buffer
  memcpy(content_buffer, gap_buffer->buffer, pre_gap_content_length);
  memcpy(content_buffer + pre_gap_content_length, gap_buffer->buffer + gap_buffer->gap_end, post_gap_content_length);
  content_buffer[content_length] = '\0'; // append NUL terminator

  return content_buffer;
}

/**@desc print `gap_buffer` content*/
inline void gap_buffer_print_content(GapBuffer const *const gap_buffer) {
  assert(gap_buffer != NULL);

  char *const content = gap_buffer_get_content(gap_buffer);
  printf("%s", content);
  free(content);
}

/**@desc move `gap_buffer` cursor one position to the left (if possible)*/
void gap_buffer_move_cursor_left(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  if (gap_buffer->gap_start == 0) return;

  gap_buffer->gap_start--;
  gap_buffer->gap_end--;
  gap_buffer->buffer[gap_buffer->gap_end] = gap_buffer->buffer[gap_buffer->gap_start];
}

/**@desc move `gap_buffer` cursor one position to the right (if possible)*/
void gap_buffer_move_cursor_right(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  if (gap_buffer->gap_end == gap_buffer->capacity) return;

  gap_buffer->buffer[gap_buffer->gap_start] = gap_buffer->buffer[gap_buffer->gap_end];
  gap_buffer->gap_start++;
  gap_buffer->gap_end++;
}
