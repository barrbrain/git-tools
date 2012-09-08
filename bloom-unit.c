#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "bloom.h"
#include "lfsr.h"

int main(int argc, char **argv) {
	struct git_bloom *bloom = calloc(sizeof(struct git_bloom), 1);
	uint32_t *test = malloc(24 << 24);
	int i, j;

	for (j = 0; j < 1 << 3; j++) {
		for (i = 0; i < (6 << 24); i++)
			test[i] = lfsr_160_u32();
		git_uniq(bloom, (char*)test, (uint32_t*)(test) + (5 << 24), 1 << 24);
	}
	for (i = 0; i < 32; i++) {
		printf("Unique[%d]: %lld (%d)\n", i, bloom->size[i], bloom->count[i]);
		printf("Total[%d]: %lld (%d)\n", i, bloom->total[i], bloom->objects[i]);
	}

	return 0;
}
