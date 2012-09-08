#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "bloom.h"

static inline void git_bloom_set(struct git_bloom *bloom, uint32_t bit)
{
	bloom->bitfield[bit >> 5] |= 1 << (bit & 31);
}

static inline int git_bloom_test(const struct git_bloom *bloom, uint32_t bit)
{
	return !!(bloom->bitfield[bit >> 5] & (1 << (bit & 31)));
}

#define x5(a) a a a a a
static inline void git_bloom_set_sha1(struct git_bloom *bloom, const char *sha1)
{
	const uint32_t *h = (const uint32_t*)sha1;
	x5(git_bloom_set(bloom, *h++);)
}

static inline int git_bloom_test_sha1(const struct git_bloom *bloom, const char *sha1)
{
	const uint32_t *h = (const uint32_t*)sha1;
	return x5(git_bloom_test(bloom, *h++) &&) 1;
}

int git_uniq(struct git_bloom *bloom, const char *sha1, const uint32_t *size, uint32_t n)
{
	int r = 0;
	while (n--) {
		const uint32_t sz = *size++;
		const int il2 = sz ? 31 - __builtin_clz(sz) : 0;
		if (!git_bloom_test_sha1(bloom, sha1)) {
			git_bloom_set_sha1(bloom, sha1);
			bloom->count[il2]++;
			bloom->size[il2] += sz;
		} else if (il2 >= 21) {
			const uint32_t *h = (const uint32_t *)sha1;
			printf("%08x%08x%08x%08x%08x\n", ntohl(h[0]), ntohl(h[1]), ntohl(h[2]), ntohl(h[3]), ntohl(h[4]));
			r = 1;
		}
		bloom->objects[il2]++;
		bloom->total[il2] += sz;
		sha1 += 20;
	}
	return r;
}
