#ifndef KURL_H
#define KURL_H

#include <sys/types.h>

#define KURL_NULL       1
#define KURL_INV_WHENCE 2
#define KURL_SEEK_OUT   3

struct kurl_t;
typedef struct kurl_t kurl_t;

#ifdef __cplusplus
extern "C" {
#endif

void kurl_init(void);
void kurl_destroy(void);

kurl_t *kurl_open(const char *url);
int kurl_close(kurl_t *ku);
ssize_t kurl_read(kurl_t *ku, void *buf, size_t nbytes);
off_t kurl_seek(kurl_t *ku, off_t offset, int whence);

off_t kurl_tell(const kurl_t *ku);
int kurl_eof(const kurl_t *ku);
int kurl_fileno(const kurl_t *ku);
int kurl_error(const kurl_t *ku);

#ifdef __cplusplus
}
#endif

#endif
