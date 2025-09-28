#ifndef BUFFERS
#define BUFFERS
#include <signal.h>
#include <stdio.h>

#define buf_at(ptr, index) ptr[index < 0 || index > *((int *) ((char *) ptr - 4)) / sizeof(ptr[0]) ? raise(SIGABRT) : index]

typedef struct buffers {
	int count;
	void **buffers;
} buffers;

void *buf_alloc(buffers *list, int size);
void *buf_realloc(buffers *list, void *ptr, int size);
void buf_free(buffers *list);
void buf_memcopy(void *buf1, const void *buf2);
void buf_strcopy(void *buf1, const void *buf2);

#endif
