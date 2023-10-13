#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/scard.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/utils.h>
#include "eim_package.h"
#include "context.h"
#include "euicc.h"

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg)
{
	struct ipa_context *ctx;
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

	return ctx;
error:
	ipa_free_ctx(ctx);
	return NULL;
}

/* A testcase to try out a full ES10b tranmssion cycle, see also TC_es10b_transceive */
void testme_es10b(struct ipa_context *ctx)
{
	struct ipa_buf *req = ipa_buf_alloc(1024);
	struct ipa_buf *res = ipa_buf_alloc(1024);

	memset(req->data, 0x41, 300);
	req->data[299] = 0xEE;
	req->len = 300;

	ipa_euicc_transceive_es10x(ctx, res, req);

	printf("============> GOT DATA: %s (%zu bytes)\n",
	       ipa_hexdump(res->data, res->len), res->len);

	IPA_FREE(req);
	IPA_FREE(res);
}

void ipa_poll(struct ipa_context *ctx)
{
#if 0
	struct ipa_eim_package *eim_package;
	eim_package = ipa_eim_package_fetch(ctx, (uint8_t*)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	ipa_eim_package_dump(eim_package, 0, SIPA, LINFO);
	ipa_eim_package_free(eim_package);
#endif

	testme_es10b(ctx);
}

void ipa_free_ctx(struct ipa_context *ctx)
{
	ipa_http_free(ctx->http_ctx);
	ipa_scard_free(ctx->scard_ctx);
	IPA_FREE(ctx);
}
