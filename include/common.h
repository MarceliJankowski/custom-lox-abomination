#ifndef COMMON_H
#define COMMON_H

// *---------------------------------------------*
// *              MACRO DEFINITIONS              *
// *---------------------------------------------*

#define COMMON__COMMON_STRINGIZE_HELPER(token) #token
#define COMMON_STRINGIZE(token) COMMON__COMMON_STRINGIZE_HELPER(token)

#define COMMON_COMMA ,
#define COMMON_MS " - " // Message Separator
#define COMMON_PS ":" // Position Separator

#define COMMON_FILE_FORMAT "%s"
#define COMMON_LINE_FORMAT "%d"
#define COMMON_COLUMN_FORMAT "%d"
#define COMMON_LINE_COLUMN_FORMAT COMMON_LINE_FORMAT COMMON_PS COMMON_COLUMN_FORMAT
#define COMMON_FILE_LINE_FORMAT COMMON_FILE_FORMAT COMMON_PS COMMON_LINE_FORMAT
#define COMMON_FILE_LINE_COLUMN_FORMAT COMMON_FILE_FORMAT COMMON_PS COMMON_LINE_COLUMN_FORMAT

#endif // COMMON_H
