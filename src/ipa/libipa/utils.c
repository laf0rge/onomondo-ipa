/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
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
#include "utils.h"

/* \! Lookup a numeric value in a num to string map and return the coresponding string.
 *  \param[in] map pointer num to str map.
 *  \param[in] num numeric value of the string to look up.
 *  \param[in] def default string to return in case the numeric value is not found.
 *  \returns found string from map, default string in case of no match. */
const char *ipa_str_from_num(const struct num_str_map *map, long num, const char *def)
{
	do {
		if (map->num == num)
			return map->str;
		map++;
	} while (map->str != NULL);

	return def;
}

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
		sprintf(out_ptr, "%02X", data[i]);
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
	assert(buf_encoded->len <= buf_encoded->data_len);

#ifdef IPA_DEBUG_ASN1
	IPA_LOGP(SIPA, LDEBUG, "ASN.1 encoder output: offset=%zu, size=%zu, buffer=%s\n", buf_encoded->len, size,
		 ipa_hexdump(buffer, size));
#endif

	/* Check whether we still have enough space to store the encoding
	 * results. */
	if (buf_encoded->data_len < buf_encoded->len + size) {
		/* TODO: Maybe work with realloc here?, then we could have small initial buffer sizes and
		 * automatically allocate more memory if needed? */
		IPA_LOGP(SIPA, LERROR,
			 "ASN.1 encoder failed due to small buffer size (have: %zu bytes, required: %zu bytes\n",
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

/*! Compare two strings case insensitive.
 *  \param[in] str1 first string to compare.
 *  \param[in] str2 second string to compare.
 *  \param[in] len length up to which we compare the two strings.
 *  \returns 0 when both strings match, -1 when there is a difference. */
int ipa_cmp_case_insensitive(const char *str1, const char *str2, size_t len)
{
	size_t i;

	if (!str1)
		return -1;
	if (!str2)
		return -1;

	for (i = 0; i < len; i++) {
		if (toupper(str1[i]) != toupper(str2[i]))
			return -1;
	}

	return 0;
}

/*! Check whether a BTLV tag is contained in a tag list.
 *  \param[in] tag tag to search for.
 *  \param[in] tag_list ipa_buf that contains the tag list.
 *  \returns true when the tag is found in the list, false otherwise. */
bool ipa_tag_in_taglist(uint16_t tag, const struct ipa_buf *tag_list)
{
	uint8_t *tag_list_ptr;
	uint16_t tag_from_list;
	size_t tag_bytes_left;

	tag_list_ptr = tag_list->data;
	tag_bytes_left = tag_list->len;

	while (1) {
		if (tag_bytes_left >= 2 && (tag_list_ptr[0] & 0x1F) == 0x1F) {
			tag_from_list = tag_list_ptr[0] << 8;
			tag_from_list |= tag_list_ptr[1];
			tag_list_ptr += 2;
			tag_bytes_left -= 2;
		} else if (tag_bytes_left >= 1 && (tag_list_ptr[0] & 0x1F) != 0x1F) {
			tag_from_list = tag_list_ptr[0];
			tag_list_ptr++;
			tag_bytes_left--;
		} else
			return false;

		if (tag_from_list == tag)
			return true;
	}

	return false;
}
