#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <stddef.h>

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
void gap_buffer_insert(GapBuffer *gap_buffer, char character);
void gap_buffer_delete_previous(GapBuffer *gap_buffer);
void gap_buffer_clear_content(GapBuffer *gap_buffer);
char *gap_buffer_get_content(GapBuffer const *gap_buffer);
size_t gap_buffer_get_content_length(GapBuffer const *gap_buffer);

#endif // GAP_BUFFER_H
