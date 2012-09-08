#include <stdint.h>

#include "bloom.h"

static inline void git_bloom_set(struct git_bloom *bloom, uint32_t bit)
{
	bloom->bitfield[bit >> 6] |= 1 << (bit & 63);
}

static inline int git_bloom_test(const struct git_bloom *bloom, uint32_t bit)
{
	return !!(bloom->bitfield[bit >> 6] & (1 << (bit & 63)));
}

#define x5(a) a a a a a
#define x11(a) x5(a) x5(a) a
static inline void git_bloom_set_sha1(struct git_bloom *bloom, const char *sha1)
{
	uint32_t h1 = *(uint32_t*)sha1, h2 = *(uint32_t*)&sha1[4], i = 0;
	x11(git_bloom_set(bloom, h1 + i++ * h2);)
}

static inline int git_bloom_test_sha1(const struct git_bloom *bloom, const char *sha1)
{
	uint32_t h1 = *(uint32_t*)sha1, h2 = *(uint32_t*)&sha1[4], i = 0;
	int r = 1;
	x11(r &= git_bloom_test(bloom, h1 + i++ * h2);)
	return r;
}

void git_uniq(struct git_bloom *bloom, const char *sha1, const uint32_t *size, uint32_t n)
{
	while (n--) {
		const uint32_t sz = *size++;
		const int il2 = sz ? 31 - __builtin_clz(sz) : 0;
		if (!git_bloom_test_sha1(bloom, sha1)) {
			git_bloom_set_sha1(bloom, sha1);
			bloom->count[il2]++;
			bloom->size[il2] += sz;
		}
		bloom->objects[il2]++;
		bloom->total[il2] += sz;
		sha1 += 20;
	}
}
