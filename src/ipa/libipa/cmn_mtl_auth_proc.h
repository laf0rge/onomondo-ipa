#pragma once

int ipa_cmn_mtl_auth_proc(struct ipa_context *ctx, const uint8_t *tac, const struct ipa_buf *allowed_ca,
			  const char *smdp_addr);
