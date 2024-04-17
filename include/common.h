#ifndef COMMON_H
#define COMMON_H

#define STRINGIZE_HELPER(token) #token
#define STRINGIZE(token) STRINGIZE_HELPER(token)

#define FILE_FORMAT "%s"
#define LINE_FORMAT "%ld"
#define COLUMN_FORMAT "%d"
#define LINE_COLUMN_FORMAT LINE_FORMAT ":" COLUMN_FORMAT
#define FILE_LINE_FORMAT FILE_FORMAT ":" LINE_FORMAT
#define FILE_LINE_COLUMN_FORMAT FILE_FORMAT ":" LINE_COLUMN_FORMAT

#define COMMA ,
#define M_S " - " // Message Separator

#endif // COMMON_H
