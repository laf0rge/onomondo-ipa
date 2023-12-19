#pragma once

#include <CancelSessionRequest.h>
#include <CancelSessionResponseOk.h>

/* GSMA SGP.22, section 5.7.14 */
struct ipa_es10b_cancel_session_req {
	struct CancelSessionRequest req;
};

struct ipa_es10b_cancel_session_res {
	struct CancelSessionResponse *res;
	CancelSessionResponseOk_t *cancel_session_ok;
	long cancel_session_err;
};

struct ipa_es10b_cancel_session_res *ipa_es10b_cancel_session(struct ipa_context *ctx,
							      const struct ipa_es10b_cancel_session_req *req);
void ipa_es10b_cancel_session_res_free(struct ipa_es10b_cancel_session_res *res);
