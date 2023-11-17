#pragma once

struct ipa_proc_direct_prfle_dwnlod_pars {
	const char *ac;
	const uint8_t *tac;
	const struct ipa_buf *allowed_ca;
};

int ipa_proc_direct_prfle_dwnlod(struct ipa_context *ctx, struct ipa_proc_direct_prfle_dwnlod_pars *pars);
