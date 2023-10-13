#pragma once

struct ipa_buf;

int ipa_euicc_transceive_es10x(struct ipa_context *ctx,
			       struct ipa_buf *es10x_res,
			       const struct ipa_buf *es10x_req);
