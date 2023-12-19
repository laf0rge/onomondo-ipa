#pragma once

#include <stdbool.h>
#include <CancelSessionRequestEsipa.h>

struct ipa_esipa_cancel_session_req {
	TransactionId_t *transaction_id;
	CancelSessionResponseOk_t *cancel_session_ok;
	long cancel_session_err;
};

struct ipa_esipa_cancel_session_res {
	struct EsipaMessageFromEimToIpa *msg_to_ipa;
	bool cancel_session_ok;
	long cancel_session_err;
};

struct ipa_esipa_cancel_session_res *ipa_esipa_cancel_session(struct ipa_context *ctx,
							      const struct ipa_esipa_cancel_session_req *req);
void ipa_esipa_cancel_session_res_free(struct ipa_esipa_cancel_session_res *res);
