#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/**@desc gap buffer data structure tailored for REPL*/
typedef struct {
  char *buffer;
  size_t capacity, gap_start, gap_end;
} GapBuffer;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void gap_buffer_init(GapBuffer *gap_buffer, size_t initial_capacity);
void gap_buffer_destroy(GapBuffer *gap_buffer);
void gap_buffer_insert_char(GapBuffer *gap_buffer, char character);
bool gap_buffer_delete_char_left(GapBuffer *gap_buffer);
bool gap_buffer_delete_char_right(GapBuffer *gap_buffer);
bool gap_buffer_delete_word_left(GapBuffer *gap_buffer);
bool gap_buffer_delete_word_right(GapBuffer *gap_buffer);
bool gap_buffer_delete_content_left(GapBuffer *gap_buffer);
void gap_buffer_clear_content(GapBuffer *gap_buffer);
char *gap_buffer_get_content(GapBuffer const *gap_buffer);
void gap_buffer_load_content(GapBuffer *gap_buffer, char const *new_content);
void gap_buffer_print_content(GapBuffer const *gap_buffer);
size_t gap_buffer_get_content_length(GapBuffer const *gap_buffer);
void gap_buffer_move_cursor_left_by_char(GapBuffer *gap_buffer);
void gap_buffer_move_cursor_left_by_word(GapBuffer *gap_buffer);
void gap_buffer_move_cursor_right_by_char(GapBuffer *gap_buffer);
void gap_buffer_move_cursor_right_by_word(GapBuffer *gap_buffer);

// *---------------------------------------------*
// *              INLINE FUNCTIONS               *
// *---------------------------------------------*

/**@desc get `gap_buffer` cursor index
@return cursor index*/
inline size_t gap_buffer_get_cursor_index(GapBuffer const *const gap_buffer) {
  assert(gap_buffer != NULL);

  return gap_buffer->gap_start;
}

#endif // GAP_BUFFER_H
