#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

/*
  An example:

#include <stdio.h>
#include "kskiplist.h"

SKIPLIST_DECLARE_TYPE(priority, 8, long, 0.25)

int calc_level(skiplist_node_t(priority) *node) {
  int ret = 0;
  while (node->forward[ret++]) {}
  return ret;
}

int main() {
  typedef skiplist_t(priority) priority_t;
  typedef skiplist_node_t(priority) priority_node_t;
  int times = 10000;
  priority_t p;
  skiplist_init(priority, &p);

  while (--times >= 0) {
    priority_node_t *node = (priority_node_t *)malloc(sizeof(priority_node_t));
    skiplist_node_init(priority, node, times);
    skiplist_insert(priority, &p, node);
  }

  priority_node_t *n;

  skiplist_for_each(n, &p) {
    printf("\n%d -> %ld", calc_level(n), n->score);
  }

  printf("\n");

  priority_node_t *n1;
  priority_node_t *n2;
  skiplist_for_each_clear(n1, n2, &p, free);
}

*/

#include <stdlib.h>
#include <string.h>

#define __SKIPLIST_TYPE(name, level, score_type) \
  typedef struct skiplist_##name##_node_s skiplist_##name##_node_t; \
  struct skiplist_##name##_node_s { \
    score_type score; \
    skiplist_##name##_node_t *forward[level]; \
  }; \
  typedef struct { \
    skiplist_##name##_node_t header; \
  } skiplist_##name##_t;

#define __SKIPLIST_IMPL(name, level, score_type, p) \
  static inline void skiplist_##name##_node_init(skiplist_##name##_node_t *node, score_type score) { \
    bzero(node, sizeof(skiplist_##name##_node_t)); \
    node->score = score; \
  } \
  static inline void skiplist_##name##_init(skiplist_##name##_t *sl) { \
    bzero(sl, sizeof(skiplist_##name##_t)); \
  } \
  static inline int skiplist_##name##_random_level() { \
    int ret = 1; \
    while ((random() & 0xFFFF) < (p * 0xFFFF)) { \
      ++ret; \
    } \
    return ret; \
  } \
  static inline void skiplist_##name##_insert(skiplist_##name##_t *sl, skiplist_##name##_node_t *node) { \
    skiplist_##name##_node_t *x = &sl->header; \
    int i, l = skiplist_##name##_random_level(); \
    if (l > level) { \
      l = level; \
    } \
    for (i = level - 1; i > l - 1; --i) { \
      while (x->forward[i] && x->forward[i]->score <= node->score) { \
        x = x->forward[i]; \
      } \
      node->forward[i] = NULL; \
    } \
    for (; i >= 0; --i) { \
      while (x->forward[i] && x->forward[i]->score <= node->score) { \
        x = x->forward[i]; \
      } \
      node->forward[i] = x->forward[i]; \
      x->forward[i] = node; \
    } \
  } \
  static inline skiplist_##name##_node_t *skiplist_##name##_extract(skiplist_##name##_t *sl, score_type min, score_type max) { \
    skiplist_##name##_node_t *prev = &sl->header; \
    skiplist_##name##_node_t *last = NULL; \
    skiplist_##name##_node_t *ret = NULL; \
    int i; \
    for (i = level - 1; i >= 0; --i) { \
      while (prev->forward[i] && prev->forward[i]->score < min) { \
        prev = prev->forward[i]; \
      } \
      if (!last && prev->forward[i] && prev->forward[i]->score >= min) { \
        last = prev; \
      } \
      if (last) { \
        while (last->forward[i] && last->forward[i]->score <= max) { \
          last = last->forward[i]; \
        } \
        prev->forward[i] = last->forward[i]; \
      } \
    } \
    if (prev->forward[0]->score >= min) { \
      ret = prev->forward[0]; \
      last->forward[0] = NULL; \
    } \
    return ret; \
  } \
  static inline skiplist_##name##_node_t *skiplist_##name##_extract_lte(skiplist_##name##_t *sl, score_type score) { \
    skiplist_##name##_node_t *first = sl->header.forward[0]; \
    skiplist_##name##_node_t *last = &sl->header; \
    skiplist_##name##_node_t *ret = NULL; \
    int i; \
    if (first) { \
      for (i = level - 1; i >= 0; --i) { \
        while (last->forward[i] && last->forward[i]->score <= score) { \
          last = last->forward[i]; \
        } \
        sl->header.forward[i] = last->forward[i]; \
      } \
      if (last != &sl->header) { \
        ret = first; \
        last->forward[0] = NULL; \
      } \
    } \
    return ret; \
  } \
  static inline skiplist_##name##_node_t *skiplist_##name##_shift(skiplist_##name##_t *sl) { \
    skiplist_##name##_node_t *ret = sl->header.forward[0]; \
    if (ret) { \
      int i = level; \
      while (--i >= 0) { \
        sl->header.forward[i] = ret->forward[i]; \
      } \
    } \
    return ret; \
  }

#define SKIPLIST_DECLARE_TYPE(name, level, score_type, p) \
  __SKIPLIST_TYPE(name, (level), score_type) \
  __SKIPLIST_IMPL(name, (level), score_type, (p))

#define skiplist_t(name) skiplist_##name##_t
#define skiplist_node_t(name) skiplist_##name##_node_t
#define skiplist_node_init(name, node, score) skiplist_##name##_node_init(node, score)
#define skiplist_init(name, sl) skiplist_##name##_init(sl)
#define skiplist_insert(name, sl, node) skiplist_##name##_insert(sl, node)
#define skiplist_extract(name, sl, min, max) skiplist_##name##_extract(sl, min, max)
#define skiplist_shift(name, sl) skiplist_##name##_shift(sl)
#define skiplist_shift_lte(name, sl, score, node, n) \
  for (node = skiplist_##name##_extract_lte(sl, score), n = node ? node->forward[0] : NULL; node; node = n, n = node ? node->forward[0] : NULL)

#define skiplist_entry(ptr, type, member) ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))
#define skiplist_for_each(node, sl) \
  for (node = (sl)->header.forward[0]; node; node = node->forward[0])
#define skiplist_for_each_clear(node, n, sl, fn) \
  do { \
    for (node = (sl)->header.forward[0]; node; node = n) { \
      n = node->forward[0]; \
        fn(node); \
    } \
    bzero((sl), sizeof(*(sl))); \
  } while (0)

#endif //!__SKIPLIST_H__
