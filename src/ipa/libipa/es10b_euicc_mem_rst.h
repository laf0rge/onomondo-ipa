#pragma once

#include <stdbool.h>
struct ipa_context;

/* GSMA SGP.22, section 5.7.13 */
struct ipa_es10b_euicc_mem_rst {
	bool operatnl_profiles;
	bool test_profiles;
	bool default_smdp_addr;
};

int ipa_es10b_euicc_mem_rst(struct ipa_context *ctx, const struct ipa_es10b_euicc_mem_rst *req);
