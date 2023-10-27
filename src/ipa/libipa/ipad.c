#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/scard.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/utils.h>
#include "esipa_init_auth.h"
#include "esipa_get_eim_pkg.h"
#include "context.h"
#include "euicc.h"
#include "es10b_get_euicc_info.h"
#include "es10b_get_euicc_chlg.h"

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg)
{
	struct ipa_context *ctx;
	int rc;

	ctx = IPA_ALLOC(struct ipa_context);
	assert(cfg);
	memset(ctx, 0, sizeof(*ctx));
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

/* A testcase to try out the ES10b function GetEuiccInfo, see also TC_es10b_get_euicc_info */
void testme_get_euicc_info(struct ipa_context *ctx)
{
	struct ipa_es10b_euicc_info *euicc_info;
	euicc_info = ipa_es10b_get_euicc_info(ctx, false);
	ipa_es10b_get_euicc_info_dump(euicc_info, 0, SES10B, LINFO);
	ipa_es10b_get_euicc_info_free(euicc_info);

	euicc_info = ipa_es10b_get_euicc_info(ctx, true);
	ipa_es10b_get_euicc_info_dump(euicc_info, 0, SES10B, LINFO);
	ipa_es10b_get_euicc_info_free(euicc_info);
}

/* A testcase to try out the ES10b function GetEuiccInfo, see also TC_es10b_get_euicc_chlg */
void testme_get_euicc_chlg(struct ipa_context *ctx)
{
	uint8_t euicc_chlg[IPA_LEN_EUICC_CHLG];
	int rc;
	rc = ipa_es10b_get_euicc_chlg(ctx, euicc_chlg);

	printf("============> GOT EUICC CHALLENGE: %s (rc=%d)\n", ipa_hexdump(euicc_chlg, sizeof(euicc_chlg)), rc);
}

/* A testcase to try out the ESipa function GetEimPackage, see also TC_esipa_get_eim_pkg */
void testme_get_eim_pkg(struct ipa_context *ctx)
{
	struct ipa_eim_pkg *eim_pkg;
	eim_pkg =
	    ipa_esipa_get_eim_pkg(ctx, (uint8_t *) "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	ipa_esipa_get_eim_pkg_dump(eim_pkg, 0, SIPA, LINFO);
	ipa_esipa_get_eim_pkg_free(eim_pkg);
}

/* A testcase to try out the ESipa function InitiateAuthentication, see also TC_esipa_init_auth */
void testme_init_auth(struct ipa_context *ctx)
{
	struct ipa_init_auth_req init_auth_req;
	struct ipa_init_auth_res *init_auth_res;
	unsigned int i;
	uint8_t euicc_challenge[IPA_LEN_EUICC_CHLG] =
	    { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
	struct ipa_es10b_euicc_info euicc_info;

	init_auth_req.euicc_challenge = euicc_challenge;

	init_auth_req.smdp_addr_present = true;
	init_auth_req.smdp_addr = "www.example.net";

	memset(&euicc_info, 0, sizeof(euicc_info));
	euicc_info.svn[0] = 0xAA;
	euicc_info.svn[1] = 0xBB;
	euicc_info.svn[2] = 0xCC;

	euicc_info.ci_pkid_verf[euicc_info.ci_pkid_verf_count] = ipa_buf_alloc_and_cpy((uint8_t *) "123", 3);
	euicc_info.ci_pkid_verf_count++;
	euicc_info.ci_pkid_verf[euicc_info.ci_pkid_verf_count] = ipa_buf_alloc_and_cpy((uint8_t *) "456", 3);
	euicc_info.ci_pkid_verf_count++;
	euicc_info.ci_pkid_verf[euicc_info.ci_pkid_verf_count] = ipa_buf_alloc_and_cpy((uint8_t *) "789", 3);
	euicc_info.ci_pkid_verf_count++;

	euicc_info.ci_pkid_sign[euicc_info.ci_pkid_sign_count] = ipa_buf_alloc_and_cpy((uint8_t *) "ABC", 3);
	euicc_info.ci_pkid_sign_count++;
	euicc_info.ci_pkid_sign[euicc_info.ci_pkid_sign_count] = ipa_buf_alloc_and_cpy((uint8_t *) "DEF", 3);
	euicc_info.ci_pkid_sign_count++;
	euicc_info.ci_pkid_sign[euicc_info.ci_pkid_sign_count] = ipa_buf_alloc_and_cpy((uint8_t *) "GHI", 3);
	euicc_info.ci_pkid_sign_count++;

	init_auth_req.euicc_info_present = true;
	init_auth_req.euicc_info = &euicc_info;

	init_auth_res = ipa_esipa_init_auth(ctx, &init_auth_req);

	for (i = 0; i < euicc_info.ci_pkid_verf_count; i++) {
		IPA_FREE(euicc_info.ci_pkid_verf[i]);
	}
	for (i = 0; i < euicc_info.ci_pkid_sign_count; i++) {
		IPA_FREE(euicc_info.ci_pkid_sign[i]);
	}

	ipa_esipa_init_auth_dump_res(init_auth_res, 0, SIPA, LINFO);
	ipa_esipa_init_auth_free_res(init_auth_res);
}

void ipa_poll(struct ipa_context *ctx)
{
//      testme_es10x(ctx);
//      testme_get_euicc_info(ctx);
//      testme_get_euicc_chlg(ctx);
//      testme_get_eim_pkg(ctx);
	testme_init_auth(ctx);
}

void ipa_free_ctx(struct ipa_context *ctx)
{
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
