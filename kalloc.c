#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kalloc.h"

/* In kalloc, a *core* is a large chunk of contiguous memory. Each core is
 * associated with a master header, which keeps the size of the current core
 * and the pointer to next core. Kalloc allocates small *blocks* of memory from
 * the cores and organizes free memory blocks in a circular single-linked list.
 *
 * In the following diagram, "@" stands for the header of a free block (of type
 * header_t), "#" for the header of an allocated block (of type size_t), "-"
 * for free memory, and "+" for allocated memory.
 *
 * master        This region is core 1.          master           This region is core 2.
 *      |                                             |
 *      *@-------#++++++#++++++++++++@--------        *@----------#++++++++++++#+++++++@------------
 *       |                           |                 |                               |
 *       p=p->ptr->ptr->ptr->ptr     p->ptr            p->ptr->ptr                     p->ptr->ptr->ptr
 */
typedef struct header_t {
	size_t size;
	struct header_t *ptr;
} header_t;

typedef struct {
	void *par;
	size_t min_core_size;
	header_t base, *loop_head, *core_head; /* base is a zero-sized block always kept in the loop */
} kmem_t;

static void panic(const char *s)
{
	fprintf(stderr, "%s\n", s);
	abort();
}

void *km_init2(void *km_par, size_t min_core_size)
{
	kmem_t *km;
	km = (kmem_t*)kcalloc(km_par, 1, sizeof(kmem_t));
	km->par = km_par;
	if (km_par) km->min_core_size = min_core_size > 0? min_core_size : ((kmem_t*)km_par)->min_core_size - 2;
	else km->min_core_size = min_core_size > 0? min_core_size : 0x80000;
	return (void*)km;
}

void *km_init(void) { return km_init2(0, 0); }

void km_destroy(void *_km)
{
	kmem_t *km = (kmem_t*)_km;
	void *km_par;
	header_t *p, *q;
	if (km == NULL) return;
	km_par = km->par;
	for (p = km->core_head; p != NULL;) {
		q = p->ptr;
		kfree(km_par, p);
		p = q;
	}
	kfree(km_par, km);
}

static header_t *morecore(kmem_t *km, size_t nu)
{
	header_t *q;
	size_t bytes, *p;
	nu = (nu + 1 + (km->min_core_size - 1)) / km->min_core_size * km->min_core_size; /* the first +1 for core header */
	bytes = nu * sizeof(header_t);
	q = (header_t*)kmalloc(km->par, bytes);
	if (!q) panic("[morecore] insufficient memory");
	q->ptr = km->core_head, q->size = nu, km->core_head = q;
	p = (size_t*)(q + 1);
	*p = nu - 1; /* the size of the free block; -1 because the first unit is used for the core header */
	kfree(km, p + 1); /* initialize the new "core"; NB: the core header is not looped. */
	return km->loop_head;
}

void kfree(void *_km, void *ap) /* kfree() also adds a new core to the circular list */
{
	header_t *p, *q;
	kmem_t *km = (kmem_t*)_km;
	
	if (!ap) return;
	if (km == NULL) {
		free(ap);
		return;
	}
	p = (header_t*)((size_t*)ap - 1);
	p->size = *((size_t*)ap - 1);
	/* Find the pointer that points to the block to be freed. The following loop can stop on two conditions:
	 *
	 * a) "p>q && p<q->ptr": @------#++++++++#+++++++@-------    @---------------#+++++++@-------
	 *    (can also be in    |      |                |        -> |                       |
	 *     two cores)        q      p           q->ptr           q                  q->ptr
	 *
	 *                       @--------    #+++++++++@--------    @--------    @------------------
	 *                       |            |         |         -> |            |
	 *                       q            p    q->ptr            q       q->ptr
	 *
	 * b) "q>=q->ptr && (p>q || p<q->ptr)":  @-------#+++++   @--------#+++++++     @-------#+++++   @----------------
	 *                                       |                |        |         -> |                |
	 *                                  q->ptr                q        p       q->ptr                q
	 *
	 *                                       #+++++++@-----   #++++++++@-------     @-------------   #++++++++@-------
	 *                                       |       |                 |         -> |                         |
	 *                                       p  q->ptr                 q       q->ptr                         q
	 */
	for (q = km->loop_head; !(p > q && p < q->ptr); q = q->ptr)
		if (q >= q->ptr && (p > q || p < q->ptr)) break;
	if (p + p->size == q->ptr) { /* two adjacent blocks, merge p and q->ptr (the 2nd and 4th cases) */
		p->size += q->ptr->size;
		p->ptr = q->ptr->ptr;
	} else if (p + p->size > q->ptr && q->ptr >= p) {
		panic("[kfree] The end of the allocated block enters a free block.");
	} else p->ptr = q->ptr; /* backup q->ptr */

	if (q + q->size == p) { /* two adjacent blocks, merge q and p (the other two cases) */
		q->size += p->size;
		q->ptr = p->ptr;
		km->loop_head = q;
	} else if (q + q->size > p && p >= q) {
		panic("[kfree] The end of a free block enters the allocated block.");
	} else km->loop_head = p, q->ptr = p; /* in two cores, cannot be merged; create a new block in the list */
}

