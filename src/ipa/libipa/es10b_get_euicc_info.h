#pragma once

#include "length.h"
#include "es10b.h"
#include <stdbool.h>
#include <EUICCInfo1.h>

#define IPA_ES10B_CI_PKID_MAX 255

/* GSMA SGP.22, section 5.7.8 */
struct ipa_es10b_euicc_info {
	struct EUICCInfo1 *euicc_info_1;
	struct EUICCInfo2 *euicc_info_2;
};

struct ipa_es10b_euicc_info *ipa_es10b_get_euicc_info(struct ipa_context *ctx, bool full);
void ipa_es10b_get_euicc_info_dump(struct ipa_es10b_euicc_info *euicc_info,
				   uint8_t indent, enum log_subsys log_subsys, enum log_level log_level);
void ipa_es10b_get_euicc_info_free(struct ipa_es10b_euicc_info *euicc_info);
