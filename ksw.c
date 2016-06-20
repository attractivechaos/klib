/* The MIT License

   Copyright (c) 2011 by Attractive Chaos <attractor@live.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#ifdef __PPC64__
#include "vec128int.h"
#else
#include <emmintrin.h>
#endif
#include "ksw.h"

#ifdef USE_MALLOC_WRAPPERS
#  include "malloc_wrap.h"
#endif

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect((x),1)
#define UNLIKELY(x) __builtin_expect((x),0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

const kswr_t g_defr = { 0, -1, -1, -1, -1, -1, -1 };

struct _kswq_t {
	int qlen, slen;
	uint8_t shift, mdiff, max, size;
	__m128i *qp, *H0, *H1, *E, *Hmax;
};

/**
 * Initialize the query data structure
 *
 * @param size   Number of bytes used to store a score; valid valures are 1 or 2
 * @param qlen   Length of the query sequence
 * @param query  Query sequence
 * @param m      Size of the alphabet
 * @param mat    Scoring matrix in a one-dimension array
 *
 * @return       Query data structure
 */
kswq_t *ksw_qinit(int size, int qlen, const uint8_t *query, int m, const int8_t *mat)
{
	kswq_t *q;
	int slen, a, tmp, p;

	size = size > 1? 2 : 1;
	p = 8 * (3 - size); // # values per __m128i
	slen = (qlen + p - 1) / p; // segmented length
	q = (kswq_t*)malloc(sizeof(kswq_t) + 256 + 16 * slen * (m + 4)); // a single block of memory
	q->qp = (__m128i*)(((size_t)q + sizeof(kswq_t) + 15) >> 4 << 4); // align memory
	q->H0 = q->qp + slen * m;
	q->H1 = q->H0 + slen;
	q->E  = q->H1 + slen;
	q->Hmax = q->E + slen;
	q->slen = slen; q->qlen = qlen; q->size = size;
	// compute shift
	tmp = m * m;
	for (a = 0, q->shift = 127, q->mdiff = 0; a < tmp; ++a) { // find the minimum and maximum score
		if (mat[a] < (int8_t)q->shift) q->shift = mat[a];
		if (mat[a] > (int8_t)q->mdiff) q->mdiff = mat[a];
	}
	q->max = q->mdiff;
	q->shift = 256 - q->shift; // NB: q->shift is uint8_t
	q->mdiff += q->shift; // this is the difference between the min and max scores
	// An example: p=8, qlen=19, slen=3 and segmentation:
	//  {{0,3,6,9,12,15,18,-1},{1,4,7,10,13,16,-1,-1},{2,5,8,11,14,17,-1,-1}}
	if (size == 1) {
		int8_t *t = (int8_t*)q->qp;
		for (a = 0; a < m; ++a) {
			int i, k, nlen = slen * p;
			const int8_t *ma = mat + a * m;
			for (i = 0; i < slen; ++i)
				for (k = i; k < nlen; k += slen) // p iterations
					*t++ = (k >= qlen? 0 : ma[query[k]]) + q->shift;
		}
	} else {
		int16_t *t = (int16_t*)q->qp;
		for (a = 0; a < m; ++a) {
			int i, k, nlen = slen * p;
			const int8_t *ma = mat + a * m;
			for (i = 0; i < slen; ++i)
				for (k = i; k < nlen; k += slen) // p iterations
					*t++ = (k >= qlen? 0 : ma[query[k]]);
		}
	}
	return q;
}

kswr_t ksw_u8(kswq_t *q, int tlen, const uint8_t *target, int _o_del, int _e_del, int _o_ins, int _e_ins, int xtra) // the first gap costs -(_o+_e)
{
	int slen, i, m_b, n_b, te = -1, gmax = 0, minsc, endsc;
	uint64_t *b;
	__m128i zero, oe_del, e_del, oe_ins, e_ins, shift, *H0, *H1, *E, *Hmax;
	kswr_t r;

#ifdef __PPC64__
#define __max_16(ret, xx) do { \
                (xx) = vec_max16ub((xx), vec_shiftrightbytes1q((xx), 8)); \
                (xx) = vec_max16ub((xx), vec_shiftrightbytes1q((xx), 4)); \
                (xx) = vec_max16ub((xx), vec_shiftrightbytes1q((xx), 2)); \
                (xx) = vec_max16ub((xx), vec_shiftrightbytes1q((xx), 1)); \
        (ret) = vec_extract8sh((xx), 0) & 0x00ff; \
        } while (0)
#else
#define __max_16(ret, xx) do { \
                (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 8)); \
                (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 4)); \
                (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 2)); \
                (xx) = _mm_max_epu8((xx), _mm_srli_si128((xx), 1)); \
        (ret) = _mm_extract_epi16((xx), 0) & 0x00ff; \
        } while (0)
#endif

	// initialization
	r = g_defr;
	minsc = (xtra&KSW_XSUBO)? xtra&0xffff : 0x10000;
	endsc = (xtra&KSW_XSTOP)? xtra&0xffff : 0x10000;
	m_b = n_b = 0; b = 0;
#ifdef __PPC64__
	zero = vec_splat4sw(0);
#else
	zero = _mm_set1_epi32(0);    /* !!!REP NOT FOUND!!! */ 
#endif
#ifdef __PPC64__
	oe_del = vec_splat16sb(_o_del + _e_del);
