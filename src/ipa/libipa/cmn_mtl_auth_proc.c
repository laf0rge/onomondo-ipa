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
#include <onomondo/ipa/log.h>
#include <AuthenticateClientRequestEsipa.h>
#include <InitiateAuthenticationResponseEsipa.h>
#include <AuthenticateClientResponseEsipa.h>
#include <AuthenticateServerRequest.h>
#include "context.h"
#include "utils.h"
#include "cmn_mtl_auth_proc.h"
#include "es10b_get_euicc_info.h"
#include "es10b_get_euicc_chlg.h"
#include "esipa_init_auth.h"
#include "es10b_auth_serv.h"
#include "esipa_auth_clnt.h"

/* Walk through the euiccCiPKIdListForVerification list and remove all entries that do not match the given eSIM CA
 * RootCA public key identifier (allowed_ca), See also GSMA SGP.22, section 3.0.1, step 1c. */
static int restrict_euicc_info(struct ipa_es10b_euicc_info *euicc_info, const struct ipa_buf *allowed_ca)
{
	int i;
	struct EUICCInfo1 *asn = euicc_info->euicc_info_1;

	if (!allowed_ca)
		return 0;

	for (i = asn->euiccCiPKIdListForVerification.list.count; i > 0; i--) {
		if (asn->euiccCiPKIdListForVerification.list.array[i - 1]->size != allowed_ca->len
		    || memcmp(asn->euiccCiPKIdListForVerification.list.array[i - 1]->buf, allowed_ca->data,
			      allowed_ca->len)) {
			IPA_FREE(asn->euiccCiPKIdListForVerification.list.array[i - 1]->buf);
			IPA_FREE(asn->euiccCiPKIdListForVerification.list.array[i - 1]);
			asn_sequence_del(&asn->euiccCiPKIdListForVerification, i - 1, 0);
		}
	}

	/* The resulting list must not be empty */
	if (asn->euiccCiPKIdListForVerification.list.count == 0) {
		IPA_LOGP(SMAIN, LERROR, "allowed CA not found in PKI list!\n");
		return -EINVAL;
	}

	return 0;
}

/* Check the certificate for plausibility (see also GSMA SGP.22, section 3.0.1, step #10) */
static int check_certificate(const struct ipa_buf *allowed_ca, const Certificate_t * certificate)
{
	/* In case there is a restriction to a single allowed eSIM CA RootCA (allowed_ca), we need to verify that the
	 * Subject Key Identifier of the certificate matches the allowed CA */
	if (allowed_ca) {
		const asn_oid_arc_t id_ce_subjectKeyIdentifier[4] = { 2, 5, 29, 14 };
		asn_oid_arc_t extension_arcs[256];
		size_t extension_arcs_len;
		const struct Extensions *extensions;
		bool allowed_ca_present = false;
		int i;

		extensions = certificate->tbsCertificate.extensions;
		for (i = 0; i < extensions->list.count; i++) {
			extension_arcs_len =
			    OBJECT_IDENTIFIER_get_arcs(&extensions->list.array[i]->extnID, extension_arcs,
						       IPA_ARRAY_SIZE(extension_arcs));
			if (extension_arcs_len == IPA_ARRAY_SIZE(id_ce_subjectKeyIdentifier)
			    && memcmp(extension_arcs, id_ce_subjectKeyIdentifier,
				      IPA_ARRAY_SIZE(id_ce_subjectKeyIdentifier)) == 0) {
				if (IPA_ASN_STR_CMP_BUF
				    (&extensions->list.array[i]->extnValue, allowed_ca->data, allowed_ca->len))
					allowed_ca_present = true;
			}
		}

		if (!allowed_ca_present) {
			IPA_LOGP(SIPA, LERROR,
				 "the CA (subject key identifier) of the server certificate does not match the allowed CA (%s)!\n",
				 ipa_buf_hexdump(allowed_ca));
			return -EINVAL;
		}
	}

	/* TODO: Verify that the certificate is valid in respect to its time window. This is an optional step but it
	 * may make sense to catch such problems early. */

	return 0;
}

static void gen_ctx_params_1(CtxParams1_t * ctx_params_1, const uint8_t *tac)
{
	assert(ctx_params_1);
	memset(ctx_params_1, 0, sizeof(*ctx_params_1));

	ctx_params_1->present = CtxParams1_PR_ctxParamsForCommonAuthentication;

	/* TODO: This is an optional field, but what is it for?
	 * ctx_params_1->choice.ctxParamsForCommonAuthentication.matchingId = ? */

	IPA_ASSIGN_BUF_TO_ASN(ctx_params_1->choice.ctxParamsForCommonAuthentication.deviceInfo.tac, (uint8_t *) tac, 4);

	/* TODO: the IMEI field is optional, do we need it?
	 * ctx_params_1->choice.ctxParamsForCommonAuthentication.deviceInfo.imei = (8 byte octet string, optional); */

	/* TODO: deviceCapabilities is a mandatory field, but the struct it represents contains only optional
	 * fields. Does that mean that we have to populate at least one of those fields?
	 * ctx_params_1->choice.ctxParamsForCommonAuthentication.deviceInfo.deviceCapabilities... = ?; */
}

