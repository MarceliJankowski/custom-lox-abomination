#include "backend/table.h"

#include "backend/gc.h"
#include "backend/value.h"

#include <assert.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define TABLE_MAX_LOAD_FACTOR 0.65
#define TABLE_INITIAL_GROWTH_CAPACITY 8
#define TABLE_CAPACITY_GROWTH_FACTOR 8

/// Get `table_ptr` next capacity.
/// @param table_ptr Pointer to initialized Table.
/// @result `table_ptr` next capacity.
#define TABLE_GET_NEXT_CAPACITY(table_ptr)                                                 \
  (((table_ptr)->capacity < TABLE_INITIAL_GROWTH_CAPACITY) ? TABLE_INITIAL_GROWTH_CAPACITY \
                                                           : (table_ptr)->capacity * TABLE_CAPACITY_GROWTH_FACTOR)

// *---------------------------------------------*
// *         INTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Initialize `entry`.
static inline void entry_init(TableEntry *const entry) {
  assert(entry != NULL);

  entry->key = NULL;
  entry->value = value_make_nil();
}

/// Kill `entry` by turning it into tombstone.
/// @note Tombstone is a special kind of entry signifying deletion.
static inline void entry_kill(TableEntry *const entry) {
  assert(entry != NULL);

  entry->key = NULL;
  entry->value = value_make_bool(false);
}

/// Probe `entries` array in search of entry associated with `key`.
/// @param entries TableEntry array to be probed.
/// @param capacity Capacity of `entries` array (must be positive).
/// @param key Key associated with entry to be searched for.
/// @return Pointer to either associated entry (if one exists), or empty entry (suitable for insertion).
static TableEntry *entries_probe(TableEntry *const entries, int const capacity, EntityString const *const key) {
  assert(entries != NULL);
  assert(capacity > 0);
  assert(key != NULL);

  TableEntry *tombstone = NULL;
  for (uint32_t index = key->hash % capacity;; index = (index + 1) % capacity) { // linear probing
    TableEntry *const entry = entries + index;

    if (entry->key == key) return entry; // associated entry
    if (entry->key != NULL) continue; // non-associated entry
    if (value_is_bool(entry->value)) { // tombstone entry
      tombstone = entry;
      continue;
    }

    // vacant entry
    return tombstone != NULL ? tombstone : entry;
  }
}

/// Grow `table` to next capacity.
/// @pre `table` is initialized.
static void table_grow(Table *const table) {
  assert(table != NULL);

  int const new_capacity = TABLE_GET_NEXT_CAPACITY(table);
  TableEntry *const new_entry_buffer = gc_allocate(sizeof(TableEntry) * new_capacity);
  for (int i = 0; i < new_capacity; i++) entry_init(new_entry_buffer + i);

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    TableEntry const *const source_entry = &table->entries[i];
    if (source_entry->key == NULL) continue;

    TableEntry *const destination_entry = entries_probe(new_entry_buffer, new_capacity, source_entry->key);
    *destination_entry = *source_entry;
    table->count++;
  }

  gc_deallocate(table->entries, sizeof(TableEntry) * table->capacity);
  table->entries = new_entry_buffer;
  table->capacity = new_capacity;
}

// *---------------------------------------------*
// *         EXTERNAL-LINKAGE FUNCTIONS          *
// *---------------------------------------------*

/// Initialize `table`.
void table_init(Table *const table) {
  assert(table != NULL);

  *table = (Table){0};
}

/// Release `table` resources and set it to uninitialized state.
/// @pre `table` is initialized.
void table_destroy(Table *const table) {
  assert(table != NULL);

  gc_deallocate(table->entries, sizeof(TableEntry) * table->capacity);

  *table = (Table){0};
}

/// Set value of `table` entry associated with `key` to `value`.
/// @pre `table` is initialized.
/// @return true if new entry got created, false otherwise (existing entry's value got overwritten).
bool table_set(Table *const table, EntityString *const key, Value const value) {
  assert(table != NULL);
  assert(key != NULL);

  // grow if needed
  if (table->count + 1 > (table->capacity * TABLE_MAX_LOAD_FACTOR)) table_grow(table);

  TableEntry *const entry = entries_probe(table->entries, table->capacity, key);
  bool const is_empty_entry = entry->key == NULL;
  if (is_empty_entry && value_is_nil(entry->value)) table->count++; // vacant entry

  entry->key = key;
  entry->value = value;

  return is_empty_entry;
}

/// Attempt to retrieve value corresponding to `key` from `table`; retrieved value gets stored in `out_value`.
/// @pre `table` is initialized.
/// @return true if value got retrieved, false otherwise (no entry is associated with `key`).
bool table_get(Table const *const table, EntityString const *const key, Value *const out_value) {
  assert(table != NULL);
  assert(key != NULL);
  assert(out_value != NULL);

  if (table->count == 0) return false;

  TableEntry const *const entry = entries_probe(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *out_value = entry->value;
  return true;
}

/// Attempt to delete entry associated with `key` from `table`.
/// @pre `table` is initialized.
/// @return true if entry got deleted, false otherwise (no entry is associated with `key`).
bool table_delete(Table *const table, EntityString const *const key) {
  assert(table != NULL);
  assert(key != NULL);

  if (table->count == 0) return false;

  TableEntry *const entry = entries_probe(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  entry_kill(entry);
  return true;
}
