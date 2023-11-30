#pragma once

struct ipa_proc_prfle_inst_pars {
	const BoundProfilePackage_t *bound_profile_package;
};

int ipa_proc_prfle_inst(struct ipa_context *ctx, struct ipa_proc_prfle_inst_pars *pars);
