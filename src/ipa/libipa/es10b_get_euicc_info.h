#pragma once

#include "length.h"
#include "es10b.h"
#include <stdbool.h>

#define IPA_ES10B_CI_PKID_MAX 255

/* GSMA SGP.22, section 5.7.8 */
struct ipa_es10b_euicc_info {
	/* Set to true when the struct contains the full eUICC info
	 * (EUICCInfo2) instead of the the minimal eUICC info (EUICCInfo1) */
	bool full;

	/* The following struct members are included in EUICCInfo1 and
	 * EUICCInfo2 */
	uint8_t svn[IPA_ES10B_VERSION_LEN];
	struct ipa_buf *ci_pkid_verf[IPA_ES10B_CI_PKID_MAX];
	size_t ci_pkid_verf_count;
	struct ipa_buf *ci_pkid_sign[IPA_ES10B_CI_PKID_MAX];
	size_t ci_pkid_sign_count;

	/* The following struct members are only included in EUICCInfo2 */
	uint8_t profile_version[IPA_ES10B_VERSION_LEN];
	uint8_t firmware_version[IPA_ES10B_VERSION_LEN];
	bool ts_102241_version_present;
	uint8_t ts_102241_version[IPA_ES10B_VERSION_LEN];
	bool gp_version_present;
	uint8_t gp_version[IPA_ES10B_VERSION_LEN];
	uint8_t pp_version[IPA_ES10B_VERSION_LEN];	
	struct ipa_buf *ext_card_resource;
	struct ipa_buf *uicc_capability;
	struct ipa_buf *rsp_capability;
	bool euicc_category_present;
	long int euicc_category;
	bool forb_profile_policy_rules_present;
	struct ipa_buf *forb_profile_policy_rules;
        char *sas_acreditation_number;
	bool cert_data_present;
	char *cert_platform_label;
	char *cert_discovery_base_url;
};

struct ipa_es10b_euicc_info *ipa_es10b_get_euicc_info(struct ipa_context *ctx, bool full);
void ipa_es10b_dump_euicc_info(struct ipa_es10b_euicc_info *euicc_info,
			       uint8_t indent, enum log_subsys log_subsys,
			       enum log_level log_level);
void ipa_es10b_free_euicc_info(struct ipa_es10b_euicc_info *euicc_info);
