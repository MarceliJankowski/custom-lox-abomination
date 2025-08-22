#include "cli/history.h"

#include "cli/gap_buffer.h"
#include "utils/error.h"

#include <assert.h>
#include <stdlib.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define HISTORY_SIZE 1000

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/**@desc ring buffer data structure tailored for REPL physical line history*/
typedef struct {
  char *entries[HISTORY_SIZE];
  int entry_count, oldest_entry_index, newest_entry_index;
  int browsed_entry_index; // -1 if none is browsed
} History;

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static History history;

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/**@desc initialize history*/
void history_init(void) {
  history = (History){.browsed_entry_index = -1};
}

/**@desc free history memory and set it to uninitialized state*/
void history_destroy(void) {
  for (int i = 0; i < history.entry_count; i++) free(history.entries[i]);
  history = (History){0};
}

/**@desc append `entry` (string) of `entry_length` to history.
This function will stop history browsing.*/
void history_append_entry(char const *const entry, size_t const entry_length) {
  assert(entry != NULL);

  history_stop_browsing();

  // copy entry
  char *const entry_copy = malloc(entry_length + 1); // account for NUL terminator
  if (entry_copy == NULL) ERROR_MEMORY_ERRNO();
  strcpy(entry_copy, entry);

  // history isn't full
  if (history.entry_count < HISTORY_SIZE) {
    history.entries[history.entry_count] = entry_copy;
    history.newest_entry_index = history.entry_count;
    history.entry_count++;
    return;
  }

  // history is full
  free(history.entries[history.oldest_entry_index]);
  history.entries[history.oldest_entry_index] = entry_copy;
  history.newest_entry_index = history.oldest_entry_index;
  history.oldest_entry_index = (history.oldest_entry_index + 1) % HISTORY_SIZE;
}

/**@desc browse older history entry (if such entry exists).
If history isn't already being browsed, this function will begin browsing it at the newest entry.*/
void history_browse_older_entry(void) {
  if (history.entry_count == 0) return; // history is empty
  if (history.browsed_entry_index == history.oldest_entry_index) return; // already at oldest entry

  if (!history_is_browsed()) { // begin browsing history
    history.browsed_entry_index = history.newest_entry_index;
    return;
  }

  history.browsed_entry_index = history.browsed_entry_index == 0 ? HISTORY_SIZE - 1 : history.browsed_entry_index - 1;
}

/**@desc browse newer history entry (if such entry exists)*/
void history_browse_newer_entry(void) {
  if (!history_is_browsed()) return;
  if (history_is_newest_entry_browsed()) return;

  history.browsed_entry_index = (history.browsed_entry_index + 1) % HISTORY_SIZE;
}

/**@desc stop browsing history*/
void history_stop_browsing(void) {
  history.browsed_entry_index = -1;
}

/**@desc determine whether history is being browsed
@return true if it is, false otherwise*/
bool history_is_browsed(void) {
  return history.browsed_entry_index != -1;
}

/**@desc determine whether newest history entry is being browsed
@return true if it is, false otherwise*/
bool history_is_newest_entry_browsed(void) {
  return history.browsed_entry_index == history.newest_entry_index;
}

/**@desc get browsed history entry (history must be browsed)
@return browsed history entry*/
char const *history_get_browsed_entry(void) {
  assert(history_is_browsed());

  return history.entries[history.browsed_entry_index];
}
