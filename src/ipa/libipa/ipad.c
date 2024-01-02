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

	return ctx;
error:
	ipa_free_ctx(ctx);
	return NULL;
}

/*! poll the IPAd (may be called in regular intervals or on purpose).
 *  \param[inout] ctx pointer to ipa_context. */
void ipa_poll(struct ipa_context *ctx)
{
	ipa_proc_eim_pkg_retr(ctx);
}

/*! close links towards eIM and eUICC and free an ipa_context.
 *  \param[inout] ctx pointer to ipa_context. */
void ipa_free_ctx(struct ipa_context *ctx)
{
	if (!ctx)
		return;
	if (ctx->scard_ctx)
		ipa_euicc_close_es10x(ctx);
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
