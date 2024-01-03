/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>

#define DEFAULT_READER_NUMBER 0
#define DEFAULT_CHANNEL_NUMBER 1
#define DEFAULT_TAC "12345678"

static void print_help(void)
{
	printf("options:\n");
	printf(" -h .................. print this text.\n");
	printf(" -t TAC .............. set TAC (default: %s)\n", DEFAULT_TAC);
	printf(" -e eimId ............ set preferred eIM (in case the eUICC has multiple)\n");
	printf(" -r N ................ set reader number (default: %d)\n", DEFAULT_READER_NUMBER);
	printf(" -c N ................ set logical channel number (default: %d)\n", DEFAULT_CHANNEL_NUMBER);
}

int main(int argc, char **argv)
{
	struct ipa_config cfg = { 0 };
	struct ipa_context *ctx;
	int opt;

	printf("IPAd!\n");

	/* Populate configuration with default values */
	cfg.reader_num = 0;
	cfg.euicc_channel = 1;
	ipa_binary_from_hexstr(cfg.tac, sizeof(cfg.tac), DEFAULT_TAC);

	/* Overwrite configuration values with user defined parameters */
	while (1) {
		opt = getopt(argc, argv, "ht:e:r:c:");
		if (opt == -1)
			break;

		switch (opt) {
		case 'h':
			print_help();
			exit(0);
			break;
		case 't':
			ipa_binary_from_hexstr(cfg.tac, sizeof(cfg.tac), optarg);
			break;
		case 'e':
			cfg.preferred_eim_id = optarg;
			break;
		case 'r':
			cfg.reader_num = atoi(optarg);
			break;
		case 'c':
			cfg.euicc_channel = atoi(optarg);
			break;
		default:
			printf("unhandled option: %c!\n", opt);
			break;
		};
	}

	/* Display current config */
	printf("config:\n");
	printf(" preferred_eim_id = %s\n", cfg.preferred_eim_id ? cfg.preferred_eim_id : "(first configured eIM)" );
	printf(" reader_num = %d\n", cfg.reader_num);
	printf(" euicc_channel = %d\n", cfg.euicc_channel);
	printf(" tac = %s\n", ipa_hexdump(cfg.tac, sizeof(cfg.tac)));

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
