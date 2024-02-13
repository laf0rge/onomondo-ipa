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

static int equip_eim_cfg(struct ipa_context *ctx)
{
	struct ipa_es10b_eim_cfg_data *eim_cfg_data = NULL;
	struct EimConfigurationData *eim_cfg_data_item = NULL;

	eim_cfg_data = ipa_es10b_get_eim_cfg_data(ctx);
	if (!eim_cfg_data) {
		IPA_LOGP(SIPA, LERROR, "cannot read EimConfigurationData from eUICC\n");
		goto error;
	}

	/* In case no preferred_eim_id is set, the first eIM configuration item will be pulled from the list */
	eim_cfg_data_item = ipa_es10b_get_eim_cfg_data_filter(eim_cfg_data, ctx->cfg->preferred_eim_id);
	if (!eim_cfg_data_item) {
		IPA_LOGP(SIPA, LERROR, "no suitable EimConfigurationData item present.\n");
		goto error;
	}

	ctx->eim_id = IPA_STR_FROM_ASN(&eim_cfg_data_item->eimId);
	if (!ctx->eim_id)
		goto error;

	if (eim_cfg_data_item->eimFqdn)
		ctx->eim_fqdn = IPA_STR_FROM_ASN(eim_cfg_data_item->eimFqdn);
	else
		goto error;

	/* TODO: We do not need this parameter very often. Maybe we can get rid of it by querying the eIM
	 * configuration when we need it. */
	if (eim_cfg_data_item->euiccCiPKId)
		ctx->euicc_ci_pkid = IPA_BUF_FROM_ASN(eim_cfg_data_item->euiccCiPKId);

	ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);

	return 0;
error:
	IPA_LOGP(SIPA, LERROR, "unable to retrieve EimConfigurationData\n");
	ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);
	return -EINVAL;
}

/*! Create a new ipa_context.
 *  \param[in] cfg IPAd configuration.
 *  \returns ipa_context on success, NULL on failure. */
struct ipa_context *ipa_new_ctx(struct ipa_config *cfg)
{
	struct ipa_context *ctx;
	int rc;

	ctx = IPA_ALLOC_ZERO(struct ipa_context);
	assert(ctx);

	ctx->cfg = cfg;

	if (cfg->iot_euicc_emu.enabled) {
		assert(cfg->iot_euicc_emu.eim_cfg_ber);
		ctx->iot_euicc_emu.eim_cfg_ber = ipa_buf_dup(cfg->iot_euicc_emu.eim_cfg_ber);
	}

	return ctx;
}

/*! Initialize IPAd and prepare links towards eIM and eUICC.
 *  \param[inout] ctx pointer to ipa_context.
 *  \returns 0 success, -EINVAL on failure. */
int ipa_init(struct ipa_context *ctx)
{
	int rc;

	ctx->http_ctx = ipa_http_init();
	if (!ctx->http_ctx)
		return -EINVAL;

	ctx->scard_ctx = ipa_scard_init(ctx->cfg->reader_num);
	if (!ctx->scard_ctx)
		return -EINVAL;

	rc = ipa_euicc_init_es10x(ctx);
	if (rc < 0)
		return -EINVAL;

	rc = ipa_es10c_get_eid(ctx, ctx->eid);
	if (rc < 0)
		return -EINVAL;

	rc = equip_eim_cfg(ctx);
	if (rc < 0)
		return -EINVAL;

	return 0;
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
	IPA_FREE(ctx->iot_euicc_emu.eim_cfg_ber);

	if (ctx->scard_ctx)
		ipa_euicc_close_es10x(ctx);
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
