/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/scard.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/utils.h>
#include "utils.h"
#include "context.h"
#include "euicc.h"
#include "es10c_get_eid.h"
#include "proc_eim_pkg_retr.h"

#include "es10b_get_eim_cfg_data.h"

static int equip_eim_cfg(struct ipa_context *ctx, struct ipa_config *cfg)
{
	struct ipa_es10b_eim_cfg_data *eim_cfg_data = NULL;
	struct EimConfigurationData *eim_cfg_data_item = NULL;

	eim_cfg_data = ipa_es10b_get_eim_cfg_data(ctx);
	if (!eim_cfg_data)
		IPA_LOGP(SIPA, LERROR, "cannot read EimConfigurationData from eUICC (consumer eUICC?)\n");
	else
		eim_cfg_data_item = ipa_es10b_get_eim_cfg_data_filter(eim_cfg_data, ctx->cfg->preferred_eim_id);

	if (!eim_cfg_data_item) {
		/* If the EimConfigurationData cannto be read for whatever reason, the user has the option to configure
		 * fallback values for the minium required fields of EimConfigurationData. Those values will the be
		 * used to substitute the missing data. */
		if (!ctx->cfg->fallback_eim_id) {
			IPA_LOGP(SIPA, LERROR, "no fallback eimId configured!\n");
			goto error;
		}
		if (!ctx->cfg->fallback_eim_fqdn) {
			IPA_LOGP(SIPA, LERROR, "no fallback eimFqdn configured!\n");
			goto error;
		}

		IPA_LOGP(SIPA, LINFO,
			 "using (optional) fallback values from config to substitute the missing EimConfigurationData!\n");
		ctx->eim_id = IPA_ALLOC_N(strlen(ctx->cfg->fallback_eim_id) + 1);
		strcpy(ctx->eim_id, ctx->cfg->fallback_eim_id);
		ctx->eim_fqdn = IPA_ALLOC_N(strlen(ctx->cfg->fallback_eim_fqdn) + 1);
		strcpy(ctx->eim_fqdn, ctx->cfg->fallback_eim_fqdn);
		if (ctx->cfg->fallback_euicc_ci_pkid)
			ctx->euicc_ci_pkid = ipa_buf_dup(ctx->cfg->fallback_euicc_ci_pkid);
	} else {
		ctx->eim_id = IPA_STR_FROM_ASN(&eim_cfg_data_item->eimId);
		ctx->eim_fqdn = IPA_STR_FROM_ASN(eim_cfg_data_item->eimFqdn);
		ctx->euicc_ci_pkid = IPA_BUF_FROM_ASN(eim_cfg_data_item->euiccCiPKId);
	}

	/* Make sure we have the minimum eIM information available */
	if (!ctx->eim_id)
		goto error;
	if (!ctx->eim_fqdn)
		goto error;

	ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);

	return 0;
error:
	IPA_LOGP(SIPA, LERROR, "unable to retrieve EimConfigurationData\n");
	ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);
	return -EINVAL;
}

/*! Create a new ipa_context and prepare links towards eIM and eUICC.
 *  \param[in] cfg IPAd configuration.
 *  \returns ipa_context on success, NULL on failure. */
struct ipa_context *ipa_new_ctx(struct ipa_config *cfg)
{
	struct ipa_context *ctx;
	int rc;

	ctx = IPA_ALLOC_ZERO(struct ipa_context);
	ctx->cfg = cfg;

	ctx->http_ctx = ipa_http_init();
	if (!ctx->http_ctx)
		goto error;

	ctx->scard_ctx = ipa_scard_init(cfg->reader_num);
	if (!ctx->scard_ctx)
		goto error;

	rc = ipa_euicc_init_es10x(ctx);
	if (rc < 0)
		goto error;

	rc = ipa_es10c_get_eid(ctx, ctx->eid);
	if (rc < 0)
		goto error;

	rc = equip_eim_cfg(ctx, cfg);
	if (rc < 0)
		goto error;

	return ctx;
error:
	ipa_free_ctx(ctx);
	return NULL;
}

/*! poll the IPAd (may be called in regular intervals or on purpose).
 *  \param[inout] ctx pointer to ipa_context.
 *  \returns 0 on success, negative on error. */
int ipa_poll(struct ipa_context *ctx)
{
	return ipa_proc_eim_pkg_retr(ctx);
}

/*! close links towards eIM and eUICC and free an ipa_context.
 *  \param[inout] ctx pointer to ipa_context. */
void ipa_free_ctx(struct ipa_context *ctx)
{
	if (!ctx)
		return;

	IPA_FREE(ctx->eim_id);
	IPA_FREE(ctx->eim_fqdn);
	IPA_FREE(ctx->euicc_ci_pkid);

	if (ctx->scard_ctx)
		ipa_euicc_close_es10x(ctx);
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
