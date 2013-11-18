#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <errno.h>
#include "kurl.h"

#define KU_DEF_BUFLEN   0x10000
//#define KU_MAX_SKIP     (KU_DEF_BUFLEN<<1) // if seek step is smaller than this, skip
#define KU_MAX_SKIP     0

typedef struct {
	CURLM *multi;
	CURL *curl;
	char *url;
	off_t off0; // offset of the first byte in the buffer
	uint8_t *buf;
	int fd, l_buf, m_buf, p_buf;
	int err, done_reading;
} kurl_t;

#define kurl_isfile(u) ((u)->fd >= 0)

static int prepare(kurl_t *ku)
{
	if (kurl_isfile(ku)) {
		if (ku->off0 > 0 && lseek(ku->fd, ku->off0, SEEK_SET) != ku->off0)
			return -1;
	} else {
		int rc;
		rc = curl_multi_remove_handle(ku->multi, ku->curl);
		rc = curl_easy_setopt(ku->curl, CURLOPT_RESUME_FROM, ku->off0);
		rc = curl_multi_add_handle(ku->multi, ku->curl);
	}
	ku->p_buf = ku->l_buf = 0; // empty the buffer
	return 0;
}

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *data)
{
	kurl_t *ku = (kurl_t*)data;
	size_t nbytes = size * nmemb;
	assert(nbytes + ku->l_buf < ku->m_buf);
	memcpy(ku->buf + ku->l_buf, ptr, nbytes);
	ku->l_buf += nbytes;
	return nbytes;
}

static int fill_buffer(kurl_t *ku)
{
	assert(ku->p_buf == ku->l_buf); // buffer is always used up when fill_buffer() is called; otherwise a bug
	ku->off0 += ku->l_buf;
	ku->p_buf = 0;
	if (ku->done_reading) return 0;
	if (ku->fd >= 0) {
		ku->l_buf = read(ku->fd, ku->buf, ku->m_buf);
		if (ku->l_buf < ku->m_buf) ku->done_reading = 1;
	} else {
		int n_running, rc;
		fd_set fdr, fdw, fde;
		do {
			int maxfd = -1;
			long curl_to = -1;
			struct timeval to;
			// the following adaped from docs/examples/fopen.c 
			FD_ZERO(&fdr); FD_ZERO(&fdw); FD_ZERO(&fde);
			to.tv_sec = 10, to.tv_usec = 0; // 10 seconds
			curl_multi_timeout(ku->multi, &curl_to);
			if (curl_to >= 0) {
				to.tv_sec = curl_to / 1000;
				if (to.tv_sec > 1) to.tv_sec = 1;
				else to.tv_usec = (curl_to % 1000) * 1000;
			}
			curl_multi_fdset(ku->multi, &fdr, &fdw, &fde, &maxfd);
			rc = select(maxfd+1, &fdr, &fdw, &fde, &to);
			if (rc >= 0) rc = curl_multi_perform(ku->multi, &n_running);
			else break;
		} while (n_running && ku->l_buf < CURL_MAX_WRITE_SIZE);
		if (ku->l_buf < CURL_MAX_WRITE_SIZE) ku->done_reading = 1;
	}
	return ku->l_buf;
}

int kurl_close(kurl_t *ku)
{
	if (ku == 0) return 0;
	if (ku->fd < 0) {
		curl_multi_remove_handle(ku->multi, ku->curl);
		curl_easy_cleanup(ku->curl);
		curl_multi_cleanup(ku->multi);
	} else close(ku->fd);
	free(ku->url); free(ku->buf);
	free(ku);
	return 0;
}

