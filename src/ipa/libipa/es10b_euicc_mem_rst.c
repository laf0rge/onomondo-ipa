/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.22, 5.7.19: Function (ES10b): eUICCMemoryReset
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
#include <EuiccMemoryResetRequest.h>
#include <EuiccMemoryResetResponse.h>
#include "context.h"
#include "utils.h"
#include "euicc.h"
#include "es10x.h"
#include "es10b_euicc_mem_rst.h"

static const struct num_str_map error_code_strings[] = {
	{ EuiccMemoryResetResponse__resetResult_ok, "ok" },
	{ EuiccMemoryResetResponse__resetResult_nothingToDelete, "nothingToDelete" },
	{ EuiccMemoryResetResponse__resetResult_catBusy, "catBusy" },
	{ EuiccMemoryResetResponse__resetResult_undefinedError, "undefinedError" },
	{ 0, NULL }
};

static int dec_euicc_mem_rst_res(const struct ipa_buf *es10b_res)
{
	struct EuiccMemoryResetResponse *asn = NULL;
	int rc;

	asn = ipa_es10x_res_dec(&asn_DEF_EuiccMemoryResetResponse, es10b_res, "eUICCMemoryReset");
	if (!asn)
		return -EINVAL;

	if (asn->resetResult != EuiccMemoryResetResponse__resetResult_ok &&
	    asn->resetResult != EuiccMemoryResetResponse__resetResult_nothingToDelete) {
		IPA_LOGP_ES10X("eUICCMemoryReset", LERROR, "function failed with error code %ld=%s!\n",
			       asn->resetResult, ipa_str_from_num(error_code_strings, asn->resetResult, "(unknown)"));
		rc = -EINVAL;
	} else {
		IPA_LOGP_ES10X("eUICCMemoryReset", LERROR, "function succeeded with status code %ld=%s!\n",
			       asn->resetResult, ipa_str_from_num(error_code_strings, asn->resetResult, "(unknown)"));
	}

	ASN_STRUCT_FREE(asn_DEF_EuiccMemoryResetResponse, asn);
	return rc;
}

/*! Function (ES10b): eUICCMemoryReset.
 *  \param[inout] ctx pointer to ipa_context.
 *  \param[in] req pointer to struct that holds the function parameters.
 *  \returns 0 on success, negative on error. */
int ipa_es10b_euicc_mem_rst(struct ipa_context *ctx, const struct ipa_es10b_euicc_mem_rst *req)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct EuiccMemoryResetRequest mem_rst_req = { 0 };
	int rc = -EINVAL;
	uint8_t rst_opt = 0;
	uint8_t empty_eim_cfg[] = { 0xBF, 0x55, 0x02, 0xA0, 0x00 };

	mem_rst_req.resetOptions.buf = &rst_opt;
	mem_rst_req.resetOptions.size = 1;
	mem_rst_req.resetOptions.bits_unused = 6;

	if (req->operatnl_profiles)
		rst_opt |= (1 << EuiccMemoryResetRequest__resetOptions_deleteOperationalProfiles);
	if (req->test_profiles)
		rst_opt |= (1 << EuiccMemoryResetRequest__resetOptions_deleteFieldLoadedTestProfiles);
	if (req->default_smdp_addr)
		rst_opt |= (1 << EuiccMemoryResetRequest__resetOptions_resetDefaultSmdpAddress);

	/* TODO: It is a bit unclear how the eIM configuration should be treated when the eUICC memory is reset.
	 * in SGP.22 v2.5, the specification we use, there is no extra bit for this in resetOptions. But in
	 * SGP.32 v1.0.1, there is resetEimConfigData(3), which is ambiguously assigned since in SGP.22 v3.1
	 * the same bit means deletePreLoadedTestProfiles(3). This is something that should be closer
	 * investigated and perhaps confirmed with real sample consumer and IoT eUICCs
	 * (The same problem also exists with resetAutoEnableConfig(4) and deleteProvisioningProfiles(4)) */
	if (ctx->cfg->iot_euicc_emu_enabled) {
		IPA_LOGP_ES10X("eUICCMemoryReset", LINFO,
			       "IoT eUICC emulation active, also clearing memory with eIM configuration...\n");
		IPA_FREE(ctx->nvstate.iot_euicc_emu.eim_cfg_ber);
		ctx->nvstate.iot_euicc_emu.eim_cfg_ber = ipa_buf_alloc_data(sizeof(empty_eim_cfg), empty_eim_cfg);
	} else {
		/* Set resetEimConfigData(3), see also TODO above */
		rst_opt |= (1 << 3);
	}

	es10b_req = ipa_es10x_req_enc(&asn_DEF_EuiccMemoryResetRequest, &mem_rst_req, "eUICCMemoryReset");
	if (!es10b_req) {
		IPA_LOGP_ES10X("eUICCMemoryReset", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10X("eUICCMemoryReset", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_euicc_mem_rst_res(es10b_res);
	if (rc < 0)
		goto error;

error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return rc;
}
