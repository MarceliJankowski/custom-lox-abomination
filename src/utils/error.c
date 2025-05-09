#include "utils/error.h"

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>

/**@desc log the most recent Windows API error*/
void error_windows_log_last(void) {
  DWORD const error_code = GetLastError();
  LPSTR error_message = NULL;

  // map error_code to error_message
  DWORD const error_message_length = FormatMessageA(
    (FORMAT_MESSAGE_ALLOCATE_BUFFER // allocate buffer for the message
     | FORMAT_MESSAGE_FROM_SYSTEM // map error_code to message using system's message table
     | FORMAT_MESSAGE_IGNORE_INSERTS // ignore (do not substitute) message insert sequences (%1, %2 etc.)
    ),
    NULL, error_code,
    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), // use US English (ensure language consistency across error messages)
    (LPSTR)&error_message, 0, NULL
  );

  // check if FormatMessage failed
  if (error_message_length == 0) {
    DWORD const format_message_error_code = GetLastError();
    fprintf(
      stderr, "FormatMessage failed for '%lu' error code ('%lu' error code)\n", (unsigned long)error_code,
      (unsigned long)format_message_error_code
    );
    return;
  } else if (error_message == NULL) {
    fprintf(stderr, "FormatMessage failed to allocate message buffer ('%lu' error code)\n", (unsigned long)error_code);
    return;
  }

  // log error_message
  fprintf(stderr, "%s", error_message);

  // free error_message
  if (LocalFree(error_message) != NULL) {
    DWORD const local_free_error_code = GetLastError();
    fprintf(stderr, "LocalFree failed with '%lu' error code\n", (unsigned long)local_free_error_code);
  }
}

#endif // _WIN32
