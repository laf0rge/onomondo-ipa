#pragma once

#include <stdbool.h>

#define IPA_LEN_FQDN 255
#define IPA_LEN_TAC 4
#define IPA_LEN_ALLOWED_CA 20
#define IPE_LEN_EIM_ID 256

struct ipa_context;

/*! IPAd Configuration */
struct ipa_config {
	char eim_addr[IPA_LEN_FQDN];
	char eim_id[IPA_LEN_FQDN];

	/*! current TAC (This struct member may be updated at any time after context creation.) */
	uint8_t tac[IPA_LEN_TAC];

	/*! Must be set to true in order to be sepec conform. (The caller may choose to disable SSL in a test environment
	 *  to simplify debugging) */
	bool eim_use_ssl;

	/*! ID number of the cardreader that interfaces the eUICC */
	unsigned int reader_num;

	/*! Number of the logical channel that is used to communicate with the ISD-R */
	uint8_t euicc_channel;
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg);
int ipa_poll(struct ipa_context *ctx);
void ipa_free_ctx(struct ipa_context *ctx);
