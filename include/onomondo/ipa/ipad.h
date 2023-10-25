#pragma once

#include <stdbool.h>

#define IPA_LEN_FQDN 255

struct ipa_context;

/* Context for one softsim instance. */
struct ipa_config {
	char default_smdp_addr[IPA_LEN_FQDN];
	char eim_addr[IPA_LEN_FQDN];
	bool eim_use_ssl;

	/* ID number of the cardreader that interfaces the eUICC */
	unsigned int reader_num;
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg);
void ipa_poll(struct ipa_context *ctx);
void ipa_free_ctx(struct ipa_context *ctx);
