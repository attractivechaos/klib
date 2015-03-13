// skhash -- snabb variant of khash ("khash without macros" or "khash for FFI")
//
// This is an adaptation of khash with a programming interface that is
// not based on C macros. The sizes of keys and values are given as
// integer parameters. The hash and equality predicts are given as
// function pointers. Iteration is not provided: the user should
// implement that if needed by direct inspection of the data
// structure.
//
// This interface is intended to be simpler to use as a library for
// other programming languages, particularly LuaJIT.
//
// (See license at end of file.)

struct skh {
  uint32_t keysize, valuesize;
  uint32_t *flags;
  char *keys;
  char *vals;
  uint32_t (*hash)(struct skh *h, const char *key);
  bool (*equal)(struct skh *h, const char *key1, const char *key2);
  uint32_t n_buckets, size, n_occupied, upper_bound;
};

// prototypes

// Create a hashtable.
//   keysize:   number of bytes per key
//   valuesize: number of bytes per value (or 0 to make a set)
//   hash:      hash function
//   equal:     equality predict
struct skh *skh_init(uint32_t keysize, 
		     uint32_t valuesize, 
		     uint32_t (*hash)(struct skh *h, const char *key),
		     bool (*equal)(struct skh *h, const char *key1, const char *key2));

void skh_destroy(struct skh *h);
void skh_clear(struct skh *h);
// Return a pointer to the table value or NULL if the key is not found.
char *skh_get(struct skh *h, const char *key);
int skh_resize(struct skh *h, int32_t new_n_buckets);
// Return a pointer to the table value.
char *skh_put(struct skh *h, const char *key, int *ret);
void skh_del(struct skh *h, uint32_t x);

// Generic routines that work for any size
uint32_t skh_hash_generic(struct skh *h, const char *key);
bool skh_equal_generic(struct skh *h, const char *key1, const char *key2);

// Specialized routines that assume a fixed size
uint32_t skh_hash_16(struct skh *h, const char *key);
bool skh_equal_16(struct skh *h, const char *key1, const char *key2);

