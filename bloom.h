#ifndef _BLOOM_H
#define _BLOOM_H

struct git_bloom {
	uint32_t bitfield[1 << (32 - 5)];
	uint32_t count[32];
	uint64_t size[32];
	uint32_t objects[32];
	uint64_t total[32];
};

int git_uniq(struct git_bloom *bloom, const char *sha1, const uint32_t *size, uint32_t n);

#endif
