#pragma once

#include <EuiccConfiguredAddressesResponse.h>
struct ipa_context;

struct ipa_es10a_euicc_cfg_addr {
	struct EuiccConfiguredAddressesResponse *res;
};

struct ipa_es10a_euicc_cfg_addr *ipa_es10a_get_euicc_cfg_addr(struct ipa_context *ctx);
void ipa_es10a_get_euicc_cfg_addr_free(struct ipa_es10a_euicc_cfg_addr *res);
