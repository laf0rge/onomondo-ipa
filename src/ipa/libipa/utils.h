#pragma once

#include <onomondo/ipa/utils.h>
#include <stddef.h>

/* \! Callback function to be passed to asn1c decoder function. */
int ipa_asn1c_consume_bytes_cb(const void *buffer, size_t size, void *priv);

/* \! Copy an ASN.1 string object into a dynamically allocated IPA_BUF. This macro is used in situations where the ASN.1
 *    specification defines a string type with arbitrary length. Then the target buffer where the data is copied to will
 *    be implemented as a pointer of type uint8_t.
 *  \param[in] asn1_obj pointer to asn1c generated string object to read from.
 *  \returns dynamically allocated IPA_BUF with contents of asn1_obj. */
#define IPA_BUF_FROM_ASN(asn1_obj) ({ \
assert(asn1_obj); \
struct ipa_buf *__ipa_buf; \
__ipa_buf = ipa_buf_alloc((asn1_obj)->size);	\
assert(__ipa_buf); \
memcpy(__ipa_buf->data, (asn1_obj)->buf, (asn1_obj)->size);	\
__ipa_buf->len = (asn1_obj)->size;				\
__ipa_buf; })

/* \! Copy an ASN.1 string object into a dynamically allocated char array. This macro is used in situations where the
 *    ASN.1 specification defines a printable string of an arbitrary length. Then the target buffer where the data is
 *    copied to will be implemented as a pointer of type char.
 *  \param[in] asn1_obj pointer to asn1c generated string object to read from.
 *  \returns null terminated char array with contents of asn1_obj. */
#define IPA_STR_FROM_ASN(asn1_obj) ({ \
assert(asn1_obj); \
char *__str; \
__str = IPA_ALLOC_N((asn1_obj)->size + 1);	\
assert(__str); \
memcpy(__str, (asn1_obj)->buf, (asn1_obj)->size);	\
__str[(asn1_obj)->size] = '\0';				\
__str; })

/* \! Copy an ASN.1 string object into a statically allocated buffer. This macro is used in situations where the ASN.1
 *    specification defines a fixed length string type. Then the target buffer will be implemented as a fixed length
 *    statically allocated buffer. (To check the buffer sizes, this macro uses sizeof(dest_buf) and the size member of
 *    the ASN.1 object. The target buffer must be bigger or equal in size as the size of the ASN.1 string.)
 *  \param[out] dest_buf destination buffer where the data is copied to.
 *  \param[in] asn1_obj pointer to asn1c generated string object to read from.
 *  \returns 0 on success, -ENOMEM on failure. */
#define IPA_COPY_ASN_BUF(dest_buf, asn1_obj) ({ \
int __rc = -ENOMEM; \
memset(dest_buf, 0, sizeof(dest_buf));			\
if (sizeof(dest_buf) >= (asn1_obj)->size) {		\
memcpy(dest_buf, (asn1_obj)->buf, (asn1_obj)->size);	\
__rc = 0; \
} \
__rc; })
