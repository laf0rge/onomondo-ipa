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
#include "esipa_get_eim_pkg.h"
#include "utils.h"
#include "context.h"
#include "euicc.h"
#include "es10b_get_eid.h"
#include "proc_cmn_mtl_auth.h"
#include "proc_cmn_cancel_sess.h"
#include "proc_direct_prfle_dwnld.h"
#include "proc_euicc_pkg_dwnld_exec.h"

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

	rc = ipa_es10b_get_eid(ctx, ctx->eid);
	if (rc < 0)
		goto error;

	return ctx;
error:
	ipa_free_ctx(ctx);
	return NULL;
}

/* A testcase to try out a full ES10b tranmssion cycle, see also TC_es10x_transceive */
void testme_es10x(struct ipa_context *ctx)
{
	struct ipa_buf *req = ipa_buf_alloc(1024);
	struct ipa_buf *res;

	memset(req->data, 0x41, 300);
	req->data[299] = 0xEE;
	req->len = 300;

	res = ipa_euicc_transceive_es10x(ctx, req);
	if (res)
		printf("============> GOT DATA: %s (%zu bytes)\n", ipa_hexdump(res->data, res->len), res->len);
	else
		printf("============> Error!\n");

	IPA_FREE(req);
	IPA_FREE(res);
}

void ipa_poll(struct ipa_context *ctx)
{
	struct ipa_esipa_get_eim_pkg_res *get_eim_pkg_res = NULL;
	struct ipa_proc_direct_prfle_dwnlod_pars direct_prfle_dwnlod_pars = { 0 };

	/* Poll eIM */
	get_eim_pkg_res = ipa_esipa_get_eim_pkg(ctx, ctx->eid);
	if (!get_eim_pkg_res)
		goto error;
	else if (get_eim_pkg_res->eim_pkg_err)
		goto error;

	/* Relay package contents to suitable handler procedure */
	if (get_eim_pkg_res->euicc_package_request)
		ipa_proc_eucc_pkg_dwnld_exec(ctx, get_eim_pkg_res->euicc_package_request);
	else if (get_eim_pkg_res->ipa_euicc_data_request) {
		/* TODO */
		assert(false);
	} else if (get_eim_pkg_res->dwnld_trigger_request) {
		struct ipa_buf allowed_ca;

		if (!get_eim_pkg_res->dwnld_trigger_request->profileDownloadData) {
			/* TODO: Perhaps there is a way to continue anyway. The ProfileDownloadTriggerRequest still
			 * contains a eimTransactionId, which is also optional. Maybe this eimTransactionId can be
			 * used to retrieve some context from somewhere that may allow us to continue. */
			IPA_LOGP(SIPA, LERROR,
				 "the ProfileDownloadTriggerRequest does not contain ProfileDownloadData -- cannot continue!\n");
			goto error;
		}
		if (get_eim_pkg_res->dwnld_trigger_request->profileDownloadData->present !=
		    ProfileDownloadData_PR_activationCode) {
			/* TODO: Perhaps there is a way to continue anyway. The ProfileDownloadData may alternatively
			 * contain a contactDefaultSmdp flag or an contactSmds information. Maybe we can just ask the
			 * SM-DP/SM-DS for more context information? */
			IPA_LOGP(SIPA, LERROR,
				 "the ProfileDownloadData does not contain an activationCode -- cannot continue!\n");
			goto error;
		}
		ipa_buf_assign(&allowed_ca, ctx->cfg->allowed_ca, IPA_LEN_ALLOWED_CA);
		direct_prfle_dwnlod_pars.allowed_ca = &allowed_ca;
		direct_prfle_dwnlod_pars.tac = ctx->cfg->tac;
		direct_prfle_dwnlod_pars.ac =
		    IPA_STR_FROM_ASN(&get_eim_pkg_res->dwnld_trigger_request->profileDownloadData->choice.
				     activationCode);
		ipa_proc_direct_prfle_dwnlod(ctx, &direct_prfle_dwnlod_pars);
		IPA_FREE((void *)direct_prfle_dwnlod_pars.ac);
	} else {
		IPA_LOGP(SIPA, LERROR,
			 "the GetEimPackageResponse contains an unsupported request -- cannot continue!\n");
		goto error;
	}

error:
	ipa_esipa_get_eim_pkg_free(get_eim_pkg_res);
}

void ipa_free_ctx(struct ipa_context *ctx)
{
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
