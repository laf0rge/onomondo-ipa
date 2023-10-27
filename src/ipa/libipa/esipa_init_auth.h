#pragma once

#include "length.h"

struct ipa_init_auth_req {
	uint8_t *euicc_challenge;

	bool smdp_addr_present;
	char *smdp_addr;

	bool euicc_info_present;
	struct ipa_es10b_euicc_info *euicc_info;
};

struct ipa_init_auth_res {
	/* Pointer to root of the decoded struct */
	struct EsipaMessageFromEimToIpa *msg_to_ipa;

	/* Pointer to initiate authentication result (null on error) */
	struct InitiateAuthenticationOkEsipa *init_auth_ok;

	/* Error code (in case init_auth_ok is null) */
	long init_auth_err;
};

struct ipa_init_auth_res *ipa_esipa_init_auth(struct ipa_context *ctx, struct ipa_init_auth_req *init_auth_req);
void ipa_esipa_init_auth_dump_res(struct ipa_init_auth_res *init_auth_res, uint8_t indent,
				  enum log_subsys log_subsys, enum log_level log_level);
void ipa_esipa_init_auth_free_res(struct ipa_init_auth_res *init_auth_res);
