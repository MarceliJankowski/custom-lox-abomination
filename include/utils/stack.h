#ifndef STACK_H
#define STACK_H

#include "darray.h"

#include <assert.h>

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define STACK_TYPE(...) DARRAY_TYPE(__VA_ARGS__)
#define STACK_INIT(...) DARRAY_INIT(__VA_ARGS__)
#define STACK_DEFINE(...) DARRAY_DEFINE(__VA_ARGS__)
#define STACK_DESTROY(...) DARRAY_DESTROY(__VA_ARGS__)
#define STACK_GROW_CAPACITY(...) DARRAY_GROW_CAPACITY(__VA_ARGS__)
#define STACK_RESIZE(...) DARRAY_RESIZE(__VA_ARGS__)
#define STACK_PUSH(...) DARRAY_PUSH(__VA_ARGS__)
#define STACK_POP(...) DARRAY_POP(__VA_ARGS__)

#define STACK_TOP(stack_ptr) (stack_ptr)->data[(stack_ptr)->count - 1]

#endif // STACK_H
