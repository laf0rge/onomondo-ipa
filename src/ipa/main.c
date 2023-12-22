/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
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

int main(void)
{
	struct ipa_config cfg;
	struct ipa_context *ctx;

	IPA_LOGP(SMAIN, LINFO, "ipa!\n");

	/* Setup a basic configuration */
	/* TODO: make those values user configurable */
	strcpy(cfg.eim_addr, "127.0.0.1:4430");
	strcpy(cfg.eim_id, "myEIM");
	cfg.eim_use_ssl = true;
	cfg.reader_num = 0;
	cfg.euicc_channel = 1;
	memcpy(cfg.tac, "\x12\x34\x56\x78", IPA_LEN_TAC);
	memcpy(cfg.allowed_ca, "\xF5\x41\x72\xBD\xF9\x8A\x95\xD6\x5C\xBE\xB8\x8A\x38\xA1\xC1\x1D\x80\x0A\x85\xC3",
	       IPA_LEN_ALLOWED_CA);

	/* Create a new IPA context */
	ctx = ipa_new_ctx(&cfg);
	if (!ctx) {
		IPA_LOGP(SMAIN, LERROR, "no context, initialization failed!\n");
		return -EINVAL;
	}

	/* Run a single poll cycle and exit. */
	ipa_poll(ctx);
	ipa_free_ctx(ctx);
	return 0;
}
