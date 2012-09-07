#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <arpa/inet.h>

struct git_bloom {
	uint64_t bitfield[1 << (32 - 6)];
	uint32_t count[32];
	uint64_t size[32];
	uint32_t objects[32];
	uint64_t total[32];
};

static inline void git_bloom_set(struct git_bloom *bloom, uint32_t bit)
{
	bloom->bitfield[bit >> 6] |= 1 << (bit & 63);
}

static inline int git_bloom_test(const struct git_bloom *bloom, uint32_t bit)
{
	return !!(bloom->bitfield[bit >> 6] & (1 << (bit & 63)));
}

#define x4(a) a a a a
#define x16(a) x4(x4(a))
#define x17(a) x16(a) a
static inline void git_bloom_set_sha1(struct git_bloom *bloom, const char *sha1)
{
	x17(git_bloom_set(bloom, *(uint32_t*)sha1++);)
}

static inline int git_bloom_test_sha1(const struct git_bloom *bloom, const char *sha1)
{
	return x17(git_bloom_test(bloom, *(uint32_t*)sha1++) &&) 1;
}

void git_uniq(struct git_bloom *bloom, const char *sha1, const uint32_t *size, uint32_t n)
{
	while (n--) {
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

char *open_idx(FILE *f, uint32_t *r_objects) {
	struct stat st;
	char *idx = NULL;
	void *map;
	uint32_t *hdr;
	int fd = fileno(f);

	fstat(fd, &st);
	if (st.st_size < 256 * 4)
		return NULL;

	map = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror(NULL);
		return NULL;
	}
	hdr = map;

	if (st.st_size >= 258 * 4 && !memcmp("\377tOc\0\0\0\2", map, 8)) {
		/* idx format v2 */
		uint32_t objects = ntohl(hdr[257]);
		if (st.st_size < 258 * 4 + objects * 28)
			goto done;
		*r_objects = objects;
		idx = malloc(objects * 24);

		hdr += 258;
		memcpy(idx, hdr, 20 * objects);
		memcpy(idx + 20 * objects, hdr + 6 * objects, 4 * objects);
	} else {
		/* idx format v1 */
		int i;
		uint32_t *sha, *off;

		uint32_t objects = ntohl(hdr[255]);
		if (st.st_size < 256 * 4 + objects * 24)
			goto done;
		*r_objects = objects;
		idx = malloc(objects * 24);
		sha = (uint32_t*)idx;
		off = sha + 5 * objects;

		hdr += 256;
		for (i = 0; i < objects; i++) {
			*sha++ = *hdr++;
			*sha++ = *hdr++;
			*sha++ = *hdr++;
			*sha++ = *hdr++;
			*sha++ = *hdr++;
			*off++ = *hdr++;
		}
	}

done:
	munmap(map, st.st_size);
	return idx;
}

int cmp32(const void *a, const void *b) {
	return *(const uint32_t*)a - *(const uint32_t*)b;
}

int main(int argc, char **argv) {
	struct git_bloom *bloom = calloc(sizeof(struct git_bloom), 1);
	int i;
	char buf[4096];

	for(fgets(buf, sizeof(buf), stdin); !feof(stdin); fgets(buf, sizeof(buf), stdin)) {
		char * map;
		uint32_t objects;
		FILE *f;
		if (strlen(buf) && buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		f = fopen(buf, "r");
		if (!f) continue;
		map = open_idx(f, &objects);
		fclose(f);
		if (map) {
			uint32_t *off = (uint32_t *)(map + objects * 20);
			uint32_t *size = off;

			uint32_t *sorted = malloc(4 * objects + 4);
			for (i = 0; i < objects; i++)
				sorted[i] = ntohl(off[i]);
			qsort(sorted, objects, 4, cmp32);
			sorted[objects] = sorted[objects - 1];
			for (i = 0; i < objects; i++) {
				uint32_t k = ntohl(off[i]);
				size[i] = 0;
				if (!(k & (1 << 31))) {
					uint32_t *v = ((uint32_t*)bsearch(&k, sorted, objects, 4, cmp32));
					if (v && !(v[1] & (1 << 31)))
						size[i] = v[1] - k;
				}
			}
			free(sorted);

			git_uniq(bloom, map, size, objects);

			free(map);
		}

		for (i = 0; i < 32; i++) {
			printf("Unique[%d]: %lld (%d)\n", i, bloom->size[i], bloom->count[i]);
			printf("Total[%d]: %lld (%d)\n", i, bloom->total[i], bloom->objects[i]);
		}
	}

	return 0;
}
