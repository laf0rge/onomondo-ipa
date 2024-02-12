#pragma once

#include <stdbool.h>

#define IPA_LEN_FQDN 255
#define IPA_LEN_TAC 4
#define IPA_LEN_ALLOWED_CA 20
#define IPE_LEN_EIM_ID 256

struct ipa_context;

/*! IPAd Configuration */
struct ipa_config {

	/*! preferred eimId (optional. When set to NULL, the first eIM config item in the EimConfigurationData list is
	 *  used.) */
	char *preferred_eim_id;

	/* TODO: SYS#6742: The mechanism that allows the configuration of fallback that should be used in case
	 * GetEimConfigurationData fails is not very elegant. This machanism should be replaced with a more
	 * elegant solution. */

	/*! fallback eimId (optional, will be used in case GetEimConfigurationData fails) */
	char *fallback_eim_id;

	/*! fallback eimFqdn (optional, will be used in case GetEimConfigurationData fails) */
	char *fallback_eim_fqdn;

	/*! fallback euiccCiPKId (optional, will be used in case GetEimConfigurationData fails) */
	struct ipa_buf *fallback_euicc_ci_pkid;

	/*! current TAC (This struct member may be updated at any time after context creation.) */
	uint8_t tac[IPA_LEN_TAC];

	/*! The caller may choose to disable SSL in a test environment to simplify debugging. */
	bool eim_disable_ssl;

	/*! ID number of the cardreader that interfaces the eUICC */
	unsigned int reader_num;

	/*! Number of the logical channel that is used to communicate with the ISD-R */
	uint8_t euicc_channel;
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg);
int ipa_init(struct ipa_context *ctx);
int ipa_poll(struct ipa_context *ctx);
void ipa_free_ctx(struct ipa_context *ctx);
