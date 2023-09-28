#pragma once

void *ipa_http_init(void);
int ipa_http_req(void *http_ctx, struct ipa_buf *res, struct ipa_buf *req,
		 char *url);
void ipa_http_free(void *http_ctx);
