/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>

int main(void)
{
	struct ipa_config cfg = { 0 };
	struct ipa_context *ctx;

	IPA_LOGP(SMAIN, LINFO, "ipa!\n");

	/* Setup a basic configuration */
	/* TODO: make those values user configurable */
	cfg.preferred_eim_id = "myEIM";
	cfg.reader_num = 0;
	cfg.euicc_channel = 1;
	memcpy(cfg.tac, "\x12\x34\x56\x78", IPA_LEN_TAC);

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
