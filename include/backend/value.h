#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

/**@desc clox value*/
typedef double Value;

/**@desc dynamic array used for storing clox values*/
typedef struct {
  Value *values;
  int32_t capacity, count;
} ValueArray;

void value_array_init(ValueArray *array);
void value_array_append(ValueArray *array, Value value);
void value_array_free(ValueArray *array);
void value_print(Value value);

#endif // VALUE_H
