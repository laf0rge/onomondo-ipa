#pragma once

#include <stdbool.h>
#include <EUICCInfo1.h>
#include <EUICCInfo2.h>
#include <SGP32-EUICCInfo2.h>

/* GSMA SGP.22, section 5.7.8 */
struct ipa_es10b_euicc_info {
	struct EUICCInfo1 *euicc_info_1;
	struct EUICCInfo2 *euicc_info_2;
	struct SGP32_EUICCInfo2 *sgp32_euicc_info_2;

	/*! When the IoT eUICC emulation is enabled this function retrieve the native euicc_info_2 from the eUICC and
	 *  convert it into sgp32_euicc_info_2. When the emulation is turned off sgp32_euicc_info_2 will be retrieved
	 *  and no conversion is needed. This also means that euicc_info_2 will be unpopulated (NULL) in this case.
	 *  It is recommended to use sgp32_euicc_info_2 only, since it will always be populated and technially the
	 *  only one needed in SGP.32 / ESipa context. */
};

struct ipa_es10b_euicc_info *ipa_es10b_get_euicc_info(struct ipa_context *ctx, bool full);
void ipa_es10b_get_euicc_info_free(struct ipa_es10b_euicc_info *res);
