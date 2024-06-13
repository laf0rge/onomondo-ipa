#pragma once

#include <GetRatResponse.h>
struct ipa_context;

struct ipa_es10b_get_rat_res {
	struct GetRatResponse *res;
};

struct ipa_es10b_get_rat_res *ipa_es10b_get_rat(struct ipa_context *ctx);
void ipa_es10b_get_rat_res_free(struct ipa_es10b_get_rat_res *res);
