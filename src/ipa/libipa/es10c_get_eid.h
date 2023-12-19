#pragma once

#include <stdint.h>
struct ipa_context;

int ipa_es10c_get_eid(struct ipa_context *ctx, uint8_t *eid);
