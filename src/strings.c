#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "buffers.h"

char *str_uppercase(buffers *list, const char *str){
	char *new = buf_alloc(list, strlen(str)+1);
	buf_strcopy(new, str);

	for(size_t i = 0; i < strlen(new); i++)
		buf_at(new, i) = toupper(buf_at(new, i));

	return new;
}

char *str_lowercase(buffers *list, const char *str){
	char *new = buf_alloc(list, strlen(str)+1);
	buf_strcopy(new, str);

	for(size_t i = 0; i < strlen(new); i++)
		buf_at(new, i) = tolower(buf_at(new, i));

	return new;
}

char *str_invertcase(buffers *list, const char *str){
	char *new = buf_alloc(list, strlen(str)+1);
	buf_strcopy(new, str);

	for(size_t i = 0; i < strlen(new); i++)
		buf_at(new, i) = isalpha(buf_at(str, i)) ? buf_at(str, i) ^ 32 : buf_at(str, i);

	return new;
}

char *str_sub(buffers *list, const char *str, const char *substr, const char *new){
	int bufsize = strlen(str) + strlen(new) + 1;
	char *final = buf_alloc(list, bufsize);

	int pos = strstr(str, substr)-str;

	for(int i = 0; i < pos; i++)
		buf_at(final, i) = buf_at(str, i);

	final = strncat(final, new, bufsize);

	const char *end = str + pos + strlen(substr);
	final = strncat(final, end, bufsize);

	return final;
}

char *str_gsub(buffers *list, const char *str, const char *substr, const char *new){
	char *state = str_sub(list, str, substr, new);
	char *new_state;

	if(strstr(state, substr) != NULL){
		new_state = str_gsub(list, state, substr, new);
		state = new_state;
	}

	return state;
}

char *str_format(buffers *list, const char *fmt, ...){
	va_list args;
	va_start(args, fmt);

	int bufsize = vsnprintf(NULL, 0, fmt, args) + 1;

	char *buf = buf_alloc(list, bufsize + 1);

	va_end(args);

	va_start(args, fmt);

	vsnprintf(buf, bufsize, fmt, args);

	va_end(args);

	return buf;
}

char *str_concat(buffers *list, const char *str1, const char *str2){
	int bufsize = strlen(str1) + strlen(str2) + 1;
	char *buf = buf_alloc(list, bufsize);

	buf_strcopy(buf, str1);

	strncat(buf, str2, bufsize);

	return buf;
}

int str_count(const char *str, const char *substr){
	const char *state = str;
	int count = 0;

	while(strstr(state, substr)){
		count++;
		state = strstr(state, substr) + strlen(substr);
	}

	return count;
}

char **str_split(buffers *list, const char *str, const char *separator){
	char *copy = buf_alloc(list, strlen(str)+1);
	buf_strcopy(copy, str);

	int count = str_count(copy, separator);
	char **strings = (char **) buf_alloc(list, (count + 1) * sizeof(char *));

	char *token = strtok(copy, separator);

	for(int i = 0; token != NULL; i++){
		buf_at(strings, i) = token;
		token = strtok(NULL, separator);
	}

	return strings;
}

int str_include(const char *str1, const char *str2){
	return strstr(str1, str2) != NULL;
}

int str_equal(const char *str1, const char *str2){
	return strcmp(str1, str2) == 0;
}
