#ifndef COMMON_H
#define COMMON_H

#define STRINGIZE_HELPER(token) #token
#define STRINGIZE(token) STRINGIZE_HELPER(token)

#define COMMA ,
#define M_S " - " // Message Separator
#define P_S ":" // Position Separator

#define FILE_FORMAT "%s"
#define LINE_FORMAT "%d"
#define COLUMN_FORMAT "%d"
#define LINE_COLUMN_FORMAT LINE_FORMAT P_S COLUMN_FORMAT
#define FILE_LINE_FORMAT FILE_FORMAT P_S LINE_FORMAT
#define FILE_LINE_COLUMN_FORMAT FILE_FORMAT P_S LINE_COLUMN_FORMAT

#endif // COMMON_H
