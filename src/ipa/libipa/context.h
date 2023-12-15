#pragma once

#include "length.h"
struct ipa_config;

/* Context for one IPA instance. */
struct ipa_context {
	struct ipa_config *cfg;
	void *http_ctx;
	void *scard_ctx;
	uint8_t eid[IPA_LEN_EID];
};
