#pragma once

#include <ProfileRollbackRequest.h>
#include <ProfileRollbackResponse.h>
struct ipa_context;

struct ipa_es10b_prfle_rollback_res {
	struct ProfileRollbackResponse *res;
};

struct ipa_es10b_prfle_rollback_res *ipa_es10b_prfle_rollback(struct ipa_context *ctx, bool refresh_flag);
void ipa_es10b_prfle_rollback_res_free(struct ipa_es10b_prfle_rollback_res *res);
