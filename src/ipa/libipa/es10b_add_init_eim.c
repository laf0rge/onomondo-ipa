/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, 5.9.4: Function (ES10b): AddInitialEim
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
#include "es10x.h"
#include "es10b_add_init_eim.h"

static const struct num_str_map error_code_strings[] = {
	{ AddInitialEimResponse__addInitialEimError_insufficientMemory, "insufficientMemory" },
	{ AddInitialEimResponse__addInitialEimError_unsignedEimConfigDisallowed, "unsignedEimConfigDisallowed" },
	{ AddInitialEimResponse__addInitialEimError_ciPKUnknown, "ciPKUnknown" },
	{ AddInitialEimResponse__addInitialEimError_invalidAssociationToken, "invalidAssociationToken" },
	{ AddInitialEimResponse__addInitialEimError_counterValueOutOfRange, "counterValueOutOfRange" },
	{ AddInitialEimResponse__addInitialEimError_undefinedError, "undefinedError" },
	{ 0, NULL }
};

static int dec_add_init_eim_res(struct ipa_es10b_add_init_eim_res *res, const struct ipa_buf *es10b_res)
{
	struct AddInitialEimResponse *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_AddInitialEimResponse, es10b_res, "AddInitialEim");
	if (!asn)
		return -EINVAL;

	switch (asn->present) {
	case AddInitialEimResponse_PR_addInitialEimOk:
		/* When we see this list, we can be sure that the configuration was accepted. */
		break;
	case AddInitialEimResponse_PR_addInitialEimError:
		res->add_init_eim_err = asn->choice.addInitialEimError;
		IPA_LOGP_ES10X("AddInitialEim", LERROR, "function failed with error code %ld=%s!\n",
			       res->add_init_eim_err, ipa_str_from_num(error_code_strings, res->add_init_eim_err,
								       "(unknown)"));
		break;
	default:
		IPA_LOGP_ES10X("AddInitialEim", LERROR, "unexpected response content!\n");
		res->add_init_eim_err = -1;
	}

	res->res = asn;
	return 0;
}

static struct ipa_es10b_add_init_eim_res *add_init_eim(struct ipa_context *ctx,
						       const struct ipa_es10b_add_init_eim_req *req)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_add_init_eim_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_add_init_eim_res);
	int rc;

	es10b_req = ipa_es10x_req_enc(&asn_DEF_AddInitialEimRequest, &req->req, "AddInitialEim");
	if (!es10b_req) {
		IPA_LOGP_ES10X("AddInitialEim", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10X("AddInitialEim", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_add_init_eim_res(res, es10b_res);
	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return res;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_add_init_eim_res_free(res);
	return NULL;
}

static struct ipa_es10b_add_init_eim_res *add_init_eim_iot_emu(struct ipa_context *ctx,
							       const struct ipa_es10b_add_init_eim_req *req)
{
	struct ipa_buf *eim_cfg_new = NULL;
	struct ipa_es10b_add_init_eim_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_add_init_eim_res);
	int rc;

	/* TODO: validate (and modify if necessary) the input data as described in SGP.32, section 5.9.4 */

	IPA_LOGP_ES10X("AddInitialEim", LINFO,
		       "IoT eUICC emulation active, pretending to query eUICC to set eIM configuration...\n");

	eim_cfg_new = ipa_es10x_req_enc(&asn_DEF_AddInitialEimRequest, &req->req, "AddInitialEim");
	if (!eim_cfg_new) {
		IPA_LOGP_ES10X("AddInitialEim", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	/* AddInitialEimRequest and GetEimConfigurationDataResponse are identical. This means we can cast
	 * AddInitialEimRequest encoded ASN.1 data to GetEimConfigurationDataResponse */
	eim_cfg_new->data[1] = 0x55;

	/* Replace the current eIM configuration with the new eIM configuration */
	IPA_FREE(ctx->iot_euicc_emu.eim_cfg_ber);
	ctx->iot_euicc_emu.eim_cfg_ber = eim_cfg_new;
	IPA_LOGP_ES10X("AddInitialEim", LINFO, "done, eIM configuration stored in memory.\n");

	/* TODO: fill in an appropriate response */
	return res;
error:
	IPA_FREE(eim_cfg_new);
	ipa_es10b_add_init_eim_res_free(res);
	return NULL;
}

/*! Function (ES10b): AddInitialEim.
 *  \param[inout] ctx pointer to ipa_context.
 *  \param[in] req pointer to struct that holds the function parameters.
 *  \returns pointer newly allocated struct with function result, NULL on error. */
struct ipa_es10b_add_init_eim_res *ipa_es10b_add_init_eim(struct ipa_context *ctx,
							  const struct ipa_es10b_add_init_eim_req *req)
{
	if (ctx->cfg->iot_euicc_emu.enabled)
		return add_init_eim_iot_emu(ctx, req);
	else
		return add_init_eim(ctx, req);
}

/*! Free results of function (ES10b): AddInitialEim.
 *  \param[in] res pointer to function result. */
void ipa_es10b_add_init_eim_res_free(struct ipa_es10b_add_init_eim_res *res)
{
	IPA_ES10X_RES_FREE(asn_DEF_AddInitialEimResponse, res);
}
