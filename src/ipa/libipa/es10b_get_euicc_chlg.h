#pragma once

#include <stdint.h>
struct ipa_context;

int ipa_es10b_get_euicc_chlg(struct ipa_context *ctx, uint8_t *euicc_chlg);
