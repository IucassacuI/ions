#ifndef STRINGS
#define STRINGS
#include "buffers.h"

char *str_uppercase(buffers *list, const char *str);
char *str_lowercase(buffers *list, const char *str);
char *str_invertcase(buffers *list, const char *str);
char *str_sub(buffers *list, const char *str, const char *substr, const char *new);
char *str_gsub(buffers *list, const char *str, const char *substr, const char *new);
char *str_format(buffers *list, const char *fmt, ...);
char *str_concat(buffers *list, const char *str1, const char *str2);
int  str_count(const char *str, const char *substr);
char **str_split(buffers *list, const char *str, const char *separator);
int  str_include(const char *str1, const char *str2);
int  str_equal(const char *str1, const char *str2);

#endif
