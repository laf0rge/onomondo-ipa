#pragma once

#define IPA_ES10B_EUICC_CHALLENGE_LEN 16

int ipa_es10b_get_euicc_challenge(struct ipa_context *ctx, uint8_t *euicc_challenge);
