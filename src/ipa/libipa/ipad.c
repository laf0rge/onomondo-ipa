#include <stdio.h>
#include <assert.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/log.h>
#include "context.h"

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg)
{
	struct ipa_context *ctx;
	ctx = IPA_ALLOC(struct ipa_context);

	assert(cfg);
	ctx->cfg = cfg;
	
	ctx->http_ctx = ipa_http_init();
	
	return ctx;
}

void ipa_poll(struct ipa_context *ctx)
{
	return;
}

void ipa_free_ctx(struct ipa_context *ctx)
{
	ipa_http_free(ctx->http_ctx);
	IPA_FREE(ctx);
}
