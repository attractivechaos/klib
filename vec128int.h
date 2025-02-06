/******************************************************************************/
/*                                                                            */
/* Licensed Materials - Property of IBM                                       */
/*                                                                            */
/* IBM Power Vector Intrinisic Functions version 1.0.3                        */
/*                                                                            */
/* Copyright IBM Corp. 2014,2016                                              */
/* US Government Users Restricted Rights - Use, duplication or                */
/* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.          */
/*                                                                            */
/* See the licence in the license subdirectory.                               */
/*                                                                            */
/* More information on this software is available on the IBM DeveloperWorks   */
/* website at                                                                 */
/*  https://www.ibm.com/developerworks/community/groups/community/powerveclib */
/*                                                                            */
/******************************************************************************/

#ifndef _H_VEC128INT
#define _H_VEC128INT

#include <altivec.h>

#define VECLIB_ALIGNED8 __attribute__ ((__aligned__ (8)))
#define VECLIB_ALIGNED16 __attribute__ ((__aligned__ (16)))

#ifdef __LITTLE_ENDIAN__
  #define __vsx_lowest_left_half_char_in_memory  8
  #define __vsr_lowest_left_half_short_in_memory 4
  #define __vsr_lowest_left_half_int_in_memory   2
  #define __vsr_left_half_long_long_in_memory    1
  #define __vsr_right_half_long_long_in_memory   0
#elif __BIG_ENDIAN__
  #define __vsx_lowest_left_half_char_in_memory  7
  #define __vsr_lowest_left_half_short_in_memory 3
  #define __vsr_lowest_left_half_int_in_memory   1
  #define __vsr_left_half_long_long_in_memory    0
  #define __vsr_right_half_long_long_in_memory   1
#endif

/* Control inlining */
#ifdef NOT_ALWAYS_INLINE
  #define VECLIB_INLINE
#else
  #define VECLIB_INLINE static inline __attribute__ ((__always_inline__))
#endif
#define VECLIB_NOINLINE static __attribute__ ((__noinline__))

#define VECLIB_ALIGNED16 __attribute__ ((__aligned__ (16)))

typedef
  VECLIB_ALIGNED8
  unsigned long long
__m64;

typedef
  VECLIB_ALIGNED16
  vector unsigned char
__m128i;

typedef
  VECLIB_ALIGNED16
  union {
    __m128i                   as_m128i;
    __m64                     as_m64               [2];
    vector signed   char      as_vector_signed_char;
    vector unsigned char      as_vector_unsigned_char;
    vector bool     char      as_vector_bool_char;
    vector signed   short     as_vector_signed_short;
    vector unsigned short     as_vector_unsigned_short;
    vector bool     short     as_vector_bool_short;
    vector signed   int       as_vector_signed_int;
    vector unsigned int       as_vector_unsigned_int;
    vector bool     int       as_vector_bool_int;
    vector signed   long long as_vector_signed_long_long;
    vector unsigned long long as_vector_unsigned_long_long;
    vector bool     long long as_vector_bool_long_long;
    char                      as_char              [16];
    short                     as_short             [8];
    int                       as_int               [4];
    unsigned int              as_unsigned_int      [4];
    long long                 as_long_long         [2];
  } __m128i_union;

typedef const long intlit3;  /* 3 bit int literal */
typedef const long intlit8;  /* 8 bit int literal */

/******************************************************** Load ********************************************************/

/* Load 128-bits of integer data, aligned */
VECLIB_INLINE __m128i vec_load1q (__m128i const* address)
{
  return (__m128i) vec_ld (0, (vector unsigned char*) address);
}

/******************************************************** Set *********************************************************/

/* Splat 8-bit char to 16 8-bit chars */
VECLIB_INLINE __m128i vec_splat16sb (char scalar)
{ return (__m128i) vec_splats ((signed char) scalar); }

/* Splat 16-bit short to 8 16-bit shorts */
VECLIB_INLINE __m128i vec_splat8sh (short scalar)
{ return (__m128i) vec_splats (scalar); }

/* Splat 32-bit int to 4 32-bit ints */
VECLIB_INLINE __m128i vec_splat4sw (int scalar)
{ return (__m128i) vec_splats (scalar); }

/******************************************************** Store *******************************************************/

/* Store 128-bits integer, aligned */
VECLIB_INLINE void vec_store1q (__m128i* address, __m128i v)
{ vec_st (v, 0, address); }


