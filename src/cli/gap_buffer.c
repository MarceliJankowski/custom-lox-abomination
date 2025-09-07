#include "cli/gap_buffer.h"

#include "utils/error.h"
#include "utils/io.h"

#include <stdlib.h>
#include <string.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define GAP_BUFFER_GROWTH_FACTOR 2

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

size_t gap_buffer_get_cursor_position(GapBuffer const *gap_buffer);

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc resize `gap_buffer` to `new_capacity`*/
static void gap_buffer_resize(GapBuffer *const gap_buffer, size_t const new_capacity) {
  assert(gap_buffer != NULL);

  // update gap end
  size_t const post_gap_content_length = gap_buffer->capacity - gap_buffer->gap_end;
  size_t const old_gap_end = gap_buffer->gap_end;
  gap_buffer->gap_end = gap_buffer->capacity - post_gap_content_length;

  // grow buffer
  gap_buffer->buffer = realloc(gap_buffer->buffer, new_capacity);
  if (gap_buffer->buffer == NULL) ERROR_MEMORY_ERRNO();

  // copy post-gap content past the new gap end
  memmove(gap_buffer->buffer + gap_buffer->gap_end, gap_buffer->buffer + old_gap_end, post_gap_content_length);
}

/**@desc grow `gap_buffer` to its next capacity*/
static void gap_buffer_grow(GapBuffer *const gap_buffer) {
  assert(gap_buffer != NULL);

  size_t const new_capacity = gap_buffer->capacity * GAP_BUFFER_GROWTH_FACTOR;

  gap_buffer_resize(gap_buffer, new_capacity);
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

  char *content_buffer = malloc(content_length + 1); // account for NUL terminator
  if (!content_buffer) ERROR_MEMORY_ERRNO();

  // fill content_buffer
  memcpy(content_buffer, gap_buffer->buffer, pre_gap_content_length);
  memcpy(content_buffer + pre_gap_content_length, gap_buffer->buffer + gap_buffer->gap_end, post_gap_content_length);
  content_buffer[content_length] = '\0'; // append NUL terminator

  return content_buffer;
}

/**@desc replace `gap_buffer` content with `new_content`; effectively loading it in.
`gap_buffer` cursor gets positioned at the end of `new_content`.
Memory area of `new_content` musn't overlap with `gap_buffer.buffer`*/
void gap_buffer_load_content(GapBuffer *const gap_buffer, char const *const new_content) {
  assert(gap_buffer != NULL);
  assert(new_content != NULL);

  size_t const new_content_length = strlen(new_content);

  // resize if needed
  size_t new_capacity = gap_buffer->capacity;
  while (new_capacity < new_content_length) new_capacity *= GAP_BUFFER_GROWTH_FACTOR;
  gap_buffer_resize(gap_buffer, new_capacity);

  // load new_content
  gap_buffer_clear_content(gap_buffer);
  memcpy(gap_buffer->buffer, new_content, new_content_length);
  gap_buffer->gap_start = new_content_length;
}

/**@desc print `gap_buffer` content*/
inline void gap_buffer_print_content(GapBuffer const *const gap_buffer) {
  assert(gap_buffer != NULL);

  char *const content = gap_buffer_get_content(gap_buffer);
  io_printf("%s", content);
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
