#pragma once

struct ipa_proc_cmn_cancel_sess_pars {
	long reason;
	const struct ipa_buf *transaction_id;
};

int ipa_proc_cmn_cancel_sess(struct ipa_context *ctx, const struct ipa_proc_cmn_cancel_sess_pars *pars);
