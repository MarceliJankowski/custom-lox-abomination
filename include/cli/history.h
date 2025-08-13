#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void history_init(void);
void history_destroy(void);
void history_append_entry(char const *entry, size_t entry_length);
void history_browse_older_entry(void);
void history_browse_newer_entry(void);
void history_stop_browsing(void);
bool history_is_browsed(void);
bool history_is_newest_entry_browsed(void);
char const *history_get_browsed_entry(void);

#endif // HISTORY_H
