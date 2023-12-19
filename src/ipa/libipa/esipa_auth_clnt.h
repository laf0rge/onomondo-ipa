#pragma once

#include <AuthenticateClientRequestEsipa.h>
#include <EsipaMessageFromEimToIpa.h>
#include <TransactionId.h>
#include <AuthenticateClientOkDPEsipa.h>
#include <AuthenticateClientOkDSEsipa.h>
struct ipa_context;

struct ipa_esipa_auth_clnt_req {
	struct AuthenticateClientRequestEsipa req;
};

struct ipa_esipa_auth_clnt_res {
	struct EsipaMessageFromEimToIpa *msg_to_ipa;
	TransactionId_t *transaction_id;
	AuthenticateClientOkDPEsipa_t *auth_clnt_ok_dpe;
	AuthenticateClientOkDSEsipa_t *auth_clnt_ok_dse;
	long auth_clnt_err;
};

struct ipa_esipa_auth_clnt_res *ipa_esipa_auth_clnt(struct ipa_context *ctx, const struct ipa_esipa_auth_clnt_req *req);
void ipa_esipa_auth_clnt_res_free(struct ipa_esipa_auth_clnt_res *res);
