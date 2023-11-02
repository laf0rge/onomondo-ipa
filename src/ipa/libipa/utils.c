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
#include <asn_application.h>

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

/*! Generate a hexdump string from the input data.
 *  \param[in] buffer pointer to chunk with encoded data.
 *  \param[in] size length of encoded data chunk.
 *  \param[out] priv pointer to caller provided ipa_buf that is used to store the encoded data.
 *  \returns 0 on success, -ENOMEM on error. */
int ipa_asn1c_consume_bytes_cb(const void *buffer, size_t size, void *priv)
{
	struct ipa_buf *buf_encoded = priv;

	assert(priv);
	assert(buffer);

	/* Check whether we still have enough space to store the encoding
	 * results. */
	if (buf_encoded->data_len < buf_encoded->len + size) {
		/* TODO: Maybe work with realloc here?, then we could have small initial buffer sizes and
		 * automatically allocate more memory if needed? */
		IPA_LOGP(SIPA, LERROR,
			 "ASN.1 decode failed due to small buffer size (have: %zu bytes, required: %zu bytes\n",
			 buf_encoded->data_len, buf_encoded->len + size);
		return -ENOMEM;
	}

	memcpy(buf_encoded->data + buf_encoded->len, buffer, size);
	buf_encoded->len += size;

	return 0;
}

struct ipa_asn1c_dump_buf {
	char printbuf[10240];
	char *printbuf_ptr;
};

static int ipa_asn1c_dump_consume(const void *buffer, size_t size, void *app_key)
{

	struct ipa_asn1c_dump_buf *buf = app_key;

	if ((buf->printbuf_ptr - buf->printbuf) + size >= sizeof(buf->printbuf)) {
		IPA_LOGP(SMAIN, LERROR, "ASN.1 print buffer too small - cannot display full struct content!\n");
		return -EINVAL;
	}

	memcpy(buf->printbuf_ptr, buffer, size);
	buf->printbuf_ptr[size] = '\0';

	buf->printbuf_ptr += size;

	return 0;
}

/*! dump contents of a decoded ASN.1 structure.
 *  \param[in] td pointer to asn_TYPE_descriptor.
 *  \param[in] struct_ptr pointer to decoded ASN.1 struct.
 *  \param[in] indent indentation level of the generated output.
 *  \param[in] log_subsys log subsystem to generate the output for.
 *  \param[in] log_level log level to generate the output for. */
void ipa_asn1c_dump(const struct asn_TYPE_descriptor_s *td, const void *struct_ptr, uint8_t indent,
		    enum log_subsys log_subsys, enum log_level log_level)
{
	char indent_str[256];
	static struct ipa_asn1c_dump_buf buf;

	memset(indent_str, ' ', indent);
	indent_str[indent] = '\0';

	buf.printbuf_ptr = buf.printbuf;
	td->op->print_struct(td, struct_ptr, 1, ipa_asn1c_dump_consume, &buf);

	char *token = strtok(buf.printbuf, "\n");

	while (token != NULL) {
		IPA_LOGP(log_subsys, log_level, "%s %s\n", indent_str, token);
		token = strtok(NULL, "\n");
	}
}
