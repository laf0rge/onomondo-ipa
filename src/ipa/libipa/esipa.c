/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/http.h>
#include "esipa.h"
#include "context.h"
#include "utils.h"
#include "length.h"

#define PREFIX_HTTP "http://"
#define PREFIX_HTTPS "https://"
#define SUFFIX "/gsma/rsp2/asn1"

/*! Read the configured eIM URL (FQDN).
 *  \param[in] ctx pointer to ipa_context.
 *  \returns pointer to eIM URL (statically allocated, do not free). */
char *ipa_esipa_get_eim_url(struct ipa_context *ctx)
{
	static char eim_url[IPA_ESIPA_URL_MAXLEN];
	size_t url_len = 0;

	memset(eim_url, 0, sizeof(eim_url));

	/* Make sure we have a reasonable minimum of space available */
	assert(sizeof(eim_url) > 255);

	if (ctx->cfg->eim_use_ssl)
		strcpy(eim_url, PREFIX_HTTPS);
	else
		strcpy(eim_url, PREFIX_HTTP);

	/* Be sure we don't accidentally overrun the buffer */
	url_len = strlen(eim_url);
	url_len += strlen(ctx->cfg->eim_addr);
	url_len += strlen(SUFFIX);
	assert(url_len < sizeof(eim_url));

	strcat(eim_url, ctx->cfg->eim_addr);
	strcat(eim_url, SUFFIX);

	return eim_url;
}

/*! Decode an ASN.1 encoded eIM to IPA message.
 *  \param[in] msg_to_ipa_encoded pointer to ipa_buf that contains the encoded message.
 *  \param[in] function_name name of the ESipa function (for log messages).
 *  \param[in] epected_res_type type of the expected eIM response (for plausibility check).
 *  \returns pointer newly allocated ASN.1 struct that contains the decoded message, NULL on error. */
struct EsipaMessageFromEimToIpa *ipa_esipa_msg_to_ipa_dec(const struct ipa_buf *msg_to_ipa_encoded,
							  const char *function_name,
							  enum EsipaMessageFromEimToIpa_PR epected_res_type)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	asn_dec_rval_t rc;

	assert(msg_to_ipa_encoded);

	if (msg_to_ipa_encoded->len == 0) {
		IPA_LOGP_ESIPA(function_name, LERROR, "eIM response contained no data!\n");
		return NULL;
	}

	rc = ber_decode(0, &asn_DEF_EsipaMessageFromEimToIpa,
			(void **)&msg_to_ipa, msg_to_ipa_encoded->data, msg_to_ipa_encoded->len);

	if (rc.code != RC_OK) {
		if (rc.code == RC_FAIL) {
			IPA_LOGP_ESIPA(function_name, LERROR,
				       "cannot decode eIM response! (invalid ASN.1 encoded data)\n");
		} else if (rc.code == RC_WMORE) {
			IPA_LOGP_ESIPA(function_name, LERROR,
				       "cannot decode eIM response! (message seems to be truncated)\n");
		} else {
			IPA_LOGP_ESIPA(function_name, LERROR, "cannot decode eIM response! (unknown cause)\n");
		}

		IPA_LOGP_ESIPA(function_name, LDEBUG, "the following (incomplete) data was decoded:\n");
		ipa_asn1c_dump(&asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa, 1, SESIPA, LERROR);

		ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa);
		return NULL;
	}
#ifdef IPA_DEBUG_ASN1
	IPA_LOGP_ESIPA(function_name, LDEBUG, "ESipa message received from eIM:\n");
	ipa_asn1c_dump(&asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa, 1, SESIPA, LDEBUG);
#endif

	if (msg_to_ipa->present != epected_res_type) {
		IPA_LOGP_ESIPA(function_name, LERROR, "unexpected eIM response\n");
		ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa);
		return NULL;
	}

	return msg_to_ipa;
}

/*! Encode an ASN.1 struct that contains an IPA to eIM message.
 *  \param[in] msg_to_eim pointer to ASN.1 struct that contains the IPA to eIM message.
 *  \param[in] function_name name of the ESipa function (for log messages).
 *  \returns pointer newly allocated ipa_buf that contains the encoded message, NULL on error. */
struct ipa_buf *ipa_esipa_msg_to_eim_enc(const struct EsipaMessageFromIpaToEim *msg_to_eim, const char *function_name)
{
	struct ipa_buf *buf_encoded;
	asn_enc_rval_t rc;

	assert(msg_to_eim);
	assert(msg_to_eim != EsipaMessageFromIpaToEim_PR_NOTHING);

#ifdef IPA_DEBUG_ASN1
	IPA_LOGP_ESIPA(function_name, LDEBUG, "ESipa message that will be sent to eIM:\n");
	ipa_asn1c_dump(&asn_DEF_EsipaMessageFromIpaToEim, msg_to_eim, 1, SESIPA, LDEBUG);
#endif

	buf_encoded = ipa_buf_alloc(IPA_ESIPA_ASN_ENCODER_BUF_SIZE);
	assert(buf_encoded);

	rc = der_encode(&asn_DEF_EsipaMessageFromIpaToEim, msg_to_eim, ipa_asn1c_consume_bytes_cb, buf_encoded);
	if (rc.encoded <= 0) {
		IPA_LOGP_ESIPA(function_name, LERROR, "cannot encode eIM request!\n");
		IPA_FREE(buf_encoded);
		return NULL;
	}

	return buf_encoded;
}

/*! Perform a request towards the eIM.
 *  \param[in] ctx pointer to ipa_context.
 *  \param[in] esipa_req ipa_buf with encoded request data
 *  \param[in] function_name name of the ESipa function (for log messages).
 *  \returns pointer newly allocated ipa_buf that contains the encoded response from the eIM, NULL on error. */
struct ipa_buf *ipa_esipa_req(struct ipa_context *ctx, const struct ipa_buf *esipa_req, const char *function_name)
{
	struct ipa_buf *esipa_res;
	int rc;

	if (!esipa_req) {
		IPA_LOGP_ESIPA(function_name, LERROR, "eIM request failed due to missing encoded request data!\n");
		return NULL;
	}

	esipa_res = ipa_buf_alloc(IPA_LIMIT_HTTP_REQ);
	assert(esipa_res);

	rc = ipa_http_req(ctx->http_ctx, esipa_res, esipa_req, ipa_esipa_get_eim_url(ctx));
	if (rc < 0) {
		IPA_LOGP_ESIPA(function_name, LERROR, "eIM request failed!\n");
		goto error;
	}

	return esipa_res;
error:

	IPA_FREE(esipa_res);
	return NULL;
}
