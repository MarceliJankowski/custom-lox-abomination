#include <stdio.h>

#include "test_common.h"
#include "util/error.h"

/**@desc open throwaway stream connected to OS dependent null file
@return pointer to writable throwaway stream*/
FILE *open_throwaway_stream(void) {
#ifdef _WIN32
  FILE *const throwaway_stream = fopen("NUL", "w");
#else
  FILE *const throwaway_stream = fopen("/dev/null", "w");
#endif

  if (throwaway_stream == NULL) IO_ERROR("%s", strerror(errno));

  return throwaway_stream;
}
