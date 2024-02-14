/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <limits.h>
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
	printf(" -f PATH ............. set initial eIM configuration\n");
	printf(" -S .................. disable HTTPS\n");
	printf(" -E PATH ............. emulate IoT euicc, set path to .ber files with emulation data\n");
}

struct ipa_buf *load_ber_from_file(char *dir, char *file)
{
	char path[PATH_MAX] = { 0 };
	FILE *ber_file = NULL;
	struct ipa_buf *ber = NULL;
	size_t ber_size;

	if (dir)
		strcpy(path, dir);
	strcat(path, file);

	ber_file = fopen(path, "r");
	assert(ber_file);

	fseek(ber_file, 0L, SEEK_END);
	ber_size = ftell(ber_file);
	rewind(ber_file);

	ber = ipa_buf_alloc(ber_size + 1);
	assert(ber);

	ber->len = fread(ber->data, sizeof(char), ber->data_len, ber_file);
	fclose(ber_file);
	printf("  loaded BER data from file %s, size: %zu\n", path, ber->len);

	return ber;
}

int main(int argc, char **argv)
{
	struct ipa_config cfg = { 0 };
	struct ipa_context *ctx;
	int opt;
	int rc;
	char *iot_euicc_emu_ber_path = NULL;
	char *initial_eim_cfg_file = NULL;

	printf("IPAd!\n");

	/* Populate configuration with default values */
	cfg.reader_num = 0;
	cfg.euicc_channel = 1;
	ipa_binary_from_hexstr(cfg.tac, sizeof(cfg.tac), DEFAULT_TAC);

	/* Overwrite configuration values with user defined parameters */
	while (1) {
		opt = getopt(argc, argv, "ht:e:r:c:SE:f:");
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
		case 'S':
			cfg.eim_disable_ssl = true;
			break;
		case 'E':
			iot_euicc_emu_ber_path = optarg;
			break;
		case 'f':
			initial_eim_cfg_file = optarg;
			break;
		default:
			printf("unhandled option: %c!\n", opt);
			break;
		};
	}

	/* Display current config */
	printf("config:\n");
	printf(" preferred_eim_id = %s\n", cfg.preferred_eim_id ? cfg.preferred_eim_id : "(first configured eIM)");
	printf(" reader_num = %d\n", cfg.reader_num);
	printf(" euicc_channel = %d\n", cfg.euicc_channel);
	printf(" eim_disable_ssl = %d\n", cfg.eim_disable_ssl);
	printf(" tac = %s\n", ipa_hexdump(cfg.tac, sizeof(cfg.tac)));

	if (iot_euicc_emu_ber_path) {
		printf(" IoT euicc emulation:\n");
		printf("  iot_euicc_emu_ber_path = %s\n", iot_euicc_emu_ber_path);
		cfg.iot_euicc_emu.enabled = true;
		cfg.iot_euicc_emu.eim_cfg_ber = load_ber_from_file(iot_euicc_emu_ber_path, "eim_cfg.ber");
	}

	printf("\n");

	/* Create a new IPA context */
	ctx = ipa_new_ctx(&cfg);
	if (!ctx) {
		IPA_LOGP(SMAIN, LERROR, "cannot create context!\n");
		rc = -EINVAL;
		goto error;
	}

	/* Initialize IPA */
	rc = ipa_init(ctx);
	if (rc < 0) {
		IPA_LOGP(SMAIN, LERROR, "initialization failed!\n");
		rc = -EINVAL;
		goto error;
	}

	/* Load initial eIM configuration */
	if (initial_eim_cfg_file) {
		struct ipa_buf *eim_cfg = load_ber_from_file(NULL, initial_eim_cfg_file);
		ipa_eim_cfg(ctx, eim_cfg);
		IPA_FREE(eim_cfg);
	} else {
		/* Run a single poll cycle and exit. */
		ipa_poll(ctx);
	}
error:
	ipa_free_ctx(ctx);
	IPA_FREE(cfg.iot_euicc_emu.eim_cfg_ber);
	return rc;
}
