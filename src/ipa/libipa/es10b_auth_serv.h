#pragma once

#include <AuthenticateServerRequest.h>

/* GSMA SGP.22, section 5.7.13 */
struct ipa_es10b_auth_serv_req {
	struct AuthenticateServerRequest req;
};

struct ipa_es10b_auth_serv_res {
	struct AuthenticateServerResponse *res;
	AuthenticateResponseOk_t *auth_serv_ok;
	long auth_serv_err;
};

struct ipa_es10b_auth_serv_res *ipa_es10b_auth_serv(struct ipa_context *ctx, const struct ipa_es10b_auth_serv_req *req);
void ipa_es10b_auth_serv_res_free(struct ipa_es10b_auth_serv_res *res);
