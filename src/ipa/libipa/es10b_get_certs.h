#pragma once

#include <GetCertsRequest.h>
#include <GetCertsResponse.h>
#include <Certificate.h>
struct ipa_context;

struct ipa_es10b_get_certs_req {
	struct GetCertsRequest req;
};

struct ipa_es10b_get_certs_res {
	struct GetCertsResponse *res;
	struct Certificate *eum_certificate;
	struct Certificate *euicc_certificate;
	long get_certs_err;
};

struct ipa_es10b_get_certs_res *ipa_es10b_get_certs(struct ipa_context *ctx, const struct ipa_es10b_get_certs_req *req);
void ipa_es10b_get_certs_res_free(struct ipa_es10b_get_certs_res *res);
