#pragma once

struct ipa_esipa_auth_clnt_res;

struct ipa_esipa_auth_clnt_res *ipa_cmn_mtl_auth_proc(struct ipa_context *ctx, const uint8_t *tac,
						      const struct ipa_buf *allowed_ca, const char *smdp_addr);
