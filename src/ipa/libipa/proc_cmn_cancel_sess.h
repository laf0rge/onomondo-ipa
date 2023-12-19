#pragma once

#include <TransactionId.h>
#include <CancelSessionReason.h>
struct ipa_context;

struct ipa_proc_cmn_cancel_sess_pars {
	long reason;
	TransactionId_t transaction_id;
};

int ipa_proc_cmn_cancel_sess(struct ipa_context *ctx, const struct ipa_proc_cmn_cancel_sess_pars *pars);
