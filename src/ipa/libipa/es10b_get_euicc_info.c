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
#include "es10b_get_euicc_info.h"
#include <GetEuiccInfo1Request.h>
#include <GetEuiccInfo2Request.h>
#include <EUICCInfo1.h>
#include <EUICCInfo2.h>

static int dec_get_euicc_info1(struct ipa_es10b_euicc_info *euicc_info, struct ipa_buf *es10b_res)
{
	struct EUICCInfo1 *asn = NULL;

	asn = ipa_es10b_res_dec(&asn_DEF_EUICCInfo1, es10b_res, "GetEuiccInfo1Request");
	if (!asn)
		return -EINVAL;

	euicc_info->euicc_info_1 = asn;
	return 0;
}

struct ipa_es10b_euicc_info *get_euicc_info1(struct ipa_context *ctx)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_euicc_info *euicc_info;
	struct GetEuiccInfo1Request get_euicc_info1_req = { 0 };
	int rc;

	euicc_info = IPA_ALLOC(struct ipa_es10b_euicc_info);
	memset(euicc_info, 0, sizeof(*euicc_info));

	/* Request minimal set of the eUICC information */
	es10b_req = ipa_es10b_req_enc(&asn_DEF_GetEuiccInfo1Request, &get_euicc_info1_req, "GetEuiccInfo1Request");
	if (!es10b_req) {
		IPA_LOGP_ES10B("GetEuiccInfo1Request", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10B("GetEuiccInfo1Request", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_euicc_info1(euicc_info, es10b_res);

	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return euicc_info;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_get_euicc_info_free(euicc_info);
	return NULL;
}

static int dec_get_euicc_info2(struct ipa_es10b_euicc_info *euicc_info, struct ipa_buf *es10b_res)
{
	struct EUICCInfo2 *asn = NULL;

	asn = ipa_es10b_res_dec(&asn_DEF_EUICCInfo2, es10b_res, "GetEuiccInfo2Request");
	if (!asn)
		return -EINVAL;

	euicc_info->euicc_info_2 = asn;
	return 0;
}

struct ipa_es10b_euicc_info *get_euicc_info2(struct ipa_context *ctx)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_euicc_info *euicc_info;
	struct GetEuiccInfo1Request get_euicc_info2_req = { 0 };
	int rc;

	euicc_info = IPA_ALLOC(struct ipa_es10b_euicc_info);
	memset(euicc_info, 0, sizeof(*euicc_info));

	/* Request full set of the eUICC information */
	es10b_req = ipa_es10b_req_enc(&asn_DEF_GetEuiccInfo2Request, &get_euicc_info2_req, "GetEuiccInfo2Request");
	if (!es10b_req) {
		IPA_LOGP_ES10B("GetEuiccInfo2Request", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10B("GetEuiccInfo2Request", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_euicc_info2(euicc_info, es10b_res);

	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return euicc_info;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_get_euicc_info_free(euicc_info);
	return NULL;
}

/*! Request eUICC info from eUICC.
 *  \param[in] ctx pointer to IPA context.
 *  \param[in] full set to true to request EUICCInfo2 instead of EUICCInfo1.
 *  \returns struct with parsed eUICC info on success, NULL on failure. */
struct ipa_es10b_euicc_info *ipa_es10b_get_euicc_info(struct ipa_context *ctx, bool full)
{
	if (full)
		return get_euicc_info2(ctx);
	else
		return get_euicc_info1(ctx);
}

/*! Free eUICC info.
 *  \param[inout] euicc_info pointer to struct that holds the eUICC info. */
void ipa_es10b_get_euicc_info_free(struct ipa_es10b_euicc_info *euicc_info)
{
	if (!euicc_info)
		return;

	ASN_STRUCT_FREE(asn_DEF_EUICCInfo1, euicc_info->euicc_info_1);
	ASN_STRUCT_FREE(asn_DEF_EUICCInfo2, euicc_info->euicc_info_2);

	IPA_FREE(euicc_info);
}
