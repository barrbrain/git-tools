#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <arpa/inet.h>

struct git_bloom {
	uint64_t bitfield[1 << (29 - 6)];
	uint32_t count[32];
	uint64_t size[32];
	uint32_t objects[32];
	uint64_t total[32];
};

static inline void git_bloom_set(struct git_bloom *bloom, uint32_t bit)
{
	bloom->bitfield[(bit >> 6) & ~(-1 << (29 - 6))] |= 1 << (bit & 63);
}

static inline int git_bloom_test(const struct git_bloom *bloom, uint32_t bit)
{
	return !!(bloom->bitfield[(bit >> 6) & ~(-1 << (29 - 6))] & (1 << (bit & 63)));
}

static inline void git_bloom_set_sha1(struct git_bloom *bloom, const char *sha1)
{
	const uint32_t *h = (const uint32_t*) sha1;
	git_bloom_set(bloom, h[0]);
	git_bloom_set(bloom, h[1]);
	git_bloom_set(bloom, h[2]);
	git_bloom_set(bloom, h[3]);
	git_bloom_set(bloom, h[4]);
}

static inline int git_bloom_test_sha1(const struct git_bloom *bloom, const char *sha1)
{
	const uint32_t *h = (const uint32_t*) sha1;
	return git_bloom_test(bloom, h[0])
	    && git_bloom_test(bloom, h[1])
	    && git_bloom_test(bloom, h[2])
	    && git_bloom_test(bloom, h[3])
	    && git_bloom_test(bloom, h[4]);
}

void git_uniq2(struct git_bloom *bloom, const char *sha1, const uint32_t *size, uint32_t n)
{
	while (--n) {
		const int il2 = *size ? 31 - __builtin_clz(*size) : 0;
		if (!git_bloom_test_sha1(bloom, sha1)) {
			git_bloom_set_sha1(bloom, sha1);
			bloom->count[il2]++;
			bloom->size[il2] += *size;
		}
		bloom->objects[il2]++;
		bloom->total[il2] += *size;
		sha1 += 20;
		size++;
	}
}

void git_uniq1(struct git_bloom *bloom, const char *sha1, const uint32_t *size, uint32_t n)
{
	while (--n) {
		const int il2 = *size ? 31 - __builtin_clz(*size) : 0;
		if (!git_bloom_test_sha1(bloom, sha1)) {
			git_bloom_set_sha1(bloom, sha1);
			bloom->count[il2]++;
			bloom->size[il2] += *size;
		}
		bloom->objects[il2]++;
		bloom->total[il2] += *size;
		sha1 += 24;
		size++;
	}
}

const char *open_idx2(FILE *f, uint32_t *objects) {
	uint32_t hdr[2 + 256];
	union { uint32_t i; char c[4]; } magic = {.c = {'\377', 't', 'O', 'c'}};
	rewind(f);
	if (258 != fread(hdr, 4, 258, f))
		return NULL;
	if (hdr[0] != magic.i || ntohl(hdr[1]) != 2)
		return NULL;

	*objects = ntohl(hdr[257]);

	return mmap(NULL, *objects * 28 + sizeof(hdr), PROT_READ, MAP_SHARED, fileno(f), 0);
}

const char *open_idx1(FILE *f, uint32_t *objects) {
	uint32_t hdr[256];
	union { uint32_t i; char c[4]; } magic = {.c = {'\377', 't', 'O', 'c'}};
	rewind(f);
	if (256 != fread(hdr, 4, 256, f))
		return NULL;
	if (hdr[0] == magic.i && ntohl(hdr[1]) == 2)
		return NULL;

	*objects = ntohl(hdr[255]);

	return mmap(NULL, *objects * 24 + sizeof(hdr), PROT_READ, MAP_SHARED, fileno(f), 0);
}

int cmp32(const void *a, const void *b) {
	return *(const uint32_t*)a - *(const uint32_t*)b;
}

int main(int argc, char **argv) {
	struct git_bloom *bloom = calloc(sizeof(struct git_bloom), 1);
	int i;
	char buf[4096];

	for(fgets(buf, sizeof(buf), stdin); !feof(stdin); fgets(buf, sizeof(buf), stdin)) {
		const char * map;
		uint32_t objects;
		FILE *f;
		if (strlen(buf) && buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		f = fopen(buf, "r");
		if (!f) continue;
		map = open_idx2(f, &objects);
		if (map && map != MAP_FAILED) {
			uint32_t *off = (uint32_t *)(map + 258 * 4 + objects * 24);
			uint32_t *sorted = malloc(4 * objects + 4);
			uint32_t *size = malloc(4 * objects);
			for (i = 0; i < objects; i++)
				sorted[i] = ntohl(off[i]);
			qsort(sorted, objects, 4, cmp32);
			sorted[objects] = sorted[objects - 1];
			for (i = 0; i < objects; i++) {
				uint32_t k = ntohl(off[i]);
				uint32_t *v = ((uint32_t*)bsearch(&k, sorted, objects, 4, cmp32));
				if (v)
					size[i] = v[1] - k;
				else size[i] = 0;
			}
			git_uniq2(bloom, map + 258 * 4, size, objects);
			free(size);
			free(sorted);
			munmap((void*)map, objects * 28 + 258 * 4);
		} else if (map) {
			perror(NULL);
		} else if (MAP_FAILED != (map = open_idx1(f, &objects))) {
			if (map) {
				uint32_t *off = (uint32_t *)(map + 256 * 4);
				uint32_t *sorted = malloc(4 * objects + 4);
				uint32_t *size = malloc(4 * objects);
				for (i = 0; i < objects; i++)
					sorted[i] = ntohl(off[i * 6]);
				qsort(sorted, objects, 4, cmp32);
				sorted[objects] = sorted[objects - 1];
				for (i = 0; i < objects; i++) {
					uint32_t k = ntohl(off[i * 6]);
					uint32_t *v = ((uint32_t*)bsearch(&k, sorted, objects, 4, cmp32));
					if (v)
						size[i] = v[1] - k;
					else size[i] = 0;
				}
				git_uniq1(bloom, map + 257 * 4, size, objects);
				free(size);
				free(sorted);
				munmap((void*)map, objects * 24 + 256 * 4);
			}
		} else {
			perror(NULL);
		}
		fclose(f);

		for (i = 0; i < 32; i++) {
			printf("Unique[%d]: %lld (%d)\n", i, bloom->size[i], bloom->count[i]);
			printf("Total[%d]: %lld (%d)\n", i, bloom->total[i], bloom->objects[i]);
		}
	}

	return 0;
}