#else
	oe_del = _mm_set1_epi8(_o_del + _e_del);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	e_del = vec_splat16sb(_e_del);
#else
	e_del = _mm_set1_epi8(_e_del);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	oe_ins = vec_splat16sb(_o_ins + _e_ins);
#else
	oe_ins = _mm_set1_epi8(_o_ins + _e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	e_ins = vec_splat16sb(_e_ins);
#else
	e_ins = _mm_set1_epi8(_e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	shift = vec_splat16sb(q->shift);
#else
	shift = _mm_set1_epi8(q->shift);
   /* NEED INSPECTION */ 
#endif
	H0 = q->H0; H1 = q->H1; E = q->E; Hmax = q->Hmax;
	slen = q->slen;
	for (i = 0; i < slen; ++i) {
#ifdef __PPC64__
		vec_store1q(E + i, zero);
#else
		_mm_store_si128(E + i, zero);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
		vec_store1q(H0 + i, zero);
#else
		_mm_store_si128(H0 + i, zero);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
		vec_store1q(Hmax + i, zero);
#else
		_mm_store_si128(Hmax + i, zero);
   /* NEED INSPECTION */ 
#endif
	}
	// the core loop
	for (i = 0; i < tlen; ++i) {
		int j, k, cmp, imax;
		__m128i e, h, t, f = zero, max = zero, *S = q->qp + target[i] * slen; // s is the 1st score vector
#ifdef __PPC64__
		h = vec_load1q(H0 + slen - 1); // h={2,5,8,11,14,17,-1,-1} in the above example
#else
		h = _mm_load_si128(H0 + slen - 1); // h={2,5,8,11,14,17,-1,-1} in the above example
   /* NEED INSPECTION */ 
#endif
		#ifdef __BIG_ENDIAN__
#ifdef __PPC64__
			h = vec_shiftrightbytes1q(h, 1);	
#else
			h = _mm_srli_si128(h, 1);	
   /* NEED INSPECTION */ 
#endif
		#else
#ifdef __PPC64__
			h = vec_shiftleftbytes1q(h, 1); // h=H(i-1,-1); << instead of >> because x64 is little-endian
#else
			h = _mm_slli_si128(h, 1); // h=H(i-1,-1); << instead of >> because x64 is little-endian
   /* NEED INSPECTION */ 
#endif
		#endif
		for (j = 0; LIKELY(j < slen); ++j) {
			/* SW cells are computed in the following order:
			 *   H(i,j)   = max{H(i-1,j-1)+S(i,j), E(i,j), F(i,j)}
			 *   E(i+1,j) = max{H(i,j)-q, E(i,j)-r}
			 *   F(i,j+1) = max{H(i,j)-q, F(i,j)-r}
			 */
			// compute H'(i,j); note that at the beginning, h=H'(i-1,j-1)
#ifdef __PPC64__
			h = vec_addsaturating16ub(h, vec_load1q(S + j));
#else
			h = _mm_adds_epu8(h, _mm_load_si128(S + j));
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			h = vec_subtractsaturating16ub(h, shift); // h=H'(i-1,j-1)+S(i,j)
#else
			h = _mm_subs_epu8(h, shift); // h=H'(i-1,j-1)+S(i,j)
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			e = vec_load1q(E + j); // e=E'(i,j)
#else
			e = _mm_load_si128(E + j); // e=E'(i,j)
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			h = vec_max16ub(h, e);
#else
			h = _mm_max_epu8(h, e);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			h = vec_max16ub(h, f); // h=H'(i,j)
#else
			h = _mm_max_epu8(h, f); // h=H'(i,j)
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			max = vec_max16ub(max, h); // set max
#else
			max = _mm_max_epu8(max, h); // set max
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			vec_store1q(H1 + j, h); // save to H'(i,j)
#else
			_mm_store_si128(H1 + j, h); // save to H'(i,j)
   /* NEED INSPECTION */ 
#endif
			// now compute E'(i+1,j)
#ifdef __PPC64__
			e = vec_subtractsaturating16ub(e, e_del); // e=E'(i,j) - e_del
#else
			e = _mm_subs_epu8(e, e_del); // e=E'(i,j) - e_del
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			t = vec_subtractsaturating16ub(h, oe_del); // h=H'(i,j) - o_del - e_del
#else
			t = _mm_subs_epu8(h, oe_del); // h=H'(i,j) - o_del - e_del
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			e = vec_max16ub(e, t); // e=E'(i+1,j)
#else
			e = _mm_max_epu8(e, t); // e=E'(i+1,j)
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			vec_store1q(E + j, e); // save to E'(i+1,j)
#else
			_mm_store_si128(E + j, e); // save to E'(i+1,j)
   /* NEED INSPECTION */ 
#endif
			// now compute F'(i,j+1)
#ifdef __PPC64__
			f = vec_subtractsaturating16ub(f, e_ins);
#else
			f = _mm_subs_epu8(f, e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			t = vec_subtractsaturating16ub(h, oe_ins); // h=H'(i,j) - o_ins - e_ins
#else
			t = _mm_subs_epu8(h, oe_ins); // h=H'(i,j) - o_ins - e_ins
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			f = vec_max16ub(f, t);
#else
			f = _mm_max_epu8(f, t);
   /* NEED INSPECTION */ 
#endif
			// get H'(i-1,j) and prepare for the next j
#ifdef __PPC64__
			h = vec_load1q(H0 + j); // h=H'(i-1,j)
#else
			h = _mm_load_si128(H0 + j); // h=H'(i-1,j)
   /* NEED INSPECTION */ 
#endif
		}
		// NB: we do not need to set E(i,j) as we disallow adjecent insertion and then deletion
		for (k = 0; LIKELY(k < 16); ++k) { // this block mimics SWPS3; NB: H(i,j) updated in the lazy-F loop cannot exceed max
			#ifdef __BIG_ENDIAN__
#ifdef __PPC64__
				f = vec_shiftrightbytes1q(f, 1);
#else
				f = _mm_srli_si128(f, 1);
   /* NEED INSPECTION */ 
#endif
			#else
#ifdef __PPC64__
				f = vec_shiftleftbytes1q(f, 1);
#else
				f = _mm_slli_si128(f, 1);
   /* NEED INSPECTION */ 
#endif
			#endif
			for (j = 0; LIKELY(j < slen); ++j) {
#ifdef __PPC64__
				h = vec_load1q(H1 + j);
#else
				h = _mm_load_si128(H1 + j);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				h = vec_max16ub(h, f); // h=H'(i,j)
#else
				h = _mm_max_epu8(h, f); // h=H'(i,j)
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				vec_store1q(H1 + j, h);
#else
				_mm_store_si128(H1 + j, h);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				h = vec_subtractsaturating16ub(h, oe_ins);
#else
				h = _mm_subs_epu8(h, oe_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				f = vec_subtractsaturating16ub(f, e_ins);
#else
				f = _mm_subs_epu8(f, e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				cmp = vec_extractupperbit16sb(vec_compareeq16sb(vec_subtractsaturating16ub(f, h), zero));
#else
				cmp = _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_subs_epu8(f, h), zero));
   /* NEED INSPECTION */ 
#endif
				if (UNLIKELY(cmp == 0xffff)) goto end_loop16;
			}
		}
end_loop16:
		//int k;for (k=0;k<16;++k)printf("%d ", ((uint8_t*)&max)[k]);printf("\n");
		__max_16(imax, max); // imax is the maximum number in max
		if (imax >= minsc) { // write the b array; this condition adds branching unfornately
			if (n_b == 0 || (int32_t)b[n_b-1] + 1 != i) { // then append
				if (n_b == m_b) {
					m_b = m_b? m_b<<1 : 8;
					b = (uint64_t*)realloc(b, 8 * m_b);
				}
				b[n_b++] = (uint64_t)imax<<32 | i;
			} else if ((int)(b[n_b-1]>>32) < imax) b[n_b-1] = (uint64_t)imax<<32 | i; // modify the last
		}
		if (imax > gmax) {
			gmax = imax; te = i; // te is the end position on the target
			for (j = 0; LIKELY(j < slen); ++j) // keep the H1 vector
#ifdef __PPC64__
				vec_store1q(Hmax + j, vec_load1q(H1 + j));
#else
				_mm_store_si128(Hmax + j, _mm_load_si128(H1 + j));
   /* NEED INSPECTION */ 
#endif
			if (gmax + q->shift >= 255 || gmax >= endsc) break;
		}
		S = H1; H1 = H0; H0 = S; // swap H0 and H1
	}
	r.score = gmax + q->shift < 255? gmax : 255;
	r.te = te;
	if (r.score != 255) { // get a->qe, the end of query match; find the 2nd best score
		int max = -1, tmp, low, high, qlen = slen * 16;
		uint8_t *t = (uint8_t*)Hmax;
		for (i = 0; i < qlen; ++i, ++t)
			if ((int)*t > max) max = *t, r.qe = i / 16 + i % 16 * slen;
			else if ((int)*t == max && (tmp = i / 16 + i % 16 * slen) < r.qe) r.qe = tmp; 
		//printf("%d,%d\n", max, gmax);
		if (b) {
			i = (r.score + q->max - 1) / q->max;
			low = te - i; high = te + i;
			for (i = 0; i < n_b; ++i) {
				int e = (int32_t)b[i];
				if ((e < low || e > high) && (int)(b[i]>>32) > r.score2)
					r.score2 = b[i]>>32, r.te2 = e;
			}
		}
	}
	free(b);
	return r;
}

kswr_t ksw_i16(kswq_t *q, int tlen, const uint8_t *target, int _o_del, int _e_del, int _o_ins, int _e_ins, int xtra) // the first gap costs -(_o+_e)
{
	int slen, i, m_b, n_b, te = -1, gmax = 0, minsc, endsc;
	uint64_t *b;
	__m128i zero, oe_del, e_del, oe_ins, e_ins, *H0, *H1, *E, *Hmax;
	kswr_t r;

#ifdef __PPC64__
#define __max_8(ret, xx) do { \
                (xx) = vec_max8sh((xx), vec_shiftrightbytes1q((xx), 8)); \
                (xx) = vec_max8sh((xx), vec_shiftrightbytes1q((xx), 4)); \
                (xx) = vec_max8sh((xx), vec_shiftrightbytes1q((xx), 2)); \
        (ret) = vec_extract8sh((xx), 0); \
        } while (0)
#else
#define __max_8(ret, xx) do { \
                (xx) = _mm_max_epi16((xx), _mm_srli_si128((xx), 8)); \
                (xx) = _mm_max_epi16((xx), _mm_srli_si128((xx), 4)); \
                (xx) = _mm_max_epi16((xx), _mm_srli_si128((xx), 2)); \
        (ret) = _mm_extract_epi16((xx), 0); \
        } while (0)
#endif

	// initialization
	r = g_defr;
	minsc = (xtra&KSW_XSUBO)? xtra&0xffff : 0x10000;
	endsc = (xtra&KSW_XSTOP)? xtra&0xffff : 0x10000;
	m_b = n_b = 0; b = 0;
#ifdef __PPC64__
	zero = vec_splat4sw(0);
#else
	zero = _mm_set1_epi32(0);    /* !!!REP NOT FOUND!!! */ 
#endif
#ifdef __PPC64__
	oe_del = vec_splat8sh(_o_del + _e_del);
#else
	oe_del = _mm_set1_epi16(_o_del + _e_del);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	e_del = vec_splat8sh(_e_del);
#else
	e_del = _mm_set1_epi16(_e_del);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	oe_ins = vec_splat8sh(_o_ins + _e_ins);
#else
	oe_ins = _mm_set1_epi16(_o_ins + _e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
	e_ins = vec_splat8sh(_e_ins);
#else
	e_ins = _mm_set1_epi16(_e_ins);
   /* NEED INSPECTION */ 
#endif
	H0 = q->H0; H1 = q->H1; E = q->E; Hmax = q->Hmax;
	slen = q->slen;
	for (i = 0; i < slen; ++i) {
#ifdef __PPC64__
		vec_store1q(E + i, zero);
#else
		_mm_store_si128(E + i, zero);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
		vec_store1q(H0 + i, zero);
#else
		_mm_store_si128(H0 + i, zero);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
		vec_store1q(Hmax + i, zero);
#else
		_mm_store_si128(Hmax + i, zero);
   /* NEED INSPECTION */ 
#endif
	}
	// the core loop
	for (i = 0; i < tlen; ++i) {
		int j, k, imax;
		__m128i e, t, h, f = zero, max = zero, *S = q->qp + target[i] * slen; // s is the 1st score vector
#ifdef __PPC64__
		h = vec_load1q(H0 + slen - 1); // h={2,5,8,11,14,17,-1,-1} in the above example
#else
		h = _mm_load_si128(H0 + slen - 1); // h={2,5,8,11,14,17,-1,-1} in the above example
   /* NEED INSPECTION */ 
#endif
		#ifdef __BIG_ENDIAN__
#ifdef __PPC64__
			h = vec_shiftrightbytes1q(h, 2);
#else
			h = _mm_srli_si128(h, 2);
   /* NEED INSPECTION */ 
#endif
		#else
#ifdef __PPC64__
			h = vec_shiftleftbytes1q(h, 2);
#else
			h = _mm_slli_si128(h, 2);
   /* NEED INSPECTION */ 
#endif
		#endif
		for (j = 0; LIKELY(j < slen); ++j) {
#ifdef __PPC64__
			h = vec_addsaturating8sh(h, *S++);
#else
			h = _mm_adds_epi16(h, *S++);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			e = vec_load1q(E + j);
#else
			e = _mm_load_si128(E + j);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			h = vec_max8sh(h, e);
#else
			h = _mm_max_epi16(h, e);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			h = vec_max8sh(h, f);
#else
			h = _mm_max_epi16(h, f);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			max = vec_max8sh(max, h);
#else
			max = _mm_max_epi16(max, h);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			vec_store1q(H1 + j, h);
#else
			_mm_store_si128(H1 + j, h);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			e = vec_subtractsaturating8uh(e, e_del);
#else
			e = _mm_subs_epu16(e, e_del);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			t = vec_subtractsaturating8uh(h, oe_del);
#else
			t = _mm_subs_epu16(h, oe_del);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			e = vec_max8sh(e, t);
#else
			e = _mm_max_epi16(e, t);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			vec_store1q(E + j, e);
#else
			_mm_store_si128(E + j, e);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			f = vec_subtractsaturating8uh(f, e_ins);
#else
			f = _mm_subs_epu16(f, e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			t = vec_subtractsaturating8uh(h, oe_ins);
#else
			t = _mm_subs_epu16(h, oe_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			f = vec_max8sh(f, t);
#else
			f = _mm_max_epi16(f, t);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
			h = vec_load1q(H0 + j);
#else
			h = _mm_load_si128(H0 + j);
   /* NEED INSPECTION */ 
#endif
		}
		for (k = 0; LIKELY(k < 16); ++k) {
			#ifdef __BIG_ENDIAN__
#ifdef __PPC64__
				f = vec_shiftrightbytes1q(f, 2);
#else
				f = _mm_srli_si128(f, 2);
   /* NEED INSPECTION */ 
#endif
			#else
#ifdef __PPC64__
				f = vec_shiftleftbytes1q(f, 2);
#else
				f = _mm_slli_si128(f, 2);
   /* NEED INSPECTION */ 
#endif
			#endif
			for (j = 0; LIKELY(j < slen); ++j) {
#ifdef __PPC64__
				h = vec_load1q(H1 + j);
#else
				h = _mm_load_si128(H1 + j);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				h = vec_max8sh(h, f);
#else
				h = _mm_max_epi16(h, f);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				vec_store1q(H1 + j, h);
#else
				_mm_store_si128(H1 + j, h);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				h = vec_subtractsaturating8uh(h, oe_ins);
#else
				h = _mm_subs_epu16(h, oe_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				f = vec_subtractsaturating8uh(f, e_ins);
#else
				f = _mm_subs_epu16(f, e_ins);
   /* NEED INSPECTION */ 
#endif
#ifdef __PPC64__
				if(UNLIKELY(!vec_extractupperbit16sb(vec_comparegt8sh(f, h)))) goto end_loop8;
#else
				if(UNLIKELY(!_mm_movemask_epi8(_mm_cmpgt_epi16(f, h)))) goto end_loop8;
   /* NEED INSPECTION */ 
#endif
			}
		}
end_loop8:
		__max_8(imax, max);
		if (imax >= minsc) {
			if (n_b == 0 || (int32_t)b[n_b-1] + 1 != i) {
				if (n_b == m_b) {
					m_b = m_b? m_b<<1 : 8;
					b = (uint64_t*)realloc(b, 8 * m_b);
				}
				b[n_b++] = (uint64_t)imax<<32 | i;
			} else if ((int)(b[n_b-1]>>32) < imax) b[n_b-1] = (uint64_t)imax<<32 | i; // modify the last
		}
		if (imax > gmax) {
			gmax = imax; te = i;
			for (j = 0; LIKELY(j < slen); ++j)
#ifdef __PPC64__
				vec_store1q(Hmax + j, vec_load1q(H1 + j));
#else
				_mm_store_si128(Hmax + j, _mm_load_si128(H1 + j));
   /* NEED INSPECTION */ 
#endif
			if (gmax >= endsc) break;
		}
		S = H1; H1 = H0; H0 = S;
	}
	r.score = gmax; r.te = te;
	{
		int max = -1, tmp, low, high, qlen = slen * 8;
		uint16_t *t = (uint16_t*)Hmax;
		for (i = 0, r.qe = -1; i < qlen; ++i, ++t)
			if ((int)*t > max) max = *t, r.qe = i / 8 + i % 8 * slen;
			else if ((int)*t == max && (tmp = i / 8 + i % 8 * slen) < r.qe) r.qe = tmp; 
		if (b) {
			i = (r.score + q->max - 1) / q->max;
			low = te - i; high = te + i;
			for (i = 0; i < n_b; ++i) {
				int e = (int32_t)b[i];
				if ((e < low || e > high) && (int)(b[i]>>32) > r.score2)
					r.score2 = b[i]>>32, r.te2 = e;
			}
		}
	}
	free(b);
	return r;
}

static inline void revseq(int l, uint8_t *s)
{
	int i, t;
	for (i = 0; i < l>>1; ++i)
		t = s[i], s[i] = s[l - 1 - i], s[l - 1 - i] = t;
}

kswr_t ksw_align2(int qlen, uint8_t *query, int tlen, uint8_t *target, int m, const int8_t *mat, int o_del, int e_del, int o_ins, int e_ins, int xtra, kswq_t **qry)
{
	int size;
	kswq_t *q;
	kswr_t r, rr;
	kswr_t (*func)(kswq_t*, int, const uint8_t*, int, int, int, int, int);

	q = (qry && *qry)? *qry : ksw_qinit((xtra&KSW_XBYTE)? 1 : 2, qlen, query, m, mat);
	if (qry && *qry == 0) *qry = q;
	func = q->size == 2? ksw_i16 : ksw_u8;
	size = q->size;
	r = func(q, tlen, target, o_del, e_del, o_ins, e_ins, xtra);
	if (qry == 0) free(q);
	if ((xtra&KSW_XSTART) == 0 || ((xtra&KSW_XSUBO) && r.score < (xtra&0xffff))) return r;
	revseq(r.qe + 1, query); revseq(r.te + 1, target); // +1 because qe/te points to the exact end, not the position after the end
	q = ksw_qinit(size, r.qe + 1, query, m, mat);
	rr = func(q, tlen, target, o_del, e_del, o_ins, e_ins, KSW_XSTOP | r.score);
	revseq(r.qe + 1, query); revseq(r.te + 1, target);
	free(q);
	if (r.score == rr.score)
		r.tb = r.te - rr.te, r.qb = r.qe - rr.qe;
	return r;
}

kswr_t ksw_align(int qlen, uint8_t *query, int tlen, uint8_t *target, int m, const int8_t *mat, int gapo, int gape, int xtra, kswq_t **qry)
{
	return ksw_align2(qlen, query, tlen, target, m, mat, gapo, gape, gapo, gape, xtra, qry);
}

/********************
 *** SW extension ***
 ********************/

typedef struct {
	int32_t h, e;
} eh_t;

int ksw_extend2(int qlen, const uint8_t *query, int tlen, const uint8_t *target, int m, const int8_t *mat, int o_del, int e_del, int o_ins, int e_ins, int w, int end_bonus, int zdrop, int h0, int *_qle, int *_tle, int *_gtle, int *_gscore, int *_max_off)
{
	eh_t *eh; // score array
	int8_t *qp; // query profile
	int i, j, k, oe_del = o_del + e_del, oe_ins = o_ins + e_ins, beg, end, max, max_i, max_j, max_ins, max_del, max_ie, gscore, max_off;
	assert(h0 > 0);
	// allocate memory
	qp = malloc(qlen * m);
	eh = calloc(qlen + 1, 8);
	// generate the query profile
	for (k = i = 0; k < m; ++k) {
		const int8_t *p = &mat[k * m];
		for (j = 0; j < qlen; ++j) qp[i++] = p[query[j]];
	}
	// fill the first row
	eh[0].h = h0; eh[1].h = h0 > oe_ins? h0 - oe_ins : 0;
	for (j = 2; j <= qlen && eh[j-1].h > e_ins; ++j)
		eh[j].h = eh[j-1].h - e_ins;
	// adjust $w if it is too large
	k = m * m;
	for (i = 0, max = 0; i < k; ++i) // get the max score
		max = max > mat[i]? max : mat[i];
	max_ins = (int)((double)(qlen * max + end_bonus - o_ins) / e_ins + 1.);
	max_ins = max_ins > 1? max_ins : 1;
	w = w < max_ins? w : max_ins;
	max_del = (int)((double)(qlen * max + end_bonus - o_del) / e_del + 1.);
	max_del = max_del > 1? max_del : 1;
	w = w < max_del? w : max_del; // TODO: is this necessary?
	// DP loop
	max = h0, max_i = max_j = -1; max_ie = -1, gscore = -1;
	max_off = 0;
	beg = 0, end = qlen;
	for (i = 0; LIKELY(i < tlen); ++i) {
		int t, f = 0, h1, m = 0, mj = -1;
		int8_t *q = &qp[target[i] * qlen];
		// apply the band and the constraint (if provided)
		if (beg < i - w) beg = i - w;
		if (end > i + w + 1) end = i + w + 1;
		if (end > qlen) end = qlen;
		// compute the first column
		if (beg == 0) {
			h1 = h0 - (o_del + e_del * (i + 1));
			if (h1 < 0) h1 = 0;
		} else h1 = 0;
		for (j = beg; LIKELY(j < end); ++j) {
			// At the beginning of the loop: eh[j] = { H(i-1,j-1), E(i,j) }, f = F(i,j) and h1 = H(i,j-1)
			// Similar to SSE2-SW, cells are computed in the following order:
			//   H(i,j)   = max{H(i-1,j-1)+S(i,j), E(i,j), F(i,j)}
			//   E(i+1,j) = max{H(i,j)-gapo, E(i,j)} - gape
			//   F(i,j+1) = max{H(i,j)-gapo, F(i,j)} - gape
			eh_t *p = &eh[j];
			int h, M = p->h, e = p->e; // get H(i-1,j-1) and E(i-1,j)
			p->h = h1;          // set H(i,j-1) for the next row
			M = M? M + q[j] : 0;// separating H and M to disallow a cigar like "100M3I3D20M"
			h = M > e? M : e;   // e and f are guaranteed to be non-negative, so h>=0 even if M<0
			h = h > f? h : f;
			h1 = h;             // save H(i,j) to h1 for the next column
			mj = m > h? mj : j; // record the position where max score is achieved
			m = m > h? m : h;   // m is stored at eh[mj+1]
			t = M - oe_del;
			t = t > 0? t : 0;
			e -= e_del;
			e = e > t? e : t;   // computed E(i+1,j)
			p->e = e;           // save E(i+1,j) for the next row
			t = M - oe_ins;
			t = t > 0? t : 0;
			f -= e_ins;
			f = f > t? f : t;   // computed F(i,j+1)
		}
		eh[end].h = h1; eh[end].e = 0;
		if (j == qlen) {
			max_ie = gscore > h1? max_ie : i;
			gscore = gscore > h1? gscore : h1;
		}
		if (m == 0) break;
		if (m > max) {
			max = m, max_i = i, max_j = mj;
			max_off = max_off > abs(mj - i)? max_off : abs(mj - i);
		} else if (zdrop > 0) {
			if (i - max_i > mj - max_j) {
				if (max - m - ((i - max_i) - (mj - max_j)) * e_del > zdrop) break;
			} else {
				if (max - m - ((mj - max_j) - (i - max_i)) * e_ins > zdrop) break;
			}
		}
		// update beg and end for the next round
		for (j = beg; LIKELY(j < end) && eh[j].h == 0 && eh[j].e == 0; ++j);
		beg = j;
		for (j = end; LIKELY(j >= beg) && eh[j].h == 0 && eh[j].e == 0; --j);
		end = j + 2 < qlen? j + 2 : qlen;
		//beg = 0; end = qlen; // uncomment this line for debugging
	}
	free(eh); free(qp);
	if (_qle) *_qle = max_j + 1;
	if (_tle) *_tle = max_i + 1;
	if (_gtle) *_gtle = max_ie + 1;
	if (_gscore) *_gscore = gscore;
	if (_max_off) *_max_off = max_off;
	return max;
}

int ksw_extend(int qlen, const uint8_t *query, int tlen, const uint8_t *target, int m, const int8_t *mat, int gapo, int gape, int w, int end_bonus, int zdrop, int h0, int *qle, int *tle, int *gtle, int *gscore, int *max_off)
{
	return ksw_extend2(qlen, query, tlen, target, m, mat, gapo, gape, gapo, gape, w, end_bonus, zdrop, h0, qle, tle, gtle, gscore, max_off);
}

/********************
 * Global alignment *
 ********************/

#define MINUS_INF -0x40000000

static inline uint32_t *push_cigar(int *n_cigar, int *m_cigar, uint32_t *cigar, int op, int len)
{
	if (*n_cigar == 0 || op != (cigar[(*n_cigar) - 1]&0xf)) {
		if (*n_cigar == *m_cigar) {
			*m_cigar = *m_cigar? (*m_cigar)<<1 : 4;
			cigar = realloc(cigar, (*m_cigar) << 2);
		}
		cigar[(*n_cigar)++] = len<<4 | op;
	} else cigar[(*n_cigar)-1] += len<<4;
	return cigar;
}

int ksw_global2(int qlen, const uint8_t *query, int tlen, const uint8_t *target, int m, const int8_t *mat, int o_del, int e_del, int o_ins, int e_ins, int w, int *n_cigar_, uint32_t **cigar_)
{
	eh_t *eh;
	int8_t *qp; // query profile
	int i, j, k, oe_del = o_del + e_del, oe_ins = o_ins + e_ins, score, n_col;
	uint8_t *z; // backtrack matrix; in each cell: f<<4|e<<2|h; in principle, we can halve the memory, but backtrack will be a little more complex
	if (n_cigar_) *n_cigar_ = 0;
	// allocate memory
	n_col = qlen < 2*w+1? qlen : 2*w+1; // maximum #columns of the backtrack matrix
	z = n_cigar_ && cigar_? malloc((long)n_col * tlen) : 0;
	qp = malloc(qlen * m);
	eh = calloc(qlen + 1, 8);
	// generate the query profile
	for (k = i = 0; k < m; ++k) {
		const int8_t *p = &mat[k * m];
		for (j = 0; j < qlen; ++j) qp[i++] = p[query[j]];
	}
	// fill the first row
	eh[0].h = 0; eh[0].e = MINUS_INF;
	for (j = 1; j <= qlen && j <= w; ++j)
		eh[j].h = -(o_ins + e_ins * j), eh[j].e = MINUS_INF;
	for (; j <= qlen; ++j) eh[j].h = eh[j].e = MINUS_INF; // everything is -inf outside the band
	// DP loop
	for (i = 0; LIKELY(i < tlen); ++i) { // target sequence is in the outer loop
		int32_t f = MINUS_INF, h1, beg, end, t;
		int8_t *q = &qp[target[i] * qlen];
		beg = i > w? i - w : 0;
		end = i + w + 1 < qlen? i + w + 1 : qlen; // only loop through [beg,end) of the query sequence
		h1 = beg == 0? -(o_del + e_del * (i + 1)) : MINUS_INF;
		if (n_cigar_ && cigar_) {
			uint8_t *zi = &z[(long)i * n_col];
			for (j = beg; LIKELY(j < end); ++j) {
				// At the beginning of the loop: eh[j] = { H(i-1,j-1), E(i,j) }, f = F(i,j) and h1 = H(i,j-1)
				// Cells are computed in the following order:
				//   M(i,j)   = H(i-1,j-1) + S(i,j)
				//   H(i,j)   = max{M(i,j), E(i,j), F(i,j)}
				//   E(i+1,j) = max{M(i,j)-gapo, E(i,j)} - gape
				//   F(i,j+1) = max{M(i,j)-gapo, F(i,j)} - gape
				// We have to separate M(i,j); otherwise the direction may not be recorded correctly.
				// However, a CIGAR like "10M3I3D10M" allowed by local() is disallowed by global().
				// Such a CIGAR may occur, in theory, if mismatch_penalty > 2*gap_ext_penalty + 2*gap_open_penalty/k.
				// In practice, this should happen very rarely given a reasonable scoring system.
				eh_t *p = &eh[j];
				int32_t h, m = p->h, e = p->e;
				uint8_t d; // direction
				p->h = h1;
				m += q[j];
				d = m >= e? 0 : 1;
				h = m >= e? m : e;
				d = h >= f? d : 2;
				h = h >= f? h : f;
				h1 = h;
				t = m - oe_del;
				e -= e_del;
				d |= e > t? 1<<2 : 0;
				e  = e > t? e    : t;
				p->e = e;
				t = m - oe_ins;
				f -= e_ins;
				d |= f > t? 2<<4 : 0; // if we want to halve the memory, use one bit only, instead of two
				f  = f > t? f    : t;
				zi[j - beg] = d; // z[i,j] keeps h for the current cell and e/f for the next cell
			}
		} else {
			for (j = beg; LIKELY(j < end); ++j) {
				eh_t *p = &eh[j];
				int32_t h, m = p->h, e = p->e;
				p->h = h1;
				m += q[j];
				h = m >= e? m : e;
				h = h >= f? h : f;
				h1 = h;
				t = m - oe_del;
				e -= e_del;
				e  = e > t? e : t;
				p->e = e;
				t = m - oe_ins;
				f -= e_ins;
				f  = f > t? f : t;
			}
		}
		eh[end].h = h1; eh[end].e = MINUS_INF;
	}
	score = eh[qlen].h;
	if (n_cigar_ && cigar_) { // backtrack
		int n_cigar = 0, m_cigar = 0, which = 0;
		uint32_t *cigar = 0, tmp;
		i = tlen - 1; k = (i + w + 1 < qlen? i + w + 1 : qlen) - 1; // (i,k) points to the last cell
		while (i >= 0 && k >= 0) {
			which = z[(long)i * n_col + (k - (i > w? i - w : 0))] >> (which<<1) & 3;
			if (which == 0)      cigar = push_cigar(&n_cigar, &m_cigar, cigar, 0, 1), --i, --k;
			else if (which == 1) cigar = push_cigar(&n_cigar, &m_cigar, cigar, 2, 1), --i;
			else                 cigar = push_cigar(&n_cigar, &m_cigar, cigar, 1, 1), --k;
		}
		if (i >= 0) cigar = push_cigar(&n_cigar, &m_cigar, cigar, 2, i + 1);
		if (k >= 0) cigar = push_cigar(&n_cigar, &m_cigar, cigar, 1, k + 1);
		for (i = 0; i < n_cigar>>1; ++i) // reverse CIGAR
			tmp = cigar[i], cigar[i] = cigar[n_cigar-1-i], cigar[n_cigar-1-i] = tmp;
		*n_cigar_ = n_cigar, *cigar_ = cigar;
	}
	free(eh); free(qp); free(z);
	return score;
}

int ksw_global(int qlen, const uint8_t *query, int tlen, const uint8_t *target, int m, const int8_t *mat, int gapo, int gape, int w, int *n_cigar_, uint32_t **cigar_)
{
	return ksw_global2(qlen, query, tlen, target, m, mat, gapo, gape, gapo, gape, w, n_cigar_, cigar_);
}

/*******************************************
 * Main function (not compiled by default) *
 *******************************************/

#ifdef _KSW_MAIN

#include <unistd.h>
#include <stdio.h>
#include <zlib.h>
#include "kseq.h"
KSEQ_INIT(gzFile, err_gzread)

unsigned char seq_nt4_table[256] = {
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

int main(int argc, char *argv[])
{
	int c, sa = 1, sb = 3, i, j, k, forward_only = 0, max_rseq = 0;
	int8_t mat[25];
	int gapo = 5, gape = 2, minsc = 0, xtra = KSW_XSTART;
	uint8_t *rseq = 0;
	gzFile fpt, fpq;
	kseq_t *kst, *ksq;

	// parse command line
	while ((c = getopt(argc, argv, "a:b:q:r:ft:1")) >= 0) {
		switch (c) {
			case 'a': sa = atoi(optarg); break;
			case 'b': sb = atoi(optarg); break;
			case 'q': gapo = atoi(optarg); break;
			case 'r': gape = atoi(optarg); break;
			case 't': minsc = atoi(optarg); break;
			case 'f': forward_only = 1; break;
			case '1': xtra |= KSW_XBYTE; break;
		}
	}
	if (optind + 2 > argc) {
		fprintf(stderr, "Usage: ksw [-1] [-f] [-a%d] [-b%d] [-q%d] [-r%d] [-t%d] <target.fa> <query.fa>\n", sa, sb, gapo, gape, minsc);
		return 1;
	}
	if (minsc > 0xffff) minsc = 0xffff;
	xtra |= KSW_XSUBO | minsc;
	// initialize scoring matrix
	for (i = k = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j)
			mat[k++] = i == j? sa : -sb;
		mat[k++] = 0; // ambiguous base
	}
	for (j = 0; j < 5; ++j) mat[k++] = 0;
	// open file
	fpt = xzopen(argv[optind],   "r"); kst = kseq_init(fpt);
	fpq = xzopen(argv[optind+1], "r"); ksq = kseq_init(fpq);
	// all-pair alignment
	while (kseq_read(ksq) > 0) {
		kswq_t *q[2] = {0, 0};
		kswr_t r;
		for (i = 0; i < (int)ksq->seq.l; ++i) ksq->seq.s[i] = seq_nt4_table[(int)ksq->seq.s[i]];
		if (!forward_only) { // reverse
			if ((int)ksq->seq.m > max_rseq) {
				max_rseq = ksq->seq.m;
				rseq = (uint8_t*)realloc(rseq, max_rseq);
			}
			for (i = 0, j = ksq->seq.l - 1; i < (int)ksq->seq.l; ++i, --j)
				rseq[j] = ksq->seq.s[i] == 4? 4 : 3 - ksq->seq.s[i];
		}
		gzrewind(fpt); kseq_rewind(kst);
		while (kseq_read(kst) > 0) {
			for (i = 0; i < (int)kst->seq.l; ++i) kst->seq.s[i] = seq_nt4_table[(int)kst->seq.s[i]];
			r = ksw_align(ksq->seq.l, (uint8_t*)ksq->seq.s, kst->seq.l, (uint8_t*)kst->seq.s, 5, mat, gapo, gape, xtra, &q[0]);
			if (r.score >= minsc)
				err_printf("%s\t%d\t%d\t%s\t%d\t%d\t%d\t%d\t%d\n", kst->name.s, r.tb, r.te+1, ksq->name.s, r.qb, r.qe+1, r.score, r.score2, r.te2);
			if (rseq) {
				r = ksw_align(ksq->seq.l, rseq, kst->seq.l, (uint8_t*)kst->seq.s, 5, mat, gapo, gape, xtra, &q[1]);
				if (r.score >= minsc)
					err_printf("%s\t%d\t%d\t%s\t%d\t%d\t%d\t%d\t%d\n", kst->name.s, r.tb, r.te+1, ksq->name.s, (int)ksq->seq.l - r.qb, (int)ksq->seq.l - 1 - r.qe, r.score, r.score2, r.te2);
			}
		}
		free(q[0]); free(q[1]);
	}
	free(rseq);
	kseq_destroy(kst); err_gzclose(fpt);
	kseq_destroy(ksq); err_gzclose(fpq);
	return 0;
}
#endif
