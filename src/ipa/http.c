/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <curl/curl.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/http_hdr.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/mem.h>

#define SKIP_VERIFICATION

struct http_ctx {
	bool initialized;
};

/*! Initialize HTTP client.
 *  \returns pointer to newly allocated HTTP client context. */
void *ipa_http_init(void)
{
	struct http_ctx *ctx = IPA_ALLOC(struct http_ctx);
	assert(ctx);
	memset(ctx, 0, sizeof(*ctx));

	curl_global_init(CURL_GLOBAL_DEFAULT);
	ctx->initialized = true;

	IPA_LOGP(SHTTP, LINFO, "HTTP client initialized.\n");

	return ctx;
}

/* Callback function to extract the HTTP response */
static size_t store_response_cb(void *ptr, size_t size, size_t nmemb, void *clientp)
{
	struct ipa_buf *buf = clientp;

	if (buf->len + size * nmemb > buf->data_len) {
		IPA_LOGP(SHTTP, LERROR, "HTTP response exceeds buffer limit!\n");
		return 0;
	}

	memcpy(buf->data + buf->len, ptr, size * nmemb);
	buf->len += size * nmemb;

	return size * nmemb;
}

/*! Perform HTTP request.
 *  \param[inout] http_ctx HTTP client context.
 *  \param[out] res buffer with HTTP response.
 *  \param[out] req buffer with HTTP request (POST).
 *  \param[in] url URL with HTTP request.
 *  \returns 0 on success, -EIO on failure. */
int ipa_http_req(void *http_ctx, struct ipa_buf *res, const struct ipa_buf *req, char *url)
{
	struct http_ctx *ctx = http_ctx;
	CURL *curl = NULL;
	CURLcode rc;
	struct curl_slist *list = NULL;

	assert(ctx->initialized);

	curl = curl_easy_init();
	if (!curl) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure!\n");
		goto error;
	}
#ifdef SKIP_VERIFICATION
	/* Bypass SSL certificate verification (only for debug, disable in productive use!) */
	rc = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}

	/* Bypass SSL hostname verification (only for debug, disable in productive use!) */
	rc = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	IPA_LOGP(SHTTP, LINFO, "security disabled: will not verify server certificate and hostname\n");
#endif

	/* Setup header, see also SGP.32, section 6.1.1 */
	list = curl_slist_append(list, "Accept:");
	list = curl_slist_append(list, "User-Agent: " IPA_HTTP_USER_AGENT);
	list = curl_slist_append(list, "X-Admin-Protocol: " IPA_HTTP_X_ADMIN_PROTOCOL);
	list = curl_slist_append(list, "Content-Type: " IPA_HTTP_CONTENT_TYPE);
	rc = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}

	/* Perform HTTP Request */
	rc = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->data);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	rc = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->len);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, store_response_cb);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)res);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	rc = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "internal HTTP-client failure: %s\n", curl_easy_strerror(rc));
		goto error;
	}
	rc = curl_easy_perform(curl);
	if (rc != CURLE_OK) {
		IPA_LOGP(SHTTP, LERROR, "HTTP request to %s failed: %s\n", url, curl_easy_strerror(rc));
		goto error;
	}
	IPA_LOGP(SHTTP, LINFO, "HTTP request to %s successful: %s\n", url, curl_easy_strerror(rc));

	curl_easy_cleanup(curl);
	curl_slist_free_all(list);
	return 0;
error:
	curl_easy_cleanup(curl);
	curl_slist_free_all(list);
	return -EIO;
}

/*! Free HTTP client.
 *  \param[inout] http_ctx HTTP client context.
 *  \returns pointer to HTTP client context. */
void ipa_http_free(void *http_ctx)
{
	struct http_ctx *ctx = http_ctx;

	if (!http_ctx)
		return;

	curl_global_cleanup();
	IPA_FREE(ctx);
	IPA_LOGP(SHTTP, LINFO, "HTTP client freed.\n");
}
