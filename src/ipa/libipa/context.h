#pragma once

#include "length.h"
struct ipa_config;

/*! Context for one IPAd instance. */
struct ipa_context {
	/*! pointer to IPAd configuration options. */
	struct ipa_config *cfg;

	/*! sub-context of the HTTP(s) connection towards the eIM, */
	void *http_ctx;

	/*! sub-context of the smartcard connection towards the eUICC, */
	void *scard_ctx;

	/*! cached eID of the eUICC. */
	uint8_t eid[IPA_LEN_EID];
};
