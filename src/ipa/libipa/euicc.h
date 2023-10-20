#pragma once

struct ipa_buf;

struct ipa_buf *ipa_euicc_transceive_es10x(struct ipa_context *ctx,
					   const struct ipa_buf *es10x_req);
