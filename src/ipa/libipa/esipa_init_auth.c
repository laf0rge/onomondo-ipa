#include <stdint.h>
#include <errno.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include "utils.h"
#include "length.h"
#include "context.h"
#include "esipa.h"
#include "esipa_init_auth.h"
#include "es10b_get_euicc_info.h"
#include <EsipaMessageFromIpaToEim.h>
#include <EsipaMessageFromEimToIpa.h>
#include <InitiateAuthenticationRequestEsipa.h>
#include <InitiateAuthenticationResponseEsipa.h>

static struct ipa_buf *enc_init_auth_req(struct ipa_init_auth_req *init_auth_req)
{
	struct EsipaMessageFromIpaToEim msg_to_eim = { 0 };
	asn_enc_rval_t rc;
	struct ipa_buf *buf_encoded;

	UTF8String_t smdp_address = { 0 };
	EUICCInfo1_t euicc_info_1 = { 0 };
	SubjectKeyIdentifier_t *key_id_item;
	int i;

	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_initiateAuthenticationRequestEsipa;

	/* add eUICC challenge */
	IPA_ASSIGN_BUF_TO_ASN(msg_to_eim.choice.initiateAuthenticationRequestEsipa.euiccChallenge,
			      init_auth_req->euicc_challenge, IPA_LEN_EUICC_CHLG);

	/* add SMDP addr */
	if (init_auth_req->smdp_addr_present) {
		msg_to_eim.choice.initiateAuthenticationRequestEsipa.smdpAddress = &smdp_address;
		IPA_ASSIGN_STR_TO_ASN(smdp_address, init_auth_req->smdp_addr);
	}

	/* eUICC info */
	msg_to_eim.choice.initiateAuthenticationRequestEsipa.euiccInfo1 = &euicc_info_1;
	IPA_ASSIGN_BUF_TO_ASN(euicc_info_1.svn, init_auth_req->euicc_info->svn, sizeof(init_auth_req->euicc_info->svn));
	for (i = 0; i < init_auth_req->euicc_info->ci_pkid_verf_count; i++) {
		key_id_item = IPA_ALLOC(SubjectKeyIdentifier_t);
		IPA_COPY_IPA_BUF_TO_ASN(key_id_item, init_auth_req->euicc_info->ci_pkid_verf[i]);
		ASN_SEQUENCE_ADD(&euicc_info_1.euiccCiPKIdListForVerification.list, key_id_item);
	}
	for (i = 0; i < init_auth_req->euicc_info->ci_pkid_sign_count; i++) {
		key_id_item = IPA_ALLOC(SubjectKeyIdentifier_t);
		IPA_COPY_IPA_BUF_TO_ASN(key_id_item, init_auth_req->euicc_info->ci_pkid_sign[i]);
		ASN_SEQUENCE_ADD(&euicc_info_1.euiccCiPKIdListForSigning.list, key_id_item);
	}

#ifdef IPA_DEBUG_ASN1
	ipa_asn1c_dump(&asn_DEF_EsipaMessageFromIpaToEim, &msg_to_eim, 0, SESIPA, LINFO);
#endif

	/* Encode */
	buf_encoded = ipa_buf_alloc(1024);
	assert(buf_encoded);
	rc = der_encode(&asn_DEF_EsipaMessageFromIpaToEim, &msg_to_eim, ipa_asn1c_consume_bytes_cb, buf_encoded);

	/* We have added dynamically allocated items to an ASN list object (see above), we must now ensure that those
	 * items are properly freed. */
	IPA_FREE_ASN_SEQUENCE_OF_STRINGS(euicc_info_1.euiccCiPKIdListForVerification);
	IPA_FREE_ASN_SEQUENCE_OF_STRINGS(euicc_info_1.euiccCiPKIdListForSigning);

	if (rc.encoded <= 0) {
		IPA_FREE(buf_encoded);
		return NULL;
	}

	return buf_encoded;
}

