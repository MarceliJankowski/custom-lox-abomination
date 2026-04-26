#ifndef TABLE_H
#define TABLE_H

#include "backend/entity.h"
#include "backend/value.h"

// *---------------------------------------------*
// *              TYPE DEFINITIONS               *
// *---------------------------------------------*

/// Table entry.
typedef struct {
  EntityString *key;
  Value value;
} TableEntry;

/// Hash table tailored for CLA values.
typedef struct {
  int count, capacity;
  TableEntry *entries;
} Table;

// *---------------------------------------------*
// *             FUNCTION PROTOTYPES             *
// *---------------------------------------------*

void table_init(Table *table);
void table_destroy(Table *table);
bool table_set(Table *table, EntityString *key, Value value);
bool table_get(Table const *table, EntityString const *key, Value *out_value);
bool table_delete(Table *table, EntityString const *key);
EntityString *table_probe_key(Table const *table, char const *content, int content_length, uint32_t hash);

#endif // TABLE_H
