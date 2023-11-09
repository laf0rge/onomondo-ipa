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
#include "utils.h"
#include "euicc.h"
#include "es10b.h"
#include <AuthenticateServerRequest.h>
#include <AuthenticateServerResponse.h>
#include "es10b_auth_serv.h"

static int dec_auth_serv_res(struct ipa_es10b_auth_serv_res *res, struct ipa_buf *es10b_res)
{
	struct AuthenticateServerResponse *asn = NULL;

	asn = ipa_es10b_res_dec(&asn_DEF_AuthenticateServerResponse, es10b_res, "AuthenticateServer");
	if (!asn)
		return -EINVAL;

	switch(asn->present) {
	case AuthenticateServerResponse_PR_authenticateResponseOk:
		res->auth_serv_ok = &asn->choice.authenticateResponseOk;
		break;
	case AuthenticateServerResponse_PR_authenticateResponseError:
		res->auth_serv_err = asn->choice.authenticateResponseError.authenticateErrorCode;
		IPA_LOGP_ES10B("AuthenticateServer", LERROR, "function failed with error code %ld!\n", res->auth_serv_err);
		break;
	default:
		IPA_LOGP_ES10B("AuthenticateServer", LERROR, "unexpected response content!\n");
		res->auth_serv_err  = -1;
	}

	res->res = asn;
	return 0;
}

/*! Send an Authenticate Server request to eUICC.
 *  \param[in] ctx pointer to IPA context.
 *  \param[in] auth_serv_req pointer to struct that holds the request.
 *  \returns struct with parsed AuthenticateServer reponse info on success, NULL on failure. */
struct ipa_es10b_auth_serv_res *ipa_es10b_auth_serv(struct ipa_context *ctx, const struct ipa_es10b_auth_serv_req *req)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_auth_serv_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_auth_serv_res);
	int rc;

	es10b_req = ipa_es10b_req_enc(&asn_DEF_AuthenticateServerRequest, &req->req, "AuthenticateServer");
	if (!es10b_req) {
		IPA_LOGP_ES10B("AuthenticateServer", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10B("AuthenticateServer", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_auth_serv_res(res, es10b_res);
	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return res;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_auth_serv_res_free(res);
	return NULL;
}

/*! Free AuthenticateServer response.
 *  \param[inout] euicc_info pointer to struct that holds the AuthenticateServer response. */
void ipa_es10b_auth_serv_res_free(struct ipa_es10b_auth_serv_res *res)
{
	if (!res)
		return;

	ASN_STRUCT_FREE(asn_DEF_AuthenticateServerResponse, res->res);
	IPA_FREE(res);
}
