#pragma once

#include <stdbool.h>

struct ipa_context;

/* Context for one softsim instance. */
struct ipa_config {
	char default_smdp_addr[1024];
	char eim_addr[1024];
	bool eim_use_ssl;

	/* ID number of the cardreader that interfaces the eUICC */
	unsigned int reader_num;
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg);
void ipa_poll(struct ipa_context *ctx);
void ipa_free_ctx(struct ipa_context *ctx);
