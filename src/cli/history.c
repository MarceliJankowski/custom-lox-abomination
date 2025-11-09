#include "cli/history.h"

#include "cli/gap_buffer.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/str.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#else // POSIX
#include <pwd.h>
#include <unistd.h>
#endif

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define HISTORY_SIZE 1000
#define HISTORY_FILE_NAME ".cla_history"

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// Ring buffer data structure tailored for REPL physical line history.
typedef struct {
  char *entries[HISTORY_SIZE];
  int entry_count, oldest_entry_index, newest_entry_index;
  int browsed_entry_index; // -1 if none is browsed
} History;

// *---------------------------------------------*
// *          INTERNAL-LINKAGE OBJECTS           *
// *---------------------------------------------*

static History history;
static char *history_file_path = NULL;
static FILE *history_file_append_stream = NULL;

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Get absolute path to history file.
/// @return Pointer to dynamically allocated string with history file path.
static char *history_file_get_path(void) {
#ifdef _WIN32
  char const *const appdata_roaming_dir_path = getenv("APPDATA");
  char const *const appdata_local_dir_path = getenv("LOCALAPPDATA");
  char const *const userprofile_dir_path = getenv("USERPROFILE");

  char const *history_dir_path;
  if (appdata_roaming_dir_path) history_dir_path = appdata_roaming_dir_path;
  else if (appdata_local_dir_path) history_dir_path = appdata_local_dir_path;
  else if (userprofile_dir_path) history_dir_path = userprofile_dir_path;
  else ERROR_SYSTEM("APPDATA/LOCALAPPDATA/USERPROFILE environment variables are unset");

  size_t const history_file_path_length = strlen(history_dir_path) + sizeof(HISTORY_FILE_NAME) + 1; // account for '\\'
  char *const history_file_path = malloc(history_file_path_length);
  if (!history_file_path) ERROR_MEMORY_ERRNO();

  if (sprintf(history_file_path, "%s\\%s", history_dir_path, HISTORY_FILE_NAME) < 0) ERROR_IO_ERRNO();

  return history_file_path;
#else // POSIX
  char const *home_dir_path = getenv("HOME");
  if (home_dir_path == NULL) {
    errno = 0;
    struct passwd const *const passwd_entry = getpwuid(getuid());
    if (passwd_entry == NULL) {
      if (errno != 0) ERROR_SYSTEM_ERRNO();
      ERROR_SYSTEM("No passwd entry for '%d' UID", getuid());
    }
    home_dir_path = passwd_entry->pw_dir;
  }

  size_t const history_file_path_length = strlen(home_dir_path) + sizeof(HISTORY_FILE_NAME) + 1; // account for '/'
  char *const history_file_path = malloc(history_file_path_length);
  if (!history_file_path) ERROR_MEMORY_ERRNO();

  if (sprintf(history_file_path, "%s/%s", home_dir_path, HISTORY_FILE_NAME) < 0) ERROR_IO_ERRNO();

  return history_file_path;
#endif
}

/// Trim history file so that entry count does not exceed HISTORY_SIZE (removes excessive oldest entries).
/// @note This function does nothing if history file fails to be opened for reading.
static void history_file_trim(void) {
  assert(history_file_path != NULL);

  FILE *history_file_stream = fopen(history_file_path, "rb");
  if (history_file_stream == NULL) return; // nothing to trim

  char *const history_file_content = io_read_binary_stream_resource_content(history_file_stream);
  size_t const entry_count = str_count_lines(history_file_content);

  if (entry_count <= HISTORY_SIZE) return;
  size_t const excessive_entry_count = entry_count - HISTORY_SIZE;

  // skip excessive oldest entries
  char const *current_char = history_file_content;
  for (size_t skipped_entry_count = 0; skipped_entry_count < excessive_entry_count; current_char++) {
    if (*current_char == '\n') skipped_entry_count++;
  }

  // clear history file's content
  history_file_stream = freopen(history_file_path, "wb", history_file_stream);
  if (history_file_stream == NULL) ERROR_IO_ERRNO();

  // write trimmed content
  io_fprintf(history_file_stream, current_char);
  if (fflush(history_file_stream) == EOF) ERROR_IO_ERRNO();

  free(history_file_content);
  if (fclose(history_file_stream)) ERROR_IO_ERRNO();
}

