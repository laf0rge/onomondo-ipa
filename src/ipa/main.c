/*
 * Author: Philipp Maier
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/scard.h>
#include <onomondo/ipa/ipad.h>

#include <EsipaMessageFromIpaToEim.h>
#include <GetEuiccDataRequest.h>

int asn_app_consume_bytes(const void *buffer, size_t size,
			  void *application_specific_key)
{
	printf("buffer=%s, size=%li\n", ipa_hexdump(buffer, size), size);
	return 0;
}

void testme_esipa_encode(void)
{
	struct EsipaMessageFromIpaToEim msg_to_eim;
	asn_enc_rval_t rc;
	uint8_t challenge[2] = { 0xAA, 0xBB };

	memset(&msg_to_eim, 0, sizeof(msg_to_eim));
	msg_to_eim.present =
	    EsipaMessageFromIpaToEim_PR_initiateAuthenticationRequestEsipa;
	msg_to_eim.choice.initiateAuthenticationRequestEsipa.euiccChallenge.
	    buf = challenge;
	msg_to_eim.choice.initiateAuthenticationRequestEsipa.euiccChallenge.
	    size = 2;

	rc = der_encode(&asn_DEF_EsipaMessageFromIpaToEim, &msg_to_eim,
			asn_app_consume_bytes, NULL);
	printf("============> rc.encoded = %li\n", rc.encoded);

}

void testme_esipa_decode(void)
{
	uint8_t msg_to_eim_encoded[] =
	    { 0xbf, 0x39, 0x04, 0x81, 0x02, 0xaa, 0xbb };
	asn_dec_rval_t rc;
	struct EsipaMessageFromIpaToEim *msg_to_eim;

	rc = ber_decode(0, &asn_DEF_EsipaMessageFromIpaToEim,
			(void **)&msg_to_eim, msg_to_eim_encoded,
			sizeof(msg_to_eim_encoded));

	printf("============> rc.code = %i\n", rc.code);
	asn_fprint(stdout, &asn_DEF_EsipaMessageFromIpaToEim, msg_to_eim);
	ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromIpaToEim, msg_to_eim);
}

void testme_es10_encode(void)
{
	struct GetEuiccDataRequest get_euicc_data;
	asn_enc_rval_t rc;
	uint8_t tagList[1] = { 0x5A };

	memset(&get_euicc_data, 0, sizeof(get_euicc_data));
	get_euicc_data.tagList.buf = tagList;
	get_euicc_data.tagList.size = 1;

	rc = der_encode(&asn_DEF_GetEuiccDataRequest, &get_euicc_data,
			asn_app_consume_bytes, NULL);
	printf("============> rc.encoded = %li\n", rc.encoded);
}

void testme_es10_decode(void)
{
	uint8_t get_euicc_data_encoded[] =
	    { 0xbf, 0x3e, 0x03, 0x5c, 0x01, 0x5a };
	asn_dec_rval_t rc;
	struct GetEuiccDataRequest *get_euicc_data;

	rc = ber_decode(0, &asn_DEF_GetEuiccDataRequest,
			(void **)&get_euicc_data, get_euicc_data_encoded,
			sizeof(get_euicc_data_encoded));

	printf("============> rc.code = %i\n", rc.code);
	asn_fprint(stdout, &asn_DEF_GetEuiccDataRequest, get_euicc_data);
	ASN_STRUCT_FREE(asn_DEF_GetEuiccDataRequest, get_euicc_data);
}

void testme_http(void)
{
	struct ipa_buf *buf_res = ipa_buf_alloc(1024);
	struct ipa_buf *buf_req = ipa_buf_alloc(1024);

	void *http_ctx;

	strcpy((char *)buf_req->data, "This is my POST request");
	buf_req->len = strlen((char *)buf_req->data);

	http_ctx = ipa_http_init();

	ipa_http_req(http_ctx, buf_res, buf_req,
		     "https://127.0.0.1:4430/gsma/rsp2/asn1");
	printf("==========> RESPONSE: %s\n", buf_res->data);

	ipa_http_free(http_ctx);
	IPA_FREE(buf_res);
	IPA_FREE(buf_req);
}

void testme_scard(void)
{
	struct ipa_buf *buf_res = ipa_buf_alloc(1024);
	struct ipa_buf *buf_req = ipa_buf_alloc(1024);
	struct ipa_buf *buf_atr = ipa_buf_alloc(1024);
	void *scard_ctx;

	memcpy((char *)buf_req->data, "\x00\xA4\x00\x00\x02\x3F\x00", 7);
	buf_req->len = 7;

	scard_ctx = ipa_scard_init(0);
	if (scard_ctx) {
		ipa_scard_reset(scard_ctx);
		ipa_scard_atr(scard_ctx, buf_atr);

		ipa_scard_transceive(scard_ctx, buf_res, buf_req);
		printf("==========> RESPONSE: %s\n",
		       ipa_hexdump(buf_res->data, buf_res->len));

		ipa_scard_free(scard_ctx);
	}

	IPA_FREE(buf_res);
	IPA_FREE(buf_req);
	IPA_FREE(buf_atr);
}

int main(void)
{
	IPA_LOGP(SMAIN, LINFO, "ipa!\n");
//      testme_esipa_encode();
//      testme_esipa_decode();
//      testme_es10_encode();
//      testme_es10_decode();
//      testme_http();
//	testme_scard();

#if 1
	struct ipa_config cfg;
	struct ipa_context *ctx;

	strcpy(cfg.default_smdp_addr, "www.example.net");
	cfg.reader_num = 0;
	ctx = ipa_new_ctx(&cfg);
	if (!ctx) {
		IPA_LOGP(LERROR, LINFO, "no context, initialization failed!\n");
		return -EINVAL;
	}

	ipa_poll(ctx);
	ipa_free_ctx(ctx);
#endif
	return 0;
}