/******************************************************* Extract ******************************************************/

/* Extract upper bit of 16 8-bit chars */
VECLIB_INLINE int vec_extractupperbit16sb (__m128i v)
{
  __m128i_union t;
  t.as_m128i = v;
  int result = 0;
  #ifdef __LITTLE_ENDIAN__
    result |= (t.as_char[15] & 0x80) << (15-7);
    result |= (t.as_char[14] & 0x80) << (14-7);
    result |= (t.as_char[13] & 0x80) << (13-7);
    result |= (t.as_char[12] & 0x80) << (12-7);
    result |= (t.as_char[11] & 0x80) << (11-7);
    result |= (t.as_char[10] & 0x80) << (10-7);
    result |= (t.as_char[9]  & 0x80) <<  (9-7);
    result |= (t.as_char[8]  & 0x80) <<  (8-7);
    result |= (t.as_char[7]  & 0x80);
    result |= (t.as_char[6]  & 0x80) >>  (7-6);
    result |= (t.as_char[5]  & 0x80) >>  (7-5);
    result |= (t.as_char[4]  & 0x80) >>  (7-4);
    result |= (t.as_char[3]  & 0x80) >>  (7-3);
    result |= (t.as_char[2]  & 0x80) >>  (7-2);
    result |= (t.as_char[1]  & 0x80) >>  (7-1);
    result |= (t.as_char[0]  & 0x80) >>   7;
  #elif __BIG_ENDIAN__
    result |= (t.as_char[0]  & 0x80) << (15-7);
    result |= (t.as_char[1]  & 0x80) << (14-7);
    result |= (t.as_char[2]  & 0x80) << (13-7);
    result |= (t.as_char[3]  & 0x80) << (12-7);
    result |= (t.as_char[4]  & 0x80) << (11-7);
    result |= (t.as_char[5]  & 0x80) << (10-7);
    result |= (t.as_char[6]  & 0x80) <<  (9-7);
    result |= (t.as_char[7]  & 0x80) <<  (8-7);
    result |= (t.as_char[8]  & 0x80);
    result |= (t.as_char[9]  & 0x80) >>  (7-6);
    result |= (t.as_char[10] & 0x80) >>  (7-5);
    result |= (t.as_char[11] & 0x80) >>  (7-4);
    result |= (t.as_char[12] & 0x80) >>  (7-3);
    result |= (t.as_char[13] & 0x80) >>  (7-2);
    result |= (t.as_char[14] & 0x80) >>  (7-1);
    result |= (t.as_char[15] & 0x80) >>   7;
  #endif
  return result;
}

/* Extract 16-bit short from one of 8 16-bit shorts */
VECLIB_INLINE int vec_extract8sh (__m128i v, intlit3 element_from_right)
{
  __m128i_union t;
  #ifdef __LITTLE_ENDIAN__
    t.as_m128i = v;
    return t.as_short[element_from_right & 7];
  #elif __BIG_ENDIAN__
    static const vector unsigned char permute_selector[8] = {
    /* To extract specified halfword element into lowest halfword of the left half with other elements zeroed */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x1E,0x1F, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 0 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x1C,0x1D, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 1 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x1A,0x1B, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 2 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x18,0x19, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 3 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x16,0x17, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 4 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x14,0x15, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 5 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x12,0x13, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F },  /* element 6 */
      { 0x00,0x01, 0x02,0x03, 0x04,0x05, 0x10,0x11, 0x08,0x09, 0x0A,0x0B, 0x0C,0x0D, 0x0E,0x0F }   /* element 7 */
    };
    t.as_m128i = vec_perm (vec_splats ((unsigned char) 0), v, permute_selector[element_from_right & 7]);
    return (short) t.as_long_long[__vsr_left_half_long_long_in_memory];
  #endif
}

/***************************************************** Arithmetic *****************************************************/

/* Add 16 8-bit chars with signed saturation */
VECLIB_INLINE __m128i vec_addsaturating16sb (__m128i left, __m128i right)
{
  return (__m128i) vec_adds ((vector signed char) left, (vector signed char) right);
}

/* Add 16 8-bit chars with unsigned saturation */
VECLIB_INLINE __m128i vec_addsaturating16ub (__m128i left, __m128i right)
{ return (__m128i) vec_adds ((vector unsigned char) left, (vector unsigned char) right); }

