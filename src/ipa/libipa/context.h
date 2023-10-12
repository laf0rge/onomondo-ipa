#pragma once

struct ipa_config;

/* Context for one softsim instance. */
struct ipa_context {
	struct ipa_config *cfg;	
	void *http_ctx;
	void *scard_ctx;
};
