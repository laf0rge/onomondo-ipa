#pragma once

#include "length.h"

struct ipa_esipa_init_auth_req {
	uint8_t *euicc_challenge;
	char *smdp_addr;
	EUICCInfo1_t *euicc_info_1;
};

struct ipa_esipa_init_auth_res {
	struct EsipaMessageFromEimToIpa *msg_to_ipa;
	struct InitiateAuthenticationOkEsipa *init_auth_ok;
	long init_auth_err;
};

struct ipa_esipa_init_auth_res *ipa_esipa_init_auth(struct ipa_context *ctx, const struct ipa_esipa_init_auth_req *req);
void ipa_esipa_init_auth_res_free(struct ipa_esipa_init_auth_res *res);
