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
#include <EsipaMessageFromIpaToEim.h>
#include <EsipaMessageFromEimToIpa.h>
#include <InitiateAuthenticationRequestEsipa.h>
#include <InitiateAuthenticationResponseEsipa.h>

static struct ipa_buf *enc_init_auth_req(struct ipa_init_auth_req *init_auth_req)
{
	struct EsipaMessageFromIpaToEim msg_to_eim = { 0 };
	UTF8String_t smdp_address = { 0 };

	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_initiateAuthenticationRequestEsipa;

	/* add eUICC challenge */
	IPA_ASSIGN_BUF_TO_ASN(msg_to_eim.choice.initiateAuthenticationRequestEsipa.euiccChallenge,
			      init_auth_req->euicc_challenge, IPA_LEN_EUICC_CHLG);

	/* add SMDP addr */
	if (init_auth_req->smdp_addr) {
		msg_to_eim.choice.initiateAuthenticationRequestEsipa.smdpAddress = &smdp_address;
		IPA_ASSIGN_STR_TO_ASN(smdp_address, init_auth_req->smdp_addr);
	}

	/* eUICC info */
	msg_to_eim.choice.initiateAuthenticationRequestEsipa.euiccInfo1 = init_auth_req->euicc_info_1;

	/* Encode */
        return ipa_esipa_msg_to_eim_enc(&msg_to_eim, "InitiateAuthentication");
}

static struct ipa_init_auth_res *dec_init_auth_res(struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	struct ipa_init_auth_res *init_auth_res = NULL;

	msg_to_ipa =
	    ipa_esipa_msg_to_ipa_dec(msg_to_ipa_encoded, "InitiateAuthentication",
				     EsipaMessageFromEimToIpa_PR_initiateAuthenticationResponseEsipa);
	if (!msg_to_ipa)
		return NULL;

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
		IPA_LOGP_ESIPA("InitiateAuthentication", LERROR, "eIM package request failed!\n");
		goto error;
	}

	init_auth_res = dec_init_auth_res(esipa_res);

	if (!init_auth_res->init_auth_ok)
		IPA_LOGP_ESIPA("InitiateAuthentication", LERROR, "function failed with error code %ld!\n", init_auth_res->init_auth_err);

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return init_auth_res;
}

void ipa_esipa_init_auth_res_dump(struct ipa_init_auth_res *init_auth_res, uint8_t indent,
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

void ipa_esipa_init_auth_res_free(struct ipa_init_auth_res *init_auth_res)
{
	if (!init_auth_res)
		return;

	ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromEimToIpa, init_auth_res->msg_to_ipa);
	IPA_FREE(init_auth_res);
}
