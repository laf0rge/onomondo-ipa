#pragma once

#include <ProfileInfoListRequest.h>
#include <ProfileInfoListResponse.h>
#include <SGP32-ProfileInfoListResponse.h>
struct ipa_context;

struct ipa_es10c_get_prfle_info_req {
	struct ProfileInfoListRequest req;
};

struct ipa_es10c_get_prfle_info_res {
	struct ProfileInfoListResponse *res;
	struct SGP32_ProfileInfoListResponse *sgp32_res;
	struct ProfileInfoListResponse__profileInfoListOk *prfle_info_lst;
	struct ProfileInfo *currently_active_prfle;
	long prfle_info_list_err;
};

struct ipa_es10c_get_prfle_info_res *ipa_es10c_get_prfle_info(struct ipa_context *ctx,
							      const struct ipa_es10c_get_prfle_info_req *req);
void ipa_es10c_get_prfle_info_res_free(struct ipa_es10c_get_prfle_info_res *res);
