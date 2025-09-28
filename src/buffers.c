#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "buffers.h"

void *buf_alloc(buffers *list, int size){
	if(list->buffers == NULL){
		list->buffers = calloc(1, sizeof(void *));
	}

	void *ptr = malloc(size + 4);
	memset(ptr, 0, size + 4);

	list->buffers[list->count] = ptr;
	list->count++;
	list->buffers = reallocarray(list->buffers, list->count + 1, sizeof(void *));

	*((int *) ptr) = size;

	return ptr + 4;
}

void *buf_realloc(buffers *list, void *ptr, int size){
	ptr -= 4;
	*((int *) ptr) = size;

	void *new = realloc(ptr, size);

	for(int i = 0; i <= list->count; i++){
		if(list->buffers[i] == ptr){
			list->buffers[i] = new;
		}
	}

	return new + 4;
}

void buf_free(buffers *list){
	for(int i = 0; i < list->count; i++){
		free(list->buffers[i]);
	}

	free(list->buffers);
}

void buf_memcopy(void *dest, const void *src){
	int limit = *((int *) (dest - 4));
	memcpy(dest, src, limit);
}

void buf_strcopy(void *dest, const void *src){
	int limit = *((int *) (dest - 4));
	strncpy(dest, src, limit);
}
