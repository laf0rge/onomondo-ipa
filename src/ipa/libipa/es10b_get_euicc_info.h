#pragma once

#include <stdbool.h>
#include <EUICCInfo1.h>
#include <EUICCInfo2.h>

/* GSMA SGP.22, section 5.7.8 */
struct ipa_es10b_euicc_info {
	struct EUICCInfo1 *euicc_info_1;
	struct EUICCInfo2 *euicc_info_2;
};

struct ipa_es10b_euicc_info *ipa_es10b_get_euicc_info(struct ipa_context *ctx, bool full);
void ipa_es10b_get_euicc_info_free(struct ipa_es10b_euicc_info *euicc_info);
