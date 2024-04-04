#pragma once

#include <stdbool.h>
struct ipa_buf;

void *ipa_http_init(const char *cabundle, bool no_verif);
int ipa_http_req(void *http_ctx, struct ipa_buf *res, const struct ipa_buf *req, const char *url);
void ipa_http_close(void *http_ctx);
void ipa_http_free(void *http_ctx);