int ipa_cmn_mtl_auth_proc(struct ipa_context *ctx, const uint8_t *tac, const struct ipa_buf *allowed_ca,
			  const char *smdp_addr)
{
	struct ipa_es10b_euicc_info *euicc_info = NULL;
	uint8_t euicc_challenge[IPA_LEN_SERV_CHLG];
	struct ipa_esipa_init_auth_req init_auth_req = { 0 };
	struct ipa_esipa_init_auth_res *init_auth_res = NULL;;
	struct ipa_es10b_auth_serv_req auth_serv_req = { 0 };
	struct ipa_es10b_auth_serv_res *auth_serv_res = NULL;
	struct ipa_esipa_auth_clnt_req auth_clnt_req = { 0 };
	struct ipa_esipa_auth_clnt_res *auth_clnt_res = NULL;
	int rc;

	/* Step #1 */
	euicc_info = ipa_es10b_get_euicc_info(ctx, false);
	if (!euicc_info) {
		rc = -EINVAL;
		goto error;
	}
	rc = restrict_euicc_info(euicc_info, allowed_ca);
	if (rc < 0) {
		rc = -EINVAL;
		goto error;
	}

	/* Step #2-#4 */
	rc = ipa_es10b_get_euicc_chlg(ctx, euicc_challenge);
	if (rc < 0) {
		rc = -EINVAL;
		goto error;
	}

	/* Step #5-#9 */
	init_auth_req.euicc_challenge = euicc_challenge;
	init_auth_req.smdp_addr = (char *)smdp_addr;
	init_auth_req.euicc_info_1 = euicc_info->euicc_info_1;
	init_auth_res = ipa_esipa_init_auth(ctx, &init_auth_req);
	if (!init_auth_res) {
		rc = -EINVAL;
		goto error;
	} else if (init_auth_res->init_auth_err) {
		rc = init_auth_res->init_auth_err;
		goto error;
	} else if (!init_auth_res->init_auth_ok) {
		rc = -EINVAL;
		goto error;
	}

	/* Step #10 */
	rc = check_certificate(allowed_ca, &init_auth_res->init_auth_ok->serverCertificate);
	if (rc < 0) {
		rc = -EINVAL;
		goto error;
	}

	/* Step #11-#14 */
	auth_serv_req.req.serverSigned1 = init_auth_res->init_auth_ok->serverSigned1;
	auth_serv_req.req.serverSignature1 = init_auth_res->init_auth_ok->serverSignature1;
	auth_serv_req.req.euiccCiPKIdToBeUsed = init_auth_res->init_auth_ok->euiccCiPKIdToBeUsed;
	auth_serv_req.req.serverCertificate = init_auth_res->init_auth_ok->serverCertificate;
	gen_ctx_params_1(&auth_serv_req.req.ctxParams1, tac);
	auth_serv_res = ipa_es10b_auth_serv(ctx, &auth_serv_req);
	if (!auth_serv_res) {
		rc = -EINVAL;
		goto error;
	}

	/* Step #15-#19 */
	auth_clnt_req.req.transactionId = init_auth_res->init_auth_ok->serverSigned1.transactionId;
	if (auth_serv_res->auth_serv_err) {
		auth_clnt_req.req.authenticateServerResponse.present =
		    SGP32_AuthenticateServerResponse_PR_authenticateResponseError;
		auth_clnt_req.req.authenticateServerResponse.choice.authenticateResponseError.authenticateErrorCode =
		    auth_serv_res->auth_serv_err;
		auth_clnt_req.req.authenticateServerResponse.choice.authenticateResponseError.transactionId =
		    init_auth_res->init_auth_ok->serverSigned1.transactionId;

	} else if (auth_serv_res->auth_serv_ok) {
		auth_clnt_req.req.authenticateServerResponse.present =
		    SGP32_AuthenticateServerResponse_PR_authenticateResponseOk;
		auth_clnt_req.req.authenticateServerResponse.choice.authenticateResponseOk =
		    *auth_serv_res->auth_serv_ok;
	}
	auth_clnt_res = ipa_esipa_auth_clnt(ctx, &auth_clnt_req);
	if (!auth_clnt_res) {
		rc = -EINVAL;
		goto error;
	} else if (auth_clnt_res->auth_clnt_err) {
		rc = auth_clnt_res->auth_clnt_err;
		goto error;
	} else if (!auth_clnt_res->auth_clnt_ok_dpe && !auth_clnt_res->auth_clnt_ok_dse) {
		rc = -EINVAL;
		goto error;
	}

	rc = 0;

error:
	ipa_esipa_init_auth_res_free(init_auth_res);
	ipa_es10b_get_euicc_info_free(euicc_info);
	ipa_es10b_auth_serv_res_free(auth_serv_res);
	ipa_esipa_auth_clnt_res_free(auth_clnt_res);
	return rc;
}
