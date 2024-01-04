/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, 5.9.18: Function (ES10b): GetEimConfigurationData
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
#include <GetEimConfigurationDataRequest.h>
#include "context.h"
#include "utils.h"
#include "euicc.h"
#include "es10x.h"
#include "es10b_get_eim_cfg_data.h"

static int dec_get_eim_cfg_data(struct ipa_es10b_eim_cfg_data *eim_cfg_data, const struct ipa_buf *es10a_res)
{
	struct GetEimConfigurationDataResponse *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_GetEimConfigurationDataResponse, es10a_res, "GetEimConfigurationData");
	if (!asn)
		return -EINVAL;

	eim_cfg_data->res = asn;
	return 0;
}

/*! Function (ES10b): GetEimConfigurationData.
 *  \param[inout] ctx pointer to ipa_context.
 *  \returns pointer newly allocated struct with function result, NULL on error. */
struct ipa_es10b_eim_cfg_data *ipa_es10b_get_eim_cfg_data(struct ipa_context *ctx)
{
	struct ipa_buf *es10a_req = NULL;
	struct ipa_buf *es10a_res = NULL;
	struct ipa_es10b_eim_cfg_data *eim_cfg_data = IPA_ALLOC_ZERO(struct ipa_es10b_eim_cfg_data);
	struct GetEimConfigurationDataRequest get_eim_cfg_data_req = { 0 };
	int rc;

	es10a_req =
	    ipa_es10x_req_enc(&asn_DEF_GetEimConfigurationDataRequest, &get_eim_cfg_data_req,
			      "GetEimConfigurationData");
	if (!es10a_req) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10a_res = ipa_euicc_transceive_es10x(ctx, es10a_req);
	if (!es10a_res) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_eim_cfg_data(eim_cfg_data, es10a_res);

	if (rc < 0)
		goto error;

	IPA_FREE(es10a_req);
	IPA_FREE(es10a_res);
	return eim_cfg_data;
error:
	IPA_FREE(es10a_req);
	IPA_FREE(es10a_res);
	ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);
	return NULL;
}

/*! Free results of function (ES10b): GetEimConfigurationData.
 *  \param[in] res pointer to function result. */
void ipa_es10b_get_eim_cfg_data_free(struct ipa_es10b_eim_cfg_data *res)
{
	IPA_ES10X_RES_FREE(asn_DEF_GetEimConfigurationDataResponse, res);
}

/*! Filter one EimConfigurationData list item from GetEimConfigurationDataResponse.
 *  \param[in] res pointer to function result that contains the GetEimConfigurationDataResponse.
 *  \param[in] eim_id eimId to look for. When set to NULL, the first item of the EimConfigurationData list is returned.
 *  \returns pointer to EimConfigurationData item, NULL on error. */
struct EimConfigurationData *ipa_es10b_get_eim_cfg_data_filter(struct ipa_es10b_eim_cfg_data *res, char *eim_id)
{
	unsigned int i;

	if (!res || !res->res) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR,
			       "cannot filter non existent EimConfigurationData list\n");
		return NULL;
	}

	if (res->res->eimConfigurationDataList.list.count < 1) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "cannot filter empty EimConfigurationData list\n");
		return NULL;
	}

	/* In case no eim_id is specified, just pick the first item from the list */
	if (!eim_id || eim_id[0] == '\0') {
		return res->res->eimConfigurationDataList.list.array[0];
	}

	for (i = 0; i < res->res->eimConfigurationDataList.list.count; i++) {
		if (IPA_ASN_STR_CMP_BUF
		    (&res->res->eimConfigurationDataList.list.array[i]->eimId, eim_id,
		     strlen(eim_id))) {
			return res->res->eimConfigurationDataList.list.array[i];
		}
	}

	IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "cannot find eimId %s in EimConfigurationData list\n",
		       eim_id);
	return NULL;
}
