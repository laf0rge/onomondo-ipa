#pragma once

#define IPA_LOGP_ESIPA(func, level, fmt, args...) \
	IPA_LOGP(SESIPA, level, "%s: " fmt, func, ## args)

#define IPA_ESIPA_URL_MAXLEN 1024

char *ipa_esipa_get_eim_url(struct ipa_context *ctx);
