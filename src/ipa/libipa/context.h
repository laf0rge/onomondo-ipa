#pragma once

#include "length.h"
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/utils.h>
struct ipa_config;

/*! Context for one IPAd instance. */
struct ipa_context {
	/*! pointer to IPAd configuration options. */
	struct ipa_config *cfg;

	/*! sub-context of the HTTP(s) connection towards the eIM, */
	void *http_ctx;

	/*! sub-context of the smartcard connection towards the eUICC, */
	void *scard_ctx;

	/*! cached eID (read from eUICC on context creation) */
	uint8_t eid[IPA_LEN_EID];

	/*! cached eimId (read from eUICC on context creation) */
	char *eim_id;

	/*! cached eIM address (read from eUICC on context creation) */
	char *eim_fqdn;

	/*! cached allowed CA (optional, read from eUICC on context creation) */
	struct ipa_buf *euicc_ci_pkid;
};