void *kmalloc(void *_km, size_t n_bytes)
{
	kmem_t *km = (kmem_t*)_km;
	size_t n_units;
	header_t *p, *q;

	if (n_bytes == 0) return 0;
	if (km == NULL) return malloc(n_bytes);
	n_units = (n_bytes + sizeof(size_t) + sizeof(header_t) - 1) / sizeof(header_t); /* header+n_bytes requires at least this number of units */

	if (!(q = km->loop_head)) /* the first time when kmalloc() is called, intialize it */
		q = km->loop_head = km->base.ptr = &km->base;
	for (p = q->ptr;; q = p, p = p->ptr) { /* search for a suitable block */
		if (p->size >= n_units) { /* p->size if the size of current block. This line means the current block is large enough. */
			if (p->size == n_units) q->ptr = p->ptr; /* no need to split the block */
			else { /* split the block. NB: memory is allocated at the end of the block! */
				p->size -= n_units; /* reduce the size of the free block */
				p += p->size; /* p points to the allocated block */
				*(size_t*)p = n_units; /* set the size */
			}
			km->loop_head = q; /* set the end of chain */
			return (size_t*)p + 1;
		}
		if (p == km->loop_head) { /* then ask for more "cores" */
			if ((p = morecore(km, n_units)) == 0) return 0;
		}
	}
}

void *kcalloc(void *_km, size_t count, size_t size)
{
	kmem_t *km = (kmem_t*)_km;
	void *p;
	if (size == 0 || count == 0) return 0;
	if (km == NULL) return calloc(count, size);
	p = kmalloc(km, count * size);
	memset(p, 0, count * size);
	return p;
}

void *krealloc(void *_km, void *ap, size_t n_bytes) // TODO: this can be made more efficient in principle
{
	kmem_t *km = (kmem_t*)_km;
	size_t cap, *p, *q;

	if (n_bytes == 0) {
		kfree(km, ap); return 0;
	}
	if (km == NULL) return realloc(ap, n_bytes);
	if (ap == NULL) return kmalloc(km, n_bytes);
	p = (size_t*)ap - 1;
	cap = (*p) * sizeof(header_t) - sizeof(size_t);
	if (cap >= n_bytes) return ap; /* TODO: this prevents shrinking */
	q = (size_t*)kmalloc(km, n_bytes);
	memcpy(q, ap, cap);
	kfree(km, ap);
	return q;
}

void *krelocate(void *km, void *ap, size_t n_bytes)
{
	void *p;
	if (km == 0 || ap == 0) return ap;
	p = kmalloc(km, n_bytes);
	memcpy(p, ap, n_bytes);
	kfree(km, ap);
	return p;
}

void km_stat(const void *_km, km_stat_t *s)
{
	kmem_t *km = (kmem_t*)_km;
	header_t *p;
	memset(s, 0, sizeof(km_stat_t));
	if (km == NULL || km->loop_head == NULL) return;
	for (p = km->loop_head;; p = p->ptr) {
		s->available += p->size * sizeof(header_t);
		if (p->size != 0) ++s->n_blocks; /* &kmem_t::base is always one of the cores. It is zero-sized. */
		if (p->ptr > p && p + p->size > p->ptr)
			panic("[km_stat] The end of a free block enters another free block.");
		if (p->ptr == km->loop_head) break;
	}
	for (p = km->core_head; p != NULL; p = p->ptr) {
		size_t size = p->size * sizeof(header_t);
		++s->n_cores;
		s->capacity += size;
		s->largest = s->largest > size? s->largest : size;
	}
}

void km_stat_print(const void *km)
{
	km_stat_t st;
	km_stat(km, &st);
	fprintf(stderr, "[km_stat] cap=%ld, avail=%ld, largest=%ld, n_core=%ld, n_block=%ld\n",
			st.capacity, st.available, st.largest, st.n_blocks, st.n_cores);
}
