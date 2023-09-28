#pragma once

void *ipa_scard_init(unsigned int reader_num);
int ipa_scard_reset(void *scard_ctx);
int ipa_scard_atr(void *scard_ctx, struct ipa_buf *atr);
int ipa_scard_transceive(void *scard_ctx, struct ipa_buf *res,
			 struct ipa_buf *req);
int ipa_scard_free(void *scard_ctx);
