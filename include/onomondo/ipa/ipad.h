#pragma once

struct ipa_context;

/* Context for one softsim instance. */
struct ipa_config {
	char default_smdp_addr[1024];
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg);
void ipa_poll(struct ipa_context *ctx);
void ipa_free_ctx(struct ipa_context *ctx);