static struct ipa_init_auth_res *dec_init_auth_res(struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	asn_dec_rval_t rc;
	struct ipa_init_auth_res *init_auth_res = NULL;

	if (msg_to_ipa_encoded->len == 0) {
		IPA_LOGP_ESIPA("InitiateAuthentication", LERROR, "eIM response contained no data!\n");
		return NULL;
	}

	rc = ber_decode(0, &asn_DEF_EsipaMessageFromEimToIpa,
			(void **)&msg_to_ipa, msg_to_ipa_encoded->data, msg_to_ipa_encoded->len);

	if (rc.code != RC_OK) {
		IPA_LOGP_ESIPA("InitiateAuthentication", LERROR, "cannot decode eIM response!\n");
		goto error;
	}

#ifdef IPA_DEBUG_ASN1
	ipa_asn1c_dump(&asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa, 0, SESIPA, LINFO);
#endif

	if (msg_to_ipa->present != EsipaMessageFromEimToIpa_PR_initiateAuthenticationResponseEsipa) {
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "unexpected eIM response\n");
		goto error;
	}

	init_auth_res = IPA_ALLOC(struct ipa_init_auth_res);
	assert(init_auth_res);
	memset(init_auth_res, 0, sizeof(*init_auth_res));
	init_auth_res->msg_to_ipa = msg_to_ipa;

	switch (msg_to_ipa->choice.initiateAuthenticationResponseEsipa.present) {
	case InitiateAuthenticationResponseEsipa_PR_initiateAuthenticationOkEsipa:
		init_auth_res->init_auth_ok =
		    &msg_to_ipa->choice.initiateAuthenticationResponseEsipa.choice.initiateAuthenticationOkEsipa;
		break;
	case InitiateAuthenticationResponseEsipa_PR_initiateAuthenticationErrorEsipa:
	default:
		init_auth_res->init_auth_err =
		    msg_to_ipa->choice.initiateAuthenticationResponseEsipa.choice.initiateAuthenticationErrorEsipa;
		break;
	}

error:
	return init_auth_res;
}

struct ipa_init_auth_res *ipa_esipa_init_auth(struct ipa_context *ctx, struct ipa_init_auth_req *init_auth_req)
{
	struct ipa_buf *esipa_req;
	struct ipa_buf *esipa_res;
	struct ipa_init_auth_res *init_auth_res = NULL;
	int rc;

	IPA_LOGP_ESIPA("InitiateAuthentication", LINFO, "Requesting authentication with eUICC challenge: %s\n",
		       ipa_hexdump(init_auth_req->euicc_challenge, IPA_LEN_EUICC_CHLG));

	esipa_req = enc_init_auth_req(init_auth_req);
	esipa_res = ipa_buf_alloc(IPA_LIMIT_HTTP_REQ);
	rc = ipa_http_req(ctx->http_ctx, esipa_res, esipa_req, ipa_esipa_get_eim_url(ctx));
	if (rc < 0) {
		IPA_LOGP(SIPA, LERROR, "eIM package request failed!\n");
		goto error;
	}

	init_auth_res = dec_init_auth_res(esipa_res);

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return init_auth_res;
}

void ipa_esipa_init_auth_dump_res(struct ipa_init_auth_res *init_auth_res, uint8_t indent,
				  enum log_subsys log_subsys, enum log_level log_level)
{
	char indent_str[256];

	memset(indent_str, ' ', indent);
	indent_str[indent] = '\0';

	IPA_LOGP(log_subsys, log_level, "%seIM initiate authentication response: \n", indent_str);

	if (!init_auth_res) {
		IPA_LOGP(log_subsys, log_level, "%s (none)\n", indent_str);
		return;
	}
	if (init_auth_res->init_auth_ok)
		ipa_asn1c_dump(&asn_DEF_InitiateAuthenticationOkEsipa, init_auth_res->init_auth_ok, indent + 1,
			       log_subsys, log_level);
	else
		IPA_LOGP(log_subsys, log_level, "%s init_auth_err = %ld \n", indent_str, init_auth_res->init_auth_err);
}

void ipa_esipa_init_auth_free_res(struct ipa_init_auth_res *init_auth_res)
{
	if (!init_auth_res)
		return;

	ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromEimToIpa, init_auth_res->msg_to_ipa);
	IPA_FREE(init_auth_res);
}
