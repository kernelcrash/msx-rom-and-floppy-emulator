

#include "../includes/types.h"
#include "../includes/heap.h"


void *malloc(uint16_t size) {
	uint8_t *ret = heap_top;
	heap_top += size;
	return (void *) ret;
}