kurl_t *kurl_open(const char *url)
{
	const char *p, *q;
	kurl_t *ku;
	int fd = -1, is_file = 1;

	p = strstr(url, "://");
	if (p && *p) {
		for (q = url; q != p; ++q)
			if (!isalnum(*q)) break;
		if (q == p) is_file = 0;
	}
	if (is_file && (fd = open(url, O_RDONLY)) < 0) return 0;

	ku = calloc(1, sizeof(kurl_t));
	ku->fd = is_file? fd : -1;
	ku->url = strdup(url);
	if (!kurl_isfile(ku)) {
		ku->multi = curl_multi_init();
		ku->curl  = curl_easy_init();
		curl_easy_setopt(ku->curl, CURLOPT_URL, ku->url);
		curl_easy_setopt(ku->curl, CURLOPT_WRITEDATA, ku);
		curl_easy_setopt(ku->curl, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(ku->curl, CURLOPT_WRITEFUNCTION, write_cb);
	}
	ku->m_buf = kurl_isfile(ku)? KU_DEF_BUFLEN : CURL_MAX_WRITE_SIZE<<1;
	ku->buf = calloc(ku->m_buf, 1);
	if (prepare(ku) < 0 || fill_buffer(ku) == 0) {
		kurl_close(ku);
		return 0;
	}
	return ku;
}

ssize_t kurl_read(kurl_t *ku, void *buf, size_t nbytes)
{
	ssize_t rest = nbytes;
	if (ku->l_buf == 0) return 0; // end-of-file
	while (rest) {
		if (ku->l_buf - ku->p_buf >= rest) {
			if (buf) memcpy(buf + (nbytes - rest), ku->buf + ku->p_buf, rest);
			ku->p_buf += rest;
			rest = 0;
		} else {
			int ret;
			if (buf && ku->l_buf > ku->p_buf)
				memcpy(buf + (nbytes - rest), ku->buf + ku->p_buf, ku->l_buf - ku->p_buf);
			rest -= ku->l_buf - ku->p_buf;
			ku->p_buf = ku->l_buf;
			ret = fill_buffer(ku);
			if (ret <= 0) break;
		}
	}
	return nbytes - rest;
}

off_t kurl_seek(kurl_t *ku, off_t offset, int whence)
{
	off_t new_off = -1, cur_off;
	int failed = 0;
	if (ku == 0) return -1;
	cur_off = ku->off0 + ku->p_buf;
	if (whence == SEEK_SET) new_off = offset;
	else if (whence == SEEK_CUR) new_off += cur_off + offset;
	else { // not supported whence
		ku->err = EINVAL;
		return -1;
	}
	if (new_off - cur_off + ku->p_buf < ku->l_buf) {
		ku->p_buf += new_off - cur_off;
		return ku->off0 + ku->p_buf;
	}
	if (new_off - cur_off > KU_MAX_SKIP) { // if jump is large, do actual seek
		ku->off0 = new_off;
		ku->done_reading = 0;
		if (prepare(ku) < 0 || fill_buffer(ku) <= 0) failed = 1;
	} else { // if jump is small, read through
		off_t r;
		r = kurl_read(ku, 0, new_off - cur_off);
		if (r + cur_off != new_off) failed = 1; // out of range
	}
	if (failed) ku->err = EINVAL, ku->l_buf = ku->p_buf = 0, new_off = -1;
	return new_off;
}

off_t kurl_tell(const kurl_t *ku)
{
	return ku->off0 + ku->p_buf;
}

int kurl_eof(const kurl_t *ku)
{
	return (ku->l_buf == 0); // unless file end, buffer should never be empty
}

int kurl_fileno(const kurl_t *ku)
{
	return ku->fd;
}

int kurl_error(const kurl_t *ku)
{
	return ku->err;
}

int main()
{
	kurl_t *f;
	int i, l, rc;
	char buf[0x10000];
//	f = kurl_open("f.c");
	f = kurl_open("http://lh3lh3.users.sourceforge.net/");
	rc = kurl_seek(f, 0x100, SEEK_SET);
	fprintf(stderr, "rc=%d\n", rc);
	l = kurl_read(f, buf, 0x10000);
	for (i = 0; i < l; ++i)
		putchar(buf[i]);
	fprintf(stderr, "l=%d,eof=%d,err=%d\n", l, kurl_eof(f), kurl_error(f));
	kurl_close(f);
	return 0;
}