/// Trim history file and load its content into history data structure.
/// @note This function does nothing if history file fails to be opened for reading.
static void history_file_trim_and_load(void) {
  assert(history_file_path != NULL);

  history_file_trim();

  FILE *const history_file_stream = fopen(history_file_path, "rb");
  if (history_file_stream == NULL) return; // nothing to load

  char *const history_file_content = io_read_binary_stream_resource_content(history_file_stream);

  // load history_file_content into history data structure
  int entry_count = 0;
  int current_entry_size = 0;
  for (char const *current_char = history_file_content; *current_char != '\0'; current_char++) {
    // skip characters until we reach end of current entry
    current_entry_size++;
    if (*current_char != '\n') continue;

    // load current entry (unless it's comprised solely of a newline)
    if (current_entry_size > 1) {
      char const *const current_entry = current_char - (current_entry_size - 1);

      char *const entry_buffer = malloc(current_entry_size);
      if (entry_buffer == NULL) ERROR_MEMORY_ERRNO();

      memcpy(entry_buffer, current_entry, current_entry_size - 1);
      entry_buffer[current_entry_size - 1] = '\0';

      history.entries[entry_count] = entry_buffer;
    }

    entry_count++;
    current_entry_size = 0;
  }
  history.entry_count = entry_count;
  history.newest_entry_index = entry_count - 1;
  history.oldest_entry_index = 0;

  if (fclose(history_file_stream)) ERROR_IO_ERRNO();
}

/// Append `entry` to history file.
static void history_file_append(char const *const entry) {
  assert(history_file_append_stream != NULL);
  assert(entry != NULL);

  io_fprintf(history_file_append_stream, "%s\n", entry);
  if (fflush(history_file_append_stream) == EOF) ERROR_IO_ERRNO();
}

/// Determine whether oldest history entry is being browsed.
/// @return true if it is, false otherwise.
static inline bool history_is_oldest_entry_browsed(void) {
  return history.browsed_entry_index == history.oldest_entry_index;
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Initialize history.
void history_init(void) {
  assert(history_file_path == NULL);
  assert(history_file_append_stream == NULL);

  history = (History){.browsed_entry_index = -1};
  history_file_path = history_file_get_path();
  history_file_trim_and_load();

  history_file_append_stream = fopen(history_file_path, "ab");
  if (history_file_append_stream == NULL) ERROR_IO_ERRNO();
}

/// Release history resources and set it to uninitialized state.
void history_destroy(void) {
  assert(history_file_path != NULL);
  assert(history_file_append_stream != NULL);

  for (int i = 0; i < history.entry_count; i++) free(history.entries[i]);
  history = (History){0};

  free(history_file_path);
  if (fclose(history_file_append_stream)) ERROR_IO_ERRNO();
}

/// Append `entry` (string) of `entry_length` to history;
/// unless `entry` consists solely of whitespace characters or is a duplicate of newest history entry.
/// @note This function will stop history browsing.
void history_append_entry(char const *const entry, size_t const entry_length) {
  assert(entry != NULL);
  assert(strlen(entry) == entry_length);

  history_stop_browsing();

  if (str_is_all_whitespace(entry)) return;
  if (history.entry_count > 0) {
    char const *const newest_entry = history.entries[history.newest_entry_index];
    if (strcmp(entry, newest_entry) == 0) return;
  }

  char *const entry_copy = malloc(entry_length + 1); // account for NUL terminator
  if (entry_copy == NULL) ERROR_MEMORY_ERRNO();
  memcpy(entry_copy, entry, entry_length + 1);

  if (history.entry_count < HISTORY_SIZE) { // history isn't full
    history.entries[history.entry_count] = entry_copy;
    history.newest_entry_index = history.entry_count;
    history.entry_count++;
  } else { // history is full
    free(history.entries[history.oldest_entry_index]);
    history.entries[history.oldest_entry_index] = entry_copy;
    history.newest_entry_index = history.oldest_entry_index;
    history.oldest_entry_index = (history.oldest_entry_index + 1) % HISTORY_SIZE;
  }

  history_file_append(entry_copy);
}

/// Browse older history entry.
/// @note If history isn't already being browsed, this function will begin browsing it at the newest entry.
/// @return Pointer to older history entry (string), or NULL if such entry does not exist.
char const *history_browse_older_entry(void) {
  if (history.entry_count == 0) return NULL;
  if (history_is_oldest_entry_browsed()) return NULL;

  if (!history_is_browsed()) { // begin browsing history
    history.browsed_entry_index = history.newest_entry_index;
  } else {
    history.browsed_entry_index = history.browsed_entry_index == 0 ? HISTORY_SIZE - 1 : history.browsed_entry_index - 1;
  }

  return history.entries[history.browsed_entry_index];
}

/// Browse newer history entry.
/// @return Pointer to newer history entry (string), or NULL if such entry does not exist.
char const *history_browse_newer_entry(void) {
  if (!history_is_browsed()) return NULL;
  if (history_is_newest_entry_browsed()) return NULL;

  history.browsed_entry_index = (history.browsed_entry_index + 1) % HISTORY_SIZE;
  return history.entries[history.browsed_entry_index];
}

/// Stop browsing history.
void history_stop_browsing(void) {
  history.browsed_entry_index = -1;
}

/// Determine whether history is being browsed.
/// @return true if it is, false otherwise.
bool history_is_browsed(void) {
  return history.browsed_entry_index != -1;
}

/// Determine whether newest history entry is being browsed.
/// @return true if it is, false otherwise.
bool history_is_newest_entry_browsed(void) {
  return history.browsed_entry_index == history.newest_entry_index;
}
