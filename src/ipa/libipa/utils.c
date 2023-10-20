/*
 * Author: Philipp Maier
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>

/*! Generate a hexdump string from the input data.
 *  \param[in] data pointer to binary data.
 *  \returns pointer to generated human readable string. */
#define IPA_HEXDUMP_MAX 4
#define IPA_HEXDUMP_BUFSIZE 1024
char *ipa_hexdump(const uint8_t *data, size_t len)
{
	static char out[IPA_HEXDUMP_MAX][IPA_HEXDUMP_BUFSIZE];
	static uint8_t idx = 0;
	char *out_ptr;
	size_t i;

	idx++;
	idx = idx % IPA_HEXDUMP_MAX;
	out_ptr = out[idx];

	if (!data)
		return ("(null)");

	for (i = 0; i < len; i++) {
		sprintf(out_ptr, "%02x", data[i]);
		out_ptr += 2;

		/* put three dots and exit early in case we are running out of
		 * space */
		if (i > IPA_HEXDUMP_BUFSIZE / 2 - 4) {
			sprintf(out_ptr, "...");
			return out[idx];
		}
	}

	*out_ptr = '\0';
	return out[idx];
}

int ipa_asn1c_consume_bytes_cb(const void *buffer, size_t size, void *priv)
{
	struct ipa_buf *buf_encoded = priv;

	assert(priv);

	/* Check whether we still have enough space to store the encoding
	 * results. */
	if (buf_encoded->data_len < buf_encoded->len + size) {
		IPA_LOGP(SIPA, LERROR, "ASN.1 decode failed due to small buffer size (have: %zu bytes, required: %zu bytes\n",
			 buf_encoded->data_len, buf_encoded->len + size);
		return -ENOMEM;
	}

	memcpy(buf_encoded->data + buf_encoded->len, buffer, size);
	buf_encoded->len += size;

	return 0;
}
