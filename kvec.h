/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractor@live.co.uk>

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

/*
  An example:

#include "kvec.h"
int main() {
	kvec_t(int) array;
	kv_init(array);
	kv_push(int, array, 10); // append
	kv_a(int, array, 20) = 5; // dynamic
	kv_A(array, 20) = 4; // static
	kv_destroy(array);
	return 0;
}
*/

/*
  2008-09-22 (0.1.0):

	* The initial version.

*/

#ifndef AC_KVEC_H
#define AC_KVEC_H

#include <stdlib.h>

#define kv_roundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

/*
 * Declare a vector struct with the given type.
 *
 * A vector has 2 attributes, size and max which can be accessed with
 * kv_size and kv_max. The size of a vector is the number of elements contained
 * in the vector. The size is initially zero after the vector has been
 * initialised. The max attribute refers to the capacity of the vector. A
 * vector can store up to max elements without any additional allocation of
 * memory.
 *
 * This declaration will typically allocate the storage for the vector, but the
 * data will not be initialised. To initialise the vector you should use the
 * kv_init function.
 */
#define kvec_t(type) struct { size_t n, m; type *a; }

/*
 * Initialise a vector.
 * This function will initialise a vector to have a size of 0 and a capacity of
 * zero. You must call this function on a vector before you can access the
 * vector or call any other functions on it. 
 */
#define kv_init(v) ((v).n = (v).m = 0, (v).a = 0)

/*
 * Destroy a vector.
 * This function release any memory used by the vector.
 * Note: after calling this function the vector should not be accessed. If you
 * need to re-use the vector it is safe to call kv_init() again.
 */
#define kv_destroy(v) free((v).a)

/*
 * Access the item in the array at position i.
 * This can be used to read or write to the element.
 * 
 * This function gives direct access to the vector storage without any bounds
 * checking. You must ensure that i is non-negative, and less than the size of
 * the vector for reading, and less than the max of the vector for writing.
 *
 * Usage:
 * kvec_t(int) array; kv_init(array); // []
 * kv_push(int, array, 6);            // [6]
 * kv_push(int, array, 7);            // [6, 7]
 * printf("%d\n", kv_A(array, 0));    // => 6
 * kv_A(array, 1) = 9;                // [6, 9]
 * printf("%d\n", kv_A(array, 1));    // => 9
 */
#define kv_A(v, i) ((v).a[(i)])

/*
 * Pop an element from the end of the vector.
 * This function returns that element, and can be used as the RHS of an
 * assignment or as part of an expression.
 */
#define kv_pop(v) ((v).a[--(v).n])

/*
 * Get the number of elements currently in the vector.
 */
#define kv_size(v) ((v).n)

/*
 * Get the max size of the vector.
 */
#define kv_max(v) ((v).m)

#define kv_resize(type, v, s)  ((v).m = (s), (v).a = (type*)realloc((v).a, sizeof(type) * (v).m))

#define kv_copy(type, v1, v0) do {							\
		if ((v1).m < (v0).n) kv_resize(type, v1, (v0).n);	\
		(v1).n = (v0).n;									\
		memcpy((v1).a, (v0).a, sizeof(type) * (v0).n);		\
	} while (0)												\

/*
 * Push the given value x onto the end of the vector.
 * This function returns void and cannot be used in an expression.
 */
#define kv_push(type, v, x) do {									\
		if ((v).n == (v).m) {										\
			(v).m = (v).m? (v).m<<1 : 2;							\
			(v).a = (type*)realloc((v).a, sizeof(type) * (v).m);	\
		}															\
		(v).a[(v).n++] = (x);										\
	} while (0)

/*
 * This function returns a pointer to a location in the vector into which
 * a value can be assigne.
 *
 * Usage:
 *   kvec_t(int) array; kv_init(array);
 *   kv_pushp(int, array) = 42;
 */
#define kv_pushp(type, v) (((v).n == (v).m)?							\
						   ((v).m = ((v).m? (v).m<<1 : 2),				\
							(v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0)	\
						   : 0), ((v).a + ((v).n++))

#define kv_a(type, v, i) (((v).m <= (size_t)(i)? \
						  ((v).m = (v).n = (i) + 1, kv_roundup32((v).m), \
						   (v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0) \
						  : (v).n <= (size_t)(i)? (v).n = (i) + 1 \
						  : 0), (v).a[(i)])

#endif
