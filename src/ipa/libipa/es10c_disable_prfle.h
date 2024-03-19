#pragma once

#include <DisableProfileRequest.h>
#include <DisableProfileResponse.h>
struct ipa_context;

struct ipa_es10c_disable_prfle_req {
	struct DisableProfileRequest req;
};

struct ipa_es10c_disable_prfle_res {
	struct DisableProfileResponse *res;
};

struct ipa_es10c_disable_prfle_res *ipa_es10c_disable_prfle(struct ipa_context *ctx,
							    const struct ipa_es10c_disable_prfle_req *req);
void ipa_es10c_disable_prfle_res_free(struct ipa_es10c_disable_prfle_res *res);
