#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "mem.h"

#define IPA_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char *ipa_hexdump(const uint8_t *data, size_t len);

struct ipa_buf {
	/*! pointer to allocated memory */
	uint8_t *data;

	/*! actual length of the allocated memory. This parameter is set when
	 * the buffer is allocated and must not be modified by the API user. */
	size_t data_len;

	/*! length of useful data. This value may be modified by the API user,
	 * it is used to tell how many bytes with useful data are stored. */
	size_t len;
};

/*! Generate a hexdump string from an ipa_buf object.
 *  \param[in] buf pointer to ipa_buf object.
 *  \returns pointer to generated human readable string. */
static inline char *ipa_buf_hexdump(const struct ipa_buf *buf)
{
	if (!buf)
		return "(null)";
	return ipa_hexdump(buf->data, buf->len);
}

/*! Allocate a new ipa_buf object.
 *  \param[in] len number of bytes to allocate inside ipa_buf.
 *  \returns pointer to newly allocated ipa_buf object. */
static inline struct ipa_buf *ipa_buf_alloc(size_t len)
{
	struct ipa_buf *buf = IPA_ALLOC_N(sizeof(*buf) + len);
	assert(buf);

	memset(buf, 0, sizeof(*buf));
	buf->data = (uint8_t *) buf + sizeof(*buf);
	buf->data_len = len;

	/* A newly allocated ipa_buf naturally has 0 bytes of
	 * useful data in it. */
	buf->len = 0;

	return buf;
}

/*! Duplicate (exact copy) from another ipa_buf object.
 *  \param[in] buf ipa_buf object to duplicate.
 *  \returns pointer to newly allocated ipa_buf object. */
static inline struct ipa_buf *ipa_buf_dup(const struct ipa_buf *buf)
{
	struct ipa_buf *buf_dup = ipa_buf_alloc(buf->data_len);
	memcpy(buf_dup->data, buf->data, buf->data_len);
	buf_dup->len = buf->len;
	return buf_dup;
}

/*! Allocate a new ipa_buf and copy the data from another ipa_buf object.
 *  \param[in] buf ipa_buf object to copy from.
 *  \returns pointer to newly allocated ipa_buf object. */
static inline struct ipa_buf *ipa_buf_copy(const struct ipa_buf *buf)
{
	struct ipa_buf *buf_dup = ipa_buf_alloc(buf->len);
	memcpy(buf_dup->data, buf->data, buf->len);
	buf_dup->len = buf->len;
	return buf_dup;
}

/*! Free an ipa_buf object.
 *  \param[in] pointer to ipa_buf object to free. */
static inline void ipa_buf_free(struct ipa_buf *buf)
{
	IPA_FREE(buf);
}
