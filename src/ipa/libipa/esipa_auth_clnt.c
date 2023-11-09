#include <stdint.h>
#include <errno.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include "utils.h"
#include "length.h"
#include "context.h"
#include "esipa.h"
#include "esipa_auth_clnt.h"
#include <EsipaMessageFromIpaToEim.h>
#include <EsipaMessageFromEimToIpa.h>
#include <AuthenticateClientRequestEsipa.h>
#include <AuthenticateClientResponseEsipa.h>

static struct ipa_buf *enc_auth_clnt_req(struct ipa_esipa_auth_clnt_req *req)
{
	struct EsipaMessageFromIpaToEim msg_to_eim = { 0 };

	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_authenticateClientRequestEsipa;
	msg_to_eim.choice.authenticateClientRequestEsipa = req->req;

	/* Encode */
	return ipa_esipa_msg_to_eim_enc(&msg_to_eim, "AuthenticateClient");
}

static struct ipa_esipa_auth_clnt_res *dec_auth_clnt_res(struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	struct ipa_esipa_auth_clnt_res *res = NULL;

	msg_to_ipa = ipa_esipa_msg_to_ipa_dec(msg_to_ipa_encoded, "AuthenticateClient",
					      EsipaMessageFromEimToIpa_PR_authenticateClientResponseEsipa);
	if (!msg_to_ipa)
		return NULL;

	res = IPA_ALLOC_ZERO(struct ipa_esipa_auth_clnt_res);
	res->msg_to_ipa = msg_to_ipa;

	switch (msg_to_ipa->choice.authenticateClientResponseEsipa.present) {
	case AuthenticateClientResponseEsipa_PR_authenticateClientOkDPEsipa:
		res->auth_clnt_ok_dpe =
		    &msg_to_ipa->choice.authenticateClientResponseEsipa.choice.authenticateClientOkDPEsipa;
		res->transaction_id = res->auth_clnt_ok_dpe->transactionId;
		break;
	case AuthenticateClientResponseEsipa_PR_authenticateClientOkDSEsipa:
		res->auth_clnt_ok_dse =
		    &msg_to_ipa->choice.authenticateClientResponseEsipa.choice.authenticateClientOkDSEsipa;
		res->transaction_id = &res->auth_clnt_ok_dse->transactionId;
		break;
	case AuthenticateClientResponseEsipa_PR_authenticateClientErrorEsipa:
		res->auth_clnt_err =
		    msg_to_ipa->choice.authenticateClientResponseEsipa.choice.authenticateClientErrorEsipa;
		IPA_LOGP_ESIPA("AuthenticateClient", LERROR, "function failed with error code %ld!\n",
			       res->auth_clnt_err);
		break;
	default:
		IPA_LOGP_ESIPA("AuthenticateClient", LERROR, "unexpected response content!\n");
		res->auth_clnt_err = -1;
	}

	return res;
}

struct ipa_esipa_auth_clnt_res *ipa_esipa_auth_clnt(struct ipa_context *ctx, struct ipa_esipa_auth_clnt_req *req)
{
	struct ipa_buf *esipa_req;
	struct ipa_buf *esipa_res;
	struct ipa_esipa_auth_clnt_res *res = NULL;

	IPA_LOGP_ESIPA("AuthenticateClient", LINFO, "Requesting client authentication\n");

	esipa_req = enc_auth_clnt_req(req);
	esipa_res = ipa_esipa_req(ctx, esipa_req, "AuthenticateClient");
	if (!esipa_res)
		goto error;

	res = dec_auth_clnt_res(esipa_res);

	if (!IPA_ASN_STR_CMP(res->transaction_id, &req->req.transactionId)) {
		IPA_LOGP_ESIPA("AuthenticateClient", LERROR, "eIM responded with unexpected transaction ID\n");
		res->auth_clnt_err = -1;
		goto error;
	}

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return res;
}

void ipa_esipa_auth_clnt_res_free(struct ipa_esipa_auth_clnt_res *res)
{
	IPA_ESIPA_RES_FREE(res);
}
