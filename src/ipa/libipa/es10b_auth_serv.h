#pragma once

#include "length.h"
#include "es10b.h"
#include <stdbool.h>
#include <AuthenticateServerRequest.h>

/* GSMA SGP.22, section 5.7.13 */
struct ipa_es10b_auth_serv_req {
	struct AuthenticateServerRequest req;
};

struct ipa_es10b_auth_serv_res {
	struct AuthenticateServerResponse *res;
};

struct ipa_es10b_auth_serv_res *ipa_es10b_auth_serv(struct ipa_context *ctx,
						    const struct ipa_es10b_auth_serv_req *auth_serv_req);
void ipa_es10b_auth_serv_res_free(struct ipa_es10b_auth_serv_res *auth_serv_res);
