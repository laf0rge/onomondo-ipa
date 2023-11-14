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

int main(void)
{
	struct ipa_config cfg;
	struct ipa_context *ctx;

	IPA_LOGP(SMAIN, LINFO, "ipa!\n");

	strcpy(cfg.default_smdp_addr, "www.example.net");
	strcpy(cfg.eim_addr, "127.0.0.1:4430");
	cfg.eim_use_ssl = true;
	cfg.reader_num = 0;
	cfg.euicc_channel = 2;
	ctx = ipa_new_ctx(&cfg);
	if (!ctx) {
		IPA_LOGP(SMAIN, LERROR, "no context, initialization failed!\n");
		return -EINVAL;
	}

	ipa_poll(ctx);
	ipa_free_ctx(ctx);

	return 0;
}
