#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <onomondo/ipa/ipad.h>
#include "esipa.h"
#include "context.h"

#define PREFIX_HTTP "http://"
#define PREFIX_HTTPS "https://"
#define SUFFIX "/gsma/rsp2/asn1"

/*! Read the configured eIM URL (FQDN).
 *  \param[in] ctx pointer to ipa_context.
 *  \returns pointer to eIM URL (statically allocated, do not free). */
char *ipa_esipa_get_eim_url(struct ipa_context *ctx)
{
	static char eim_url[IPA_ESIPA_URL_MAXLEN];
	size_t url_len = 0;

	memset(eim_url, 0, sizeof(eim_url));

	/* Make sure we have a reasonable minimum of space available */
	assert(sizeof(eim_url) > 255);

	if (ctx->cfg->eim_use_ssl)
		strcpy(eim_url, PREFIX_HTTPS);
	else
		strcpy(eim_url, PREFIX_HTTP);

	/* Be sure we don't accidentally overrun the buffer */
	url_len = strlen(eim_url);
	url_len += strlen(ctx->cfg->eim_addr);
	url_len += strlen(SUFFIX);
	assert(url_len < sizeof(eim_url));

	strcat(eim_url, ctx->cfg->eim_addr);
	strcat(eim_url, SUFFIX);

	return eim_url;
}
