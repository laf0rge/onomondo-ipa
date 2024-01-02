#pragma once

struct ipa_buf;

#define IPA_EUICC_RESPONSE_BUF_SIZE 5120

struct ipa_buf *ipa_euicc_transceive_es10x(struct ipa_context *ctx, const struct ipa_buf *es10x_req);
int ipa_euicc_init_es10x(struct ipa_context *ctx);
int ipa_euicc_close_es10x(struct ipa_context *ctx);
