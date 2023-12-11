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
#include <AuthenticateClientRequestEsipa.h>
#include <AuthenticateClientResponseEsipa.h>
#include "esipa_auth_clnt.h"
#include "proc_cmn_mtl_auth.h"
#include "proc_cmn_cancel_sess.h"
#include "proc_direct_prfle_dwnld.h"

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

	printf("============> GOT DATA: %s (%zu bytes)\n", ipa_hexdump(res->data, res->len), res->len);

	IPA_FREE(req);
	IPA_FREE(res);
}

/* A testcase to try out the Common Mutual Authentication Procedure, see also TC_proc_cmn_mtl_auth */
void testme_proc_cmn_mtl_auth(struct ipa_context *ctx)
{
	uint8_t tac[4] = { 0x12, 0x34, 0x56, 0x78 };	/* TODO: Make this a parameter */
	struct ipa_esipa_auth_clnt_res *rc;
	struct ipa_proc_cmn_mtl_auth_pars cmn_mtl_auth_pars = { 0 };

	/* Brainpool */
//	struct ipa_buf *allowed_ca = ipa_buf_alloc_data(20, (uint8_t *) "\xC0\xBC\x70\xBA\x36\x92\x9D\x43\xB4\x67\xFF\x57\x57\x05\x30\xE5\x7A\xB8\xFC\xD8");

	/* NIST */
	struct ipa_buf *allowed_ca = ipa_buf_alloc_data(20, (uint8_t *) "\xF5\x41\x72\xBD\xF9\x8A\x95\xD6\x5C\xBE\xB8\x8A\x38\xA1\xC1\x1D\x80\x0A\x85\xC3");

	cmn_mtl_auth_pars.tac = tac;
	cmn_mtl_auth_pars.allowed_ca = allowed_ca;
	cmn_mtl_auth_pars.smdp_addr = "smdp.example.com";
	rc = ipa_proc_cmn_mtl_auth(ctx, &cmn_mtl_auth_pars);
	if (!rc)
		printf("============> FAILURE!\n");

	ipa_esipa_auth_clnt_res_free(rc);
	IPA_FREE(allowed_ca);
}

/* A testcase to try out the Common Cancel Session Procedure, see also TC_proc_cmn_cancel_sess */
void testme_proc_cmn_cancel_sess(struct ipa_context *ctx)
{
	int rc;
	struct ipa_buf *transaction_id = ipa_buf_alloc_data(3, (uint8_t *) "\xAA\xBB\xCC");
	struct ipa_proc_cmn_cancel_sess_pars cmn_cancel_sess_pars = { 0 };

	cmn_cancel_sess_pars.reason = 5;
	IPA_ASSIGN_IPA_BUF_TO_ASN(cmn_cancel_sess_pars.transaction_id, transaction_id);
	rc = ipa_proc_cmn_cancel_sess(ctx, &cmn_cancel_sess_pars);
	if (rc < 0)
		printf("============> FAILURE!\n");

	IPA_FREE(transaction_id);
}

/* A testcase to try out the Common Mutual Authentication Procedure, see also TC_proc_direct_prfle_dwnld */
void testme_proc_direct_prfle_dwnld(struct ipa_context *ctx)
{
	struct ipa_proc_direct_prfle_dwnlod_pars direct_prfle_dwnlod_pars = { 0 };
	uint8_t tac[4] = { 0x12, 0x34, 0x56, 0x78 };

	/* Brainpool */
//      struct ipa_buf *allowed_ca = ipa_buf_alloc_data(20, (uint8_t *) "\xC0\xBC\x70\xBA\x36\x92\x9D\x43\xB4\x67\xFF\x57\x57\x05\x30\xE5\x7A\xB8\xFC\xD8");
	/* NIST */
	struct ipa_buf *allowed_ca = ipa_buf_alloc_data(20, (uint8_t *) "\xF5\x41\x72\xBD\xF9\x8A\x95\xD6\x5C\xBE\xB8\x8A\x38\xA1\xC1\x1D\x80\x0A\x85\xC3");

	direct_prfle_dwnlod_pars.ac = "1$SMDP.EXAMPLE.COM$04386-AGYFT-A74Y8-3F815";
	direct_prfle_dwnlod_pars.tac = tac;
	direct_prfle_dwnlod_pars.allowed_ca = allowed_ca;
	ipa_proc_direct_prfle_dwnlod(ctx, &direct_prfle_dwnlod_pars);
	IPA_FREE(allowed_ca);

}

void ipa_poll(struct ipa_context *ctx)
{
//      testme_es10x(ctx);
//	testme_proc_cmn_cancel_sess(ctx);
//	testme_proc_cmn_mtl_auth(ctx);
	testme_proc_direct_prfle_dwnld(ctx);
}

void ipa_free_ctx(struct ipa_context *ctx)
{
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
