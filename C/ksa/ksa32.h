#pragma once

int ksa_bwt(unsigned char *T, int n, int k);

/**
 * Recursively construct the suffix array for a string containing multiple
 * sentinels. NULL is taken as the sentinel.
 *
 * @param T   NULL terminated input string (there can be multiple NULLs)
 * @param SA  output suffix array
 * @param fs  working space available in SA (typically 0 when first called)
 * @param n   length of T, including the trailing NULL
 * @param k   size of the alphabet (typically 256 when first called)
 * @param cs  # bytes per element in T; 1 or sizeof(saint_t) (typically 1 when first called)
 *
 * @return    0 upon success
 */
int ksa_core(unsigned char const *T, int *SA, int fs, int n, int k, int cs);

/**
 * Construct the suffix array for a NULL terminated string possibly containing
 * multiple sentinels (NULLs).
 *
 * @param T[0..n-1]  NULL terminated input string
 * @param SA[0..n-1] output suffix array
 * @param n          length of the given string, including NULL
 * @param k          size of the alphabet including the sentinel; no more than 256
 * @return           0 upon success
 */
int ksa_sa(unsigned char const *T, int *SA, int n, int k);
