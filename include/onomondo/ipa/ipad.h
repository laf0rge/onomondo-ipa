#pragma once

#include <stdbool.h>

#define IPA_LEN_FQDN 255
#define IPA_LEN_TAC 4
#define IPA_LEN_ALLOWED_CA 20
#define IPE_LEN_EIM_ID 256

struct ipa_context;

/* IPA Configuration */
struct ipa_config {
	char eim_addr[IPA_LEN_FQDN];
	char eim_id[IPA_LEN_FQDN];
	uint8_t tac[IPA_LEN_TAC];
	uint8_t allowed_ca[IPA_LEN_ALLOWED_CA];
	bool eim_use_ssl;

	/* ID number of the cardreader that interfaces the eUICC */
	unsigned int reader_num;
	uint8_t euicc_channel;
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg);
void ipa_poll(struct ipa_context *ctx);
void ipa_free_ctx(struct ipa_context *ctx);
