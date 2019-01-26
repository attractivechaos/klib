#ifndef KRNG_H
#define KRNG_H

typedef struct {
	uint64_t s[2];
} krng_t;

static inline uint64_t kr_splitmix64(uint64_t x)
{
	uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

static inline uint64_t kr_rand_r(krng_t *r)
{
	const uint64_t s0 = r->s[0];
	uint64_t s1 = r->s[1];
	const uint64_t result = s0 + s1;
	s1 ^= s0;
	r->s[0] = (s0 << 55 | s0 >> 9) ^ s1 ^ (s1 << 14);
	r->s[1] = s0 << 36 | s0 >> 28;
	return result;
}

static inline void kr_jump_r(krng_t *r)
{
	static const uint64_t JUMP[] = { 0xbeac0467eba5facbULL, 0xd86b048b86aa9922ULL };
	uint64_t s0 = 0, s1 = 0;
	int i, b;
	for (i = 0; i < 2; ++i)
		for (b = 0; b < 64; b++) {
			if (JUMP[i] & 1ULL << b)
				s0 ^= r->s[0], s1 ^= r->s[1];
			kr_rand_r(r);
		}
	r->s[0] = s0, r->s[1] = s1;
}

static inline void kr_srand_r(krng_t *r, uint64_t seed)
{
	r->s[0] = kr_splitmix64(seed);
	r->s[1] = kr_splitmix64(r->s[0]);
}

static inline double kr_drand_r(krng_t *r)
{
	union { uint64_t i; double d; } u;
	u.i = 0x3FFULL << 52 | kr_rand_r(r) >> 12;
	return u.d - 1.0;
}

#endif
