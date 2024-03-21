#pragma once

#include "length.h"
#include <onomondo/ipa/ipad.h>
#include <onomondo/ipa/utils.h>

#define IPA_NVSTATE_VERSION 1

/* Non volatile state: All struct members in this struct are automatically backed up to a non volatile memory location.
 * (see below). However, this only covers statically allocated struct members. When struct members contain a pointer
 * to a dynamically allocated memory location, then appropriate code must be added to nvstate_serialize,
 * nvstate_deserialize and nvstate_free_contents (see ipad.c). */
struct ipa_nvstate {

	uint32_t version;

	/*! internal storage for IoT eUICC emulation. */
	struct {
		struct ipa_buf *eim_cfg_ber;
	} iot_euicc_emu;

} __attribute__((packed));

/*! Context for one IPAd instance. */
struct ipa_context {
	/*! pointer to IPAd configuration options. */
	struct ipa_config *cfg;

	/*! sub-context of the HTTP(s) connection towards the eIM, */
	void *http_ctx;

	/*! sub-context of the smartcard connection towards the eUICC, */
	void *scard_ctx;

	/*! cached eID (read from eUICC when ipa_init is called) */
	uint8_t eid[IPA_LEN_EID];

	/*! cached eimId (read from eUICC when ipa_init is called) */
	char *eim_id;

	/*! cached eIM address (read from eUICC when ipa_init is called) */
	char *eim_fqdn;

	/*! cached EuiccPackageResult */
	struct ipa_es10b_load_euicc_pkg_res *load_euicc_pkg_res;

	/*! Non volatile storage: Everything stored in this struct is loaded by the API user from a non volatile memory
	 *  location on startup (ipa_new_ctx) and stored to a non volatile location on exit (ipa_free_ctx). */
	struct ipa_nvstate nvstate;
};