/* Add 8 16-bit shorts with signed saturation */
VECLIB_INLINE __m128i vec_addsaturating8sh (__m128i left, __m128i right)
{ return (__m128i) vec_adds ((vector signed short) left, (vector signed short) right); }

/* Subtract 16 8-bit chars with unsigned saturation */
VECLIB_INLINE __m128i vec_subtractsaturating16ub (__m128i left, __m128i right)
{ return (__m128i) vec_subs ((vector unsigned char) left, (vector unsigned char) right); }

/* Subtract 8 16-bit shorts with unsigned saturation */
VECLIB_INLINE __m128i vec_subtractsaturating8uh (__m128i left, __m128i right)
{ return (__m128i) vec_subs ((vector unsigned short) left, (vector unsigned short) right); }

/* Max 8 16-bit shorts */
VECLIB_INLINE __m128i vec_max8sh (__m128i left, __m128i right)
{ return (__m128i) vec_max ((vector signed short) left, (vector signed short) right); }

/* Max 16 8-bit unsigned chars */
VECLIB_INLINE __m128i vec_max16ub (__m128i left, __m128i right)
{ return (__m128i) vec_max ((vector unsigned char) left, (vector unsigned char) right); }

#ifdef __LITTLE_ENDIAN__
  #define LEleft_BEright left
  #define LEright_BEleft right
#elif __BIG_ENDIAN__
  #define LEleft_BEright right
  #define LEright_BEleft left
#endif

/****************************************************** Shift *********************************************************/

/*- SSE2 shifts >= 32 produce 0; Altivec/MMX shifts by count%count_size. */
/*- The Altivec spec says the element shift count vector register must have a shift count in each element */
/*- and the shift counts may be different for each element. */
/*- The PowerPC Architecture says all elements must contain the same shift count. That wins. */
/*- The element shift count_size is: byte shift: 3 bits (0-7), halfword: 4 bits (0-15), word: 5 bits (0-31). */
/*- For full vector shifts the Altivec/PowerPC bit shift count is in the rightmost 7 bits, */
/*- with a 4 bit slo/sro byte shift count then a 3 bit sll/srl bit shift count. */

/* Shift left */

/* Shift 128-bits left logical immediate by bytes */
VECLIB_INLINE __m128i vec_shiftleftbytes1q (__m128i v, intlit8 bytecount)
{
  if ((unsigned long) bytecount >= 16)
  {
    /* SSE2 shifts >= element_size or < 0 produce 0; Altivec/MMX shifts by bytecount%element_size. */
    return (__m128i) vec_splats (0);
  } else if (bytecount == 0) {
    return v;
  } else {
    /* The PowerPC byte shift count must be multiplied by 8. */
    /* It need not but can be replicated, which handles both LE and BE shift count positioning. */
    __m128i_union replicated_count;
    replicated_count.as_m128i = vec_splat16sb (bytecount << 3);
    return (__m128i) vec_slo (v, replicated_count.as_m128i);
  }
}

/* Shift right */

/* Shift 128-bits right logical immediate by bytes */
VECLIB_INLINE __m128i vec_shiftrightbytes1q (__m128i v, intlit8 bytecount)
{
  if ((unsigned long) bytecount >= 16)
  {
    /* SSE2 shifts >= element_size or < 0 produce 0; Altivec/MMX shifts by bytecount%element_size. */
    return (__m128i) vec_splats (0);
  } else if (bytecount == 0) {
    return v;
  } else {
    /* The PowerPC byte shift count must be multiplied by 8. */
    /* It need not but can be replicated, which handles both LE and BE shift count positioning. */
    __m128i_union replicated_count;
    replicated_count.as_m128i = vec_splat16sb (bytecount << 3);
    /* AT gcc v7.1 may miscompile vec_sro as vec_slo? */
    return (__m128i) vec_sro (v, replicated_count.as_m128i);
  }
}

/******************************************************* Compare ******************************************************/

/* Compare eq */

/* Compare 16 8-bit chars for == to vector mask */
VECLIB_INLINE __m128i vec_compareeq16sb (__m128i left, __m128i right)
{ return (__m128i) vec_cmpeq ((vector signed char) left, (vector signed char) right); }

/* Compare 8 16-bit shorts for > to vector mask */
VECLIB_INLINE __m128i vec_comparegt8sh (__m128i left, __m128i right)
{ return (__m128i) vec_cmpgt ((vector signed short) left, (vector signed short) right); }

#endif
