#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include "kurl.h"

#define KU_DEF_BUFLEN   0x10000
#define KU_MAX_SKIP     (KU_DEF_BUFLEN<<1) // if seek step is smaller than this, skip

#define kurl_isfile(u) ((u)->fd >= 0)

struct kurl_t {
	CURLM *multi; // cURL multi handler
	CURL *curl;   // cURL easy handle
	uint8_t *buf; // buffer
	off_t off0;   // offset of the first byte in the buffer; the actual file offset equals off0 + p_buf
	int fd;       // file descriptor for a normal file; <0 for a network file
	int m_buf;    // max buffer size; for a network file, set to CURL_MAX_WRITE_SIZE*2
	int l_buf;    // length of the buffer; l_buf == 0 iff the input read entirely; l_buf <= m_buf
	int p_buf;    // file position in the buffer; p_buf <= l_buf
	int done_reading; // true if we can read nothing from the file; buffer may not be empty even if done_reading is set
	int err;      // error code
};

void kurl_init(void) // required for SSL and win32 socket; NOT thread safe
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

void kurl_destroy(void)
{
	curl_global_cleanup();
}

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

static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *data) // callback required by cURL
{
	kurl_t *ku = (kurl_t*)data;
	ssize_t nbytes = size * nmemb;
	assert(nbytes + ku->l_buf < ku->m_buf);
	memcpy(ku->buf + ku->l_buf, ptr, nbytes);
	ku->l_buf += nbytes;
	return nbytes;
}

static int fill_buffer(kurl_t *ku) // fill the buffer
{
	assert(ku->p_buf == ku->l_buf); // buffer is always used up when fill_buffer() is called; otherwise a bug
	ku->off0 += ku->l_buf;
	ku->p_buf = ku->l_buf = 0;
	// For a normal file, it is not necessary to test ->done_read. For a cURL connection, however,
	// if we have already read the entire content, this function will be blocked at "select()" below.
	// Probably there are better means to check if reading is over.
	if (ku->done_reading) return 0;
	if (kurl_isfile(ku)) {
		// The following block is equivalent to "ku->l_buf = read(ku->fd, ku->buf, ku->m_buf)" on Mac.
		// On Linux, the man page does not specify whether read() guarantees to read ku->m_buf bytes
		// even if ->fd references a normal file with sufficient remaining bytes.
		while (ku->l_buf < ku->m_buf) {
			int l;
			l = read(ku->fd, ku->buf + ku->l_buf, ku->m_buf - ku->l_buf);
			if (l == 0) break;
			ku->l_buf += l;
		}
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
	free(ku->buf);
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

	ku = (kurl_t*)calloc(1, sizeof(kurl_t));
	ku->fd = is_file? fd : -1;
	if (!kurl_isfile(ku)) {
		ku->multi = curl_multi_init();
		ku->curl  = curl_easy_init();
		curl_easy_setopt(ku->curl, CURLOPT_URL, url);
		curl_easy_setopt(ku->curl, CURLOPT_WRITEDATA, ku);
		curl_easy_setopt(ku->curl, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(ku->curl, CURLOPT_WRITEFUNCTION, write_cb);
	}
	ku->m_buf = kurl_isfile(ku)? KU_DEF_BUFLEN : CURL_MAX_WRITE_SIZE<<1;
	ku->buf = (uint8_t*)calloc(ku->m_buf, 1);
	if (prepare(ku) < 0 || fill_buffer(ku) <= 0) {
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
			if (buf) memcpy((uint8_t*)buf + (nbytes - rest), ku->buf + ku->p_buf, rest);
			ku->p_buf += rest;
			rest = 0;
		} else {
			int ret;
			if (buf && ku->l_buf > ku->p_buf)
				memcpy((uint8_t*)buf + (nbytes - rest), ku->buf + ku->p_buf, ku->l_buf - ku->p_buf);
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
		ku->err = KURL_INV_WHENCE;
		return -1;
	}
	if (new_off < 0) { // negtive absolute offset
		ku->err = KURL_SEEK_OUT;
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
	if (failed) ku->err = KURL_SEEK_OUT, ku->l_buf = ku->p_buf = 0, new_off = -1;
	return new_off;
}

off_t kurl_tell(const kurl_t *ku)
{
	if (ku == 0) return -1;
	return ku->off0 + ku->p_buf;
}

int kurl_eof(const kurl_t *ku)
{
	if (ku == 0) return 1;
	return (ku->l_buf == 0); // unless file end, buffer should never be empty
}

int kurl_fileno(const kurl_t *ku)
{
	if (ku == 0) return -1;
	return ku->fd;
}

int kurl_error(const kurl_t *ku)
{
	if (ku == 0) return KURL_NULL;
	return ku->err;
}

int main()
{
	kurl_t *f;
	int i, l, rc;
	char buf[0x10000];
//	f = kurl_open("f.c");
//	f = kurl_open("http://lh3lh3.users.sourceforge.net/");
	f = kurl_open("https://github.com/");
	rc = kurl_seek(f, 0x100, SEEK_SET);
	fprintf(stderr, "rc=%d\n", rc);
	l = kurl_read(f, buf, 0x10000);
	for (i = 0; i < l; ++i)
		putchar(buf[i]);
	fprintf(stderr, "l=%d,eof=%d,err=%d\n", l, kurl_eof(f), kurl_error(f));
	kurl_close(f);
	return 0;
}
