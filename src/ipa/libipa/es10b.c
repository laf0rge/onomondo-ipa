/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/log.h>
#include <asn_application.h>
#include "es10b.h"
#include "context.h"
#include "utils.h"

/*! Decode an ASN.1 encoded eUICC response.
 *  \param[in] td pointer to asn_TYPE_descriptor.
 *  \param[in] es10b_res_encoded pointer to ipa_buf that contains the encoded message.
 *  \param[in] function_name name of the ES10b function (for log messages).
 *  \returns pointer newly allocated ASN.1 struct that contains the decoded message, NULL on error. */
void *ipa_es10b_res_dec(const struct asn_TYPE_descriptor_s *td, const struct ipa_buf *es10b_res_encoded,
			const char *function_name)
{
	asn_dec_rval_t rc;
	void *es10b_res_decoded = NULL;

	assert(es10b_res_encoded);

	rc = ber_decode(0, td, (void **)&es10b_res_decoded, es10b_res_encoded->data, es10b_res_encoded->len);
	if (rc.code != RC_OK) {
		if (rc.code == RC_FAIL) {
			IPA_LOGP_ES10B(function_name, LERROR,
				       "cannot decode eUICC response! (invalid ASN.1 encoded data)\n");
		} else if (rc.code == RC_WMORE) {
			IPA_LOGP_ES10B(function_name, LERROR,
				       "cannot decode eUICC response! (message seems to be truncated)\n");
		} else {
			IPA_LOGP_ES10B(function_name, LERROR, "cannot decode eUICC response! (unknown cause)\n");
		}

		IPA_LOGP_ES10B(function_name, LDEBUG, "the following (incomplete) data was decoded:\n");
		ipa_asn1c_dump(td, es10b_res_decoded, 1, SES10B, LERROR);

		ASN_STRUCT_FREE(*td, es10b_res_decoded);
		return NULL;
	}
#ifdef IPA_DEBUG_ASN1
	IPA_LOGP_ES10B(function_name, LDEBUG, "ES10b message received from eUICC:\n");
	ipa_asn1c_dump(td, es10b_res_decoded, 1, SES10B, LDEBUG);
#endif

	return es10b_res_decoded;
}

/*! Encode an ASN.1 struct that contains an eUICC request.
 *  \param[in] td pointer to asn_TYPE_descriptor.
 *  \param[in] es10b_req_decoded pointer to ASN.1 struct that contains the eUICC request.
 *  \param[in] function_name name of the ES10b function (for log messages).
 *  \returns pointer newly allocated ipa_buf that contains the encoded message, NULL on error. */
struct ipa_buf *ipa_es10b_req_enc(const struct asn_TYPE_descriptor_s *td, const void *es10b_req_decoded,
				  const char *function_name)
{
	struct ipa_buf *es10b_req_encoded = ipa_buf_alloc(IPA_ES10B_ASN_ENCODER_BUF_SIZE);
	asn_enc_rval_t rc;

	assert(es10b_req_decoded);

#ifdef IPA_DEBUG_ASN1
	IPA_LOGP_ES10B(function_name, LDEBUG, "ES10b message that will be sent to eUICC:\n");
	ipa_asn1c_dump(td, es10b_req_decoded, 1, SES10B, LDEBUG);
#endif

	assert(es10b_req_encoded);
	rc = der_encode(td, es10b_req_decoded, ipa_asn1c_consume_bytes_cb, es10b_req_encoded);

	if (rc.encoded <= 0) {
		IPA_LOGP_ES10B(function_name, LERROR, "cannot encode eUICC request!\n");
		IPA_FREE(es10b_req_encoded);
		return NULL;
	}

	return es10b_req_encoded;
}
