/*
 * Author: Philipp Maier
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/scard.h>
#include <onomondo/ipa/log.h>
#include "context.h"
#include "length.h"
#include "utils.h"
#include "euicc.h"
#include "es10b.h"
#include "es10b_get_euicc_chlg.h"
#include "GetEuiccChallengeRequest.h"
#include "GetEuiccChallengeResponse.h"
#include "Octet16.h"

static struct ipa_buf *enc_get_euicc1_challenge(void)
{
	struct GetEuiccChallengeRequest asn = { 0 };
	asn_enc_rval_t rc;
	struct ipa_buf *buf_encoded = ipa_buf_alloc(32);

	assert(buf_encoded);
	rc = der_encode(&asn_DEF_GetEuiccChallengeRequest, &asn, ipa_asn1c_consume_bytes_cb, buf_encoded);

	if (rc.encoded <= 0) {
		IPA_FREE(buf_encoded);
		return NULL;
	}

	return buf_encoded;
}

static int dec_get_euicc2_challenge(uint8_t *euicc_challenge, struct ipa_buf *es10b_res)
{
	asn_dec_rval_t rc;
	struct GetEuiccChallengeResponse *asn = NULL;

	rc = ber_decode(0, &asn_DEF_GetEuiccChallengeResponse, (void **)&asn, es10b_res->data, es10b_res->len);
	if (rc.code != RC_OK) {
		IPA_LOGP_ES10B("GetEuiccChallengeResponse", LERROR, "cannot decode eUICC response! (invalid asn1c)\n");
		return -EINVAL;
	}
#ifdef IPA_DEBUG_ASN1
	asn_fprint(stderr, &asn_DEF_GetEuiccChallengeResponse, asn);
#endif

	COPY_ASN_BUF(euicc_challenge, IPA_LEN_EUICC_CHALLENGE, &asn->euiccChallenge);
	ASN_STRUCT_FREE(asn_DEF_GetEuiccChallengeResponse, asn);

	return 0;
}

int ipa_es10b_get_euicc_challenge(struct ipa_context *ctx, uint8_t *euicc_challenge)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	int rc = -EINVAL;

	es10b_req = enc_get_euicc1_challenge();
	if (!es10b_req) {
		IPA_LOGP_ES10B("GetEuiccChallengeRequest", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10B("GetEuiccChallengeRequest", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_euicc2_challenge(euicc_challenge, es10b_res);
	if (rc < 0)
		goto error;

error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return rc;
}
