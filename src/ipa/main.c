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
#define DEFAULT_NVSTATE_PATH "./nvstate.bin"

static void print_help(void)
{
	printf("options:\n");
	printf(" -h .................. print this text.\n");
	printf(" -t TAC .............. set TAC (default: %s)\n", DEFAULT_TAC);
	printf(" -e eimId ............ set preferred eIM (in case the eUICC has multiple)\n");
	printf(" -r N ................ set reader number (default: %d)\n", DEFAULT_READER_NUMBER);
	printf(" -c N ................ set logical channel number (default: %d)\n", DEFAULT_CHANNEL_NUMBER);
	printf(" -f PATH ............. set initial eIM configuration\n");
	printf(" -m .................. reset eUICC memory\n");
	printf(" -S .................. disable HTTPS\n");
	printf(" -E .................. emulate IoT eUICC (compatibility mode to use consumer eUICCs)\n");
	printf(" -n PATH ............. path to nvstate file (default: %s)\n", DEFAULT_NVSTATE_PATH);
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

struct ipa_buf *load_nvstate_from_file(char *path)
{
	FILE *file_ptr = NULL;
	struct ipa_buf *nvstate = NULL;
	size_t file_size;

	file_ptr = fopen(path, "r");
	if (!file_ptr) {
		printf("  unable to load nvstate from file %s -- a new nvstate will be created.\n", path);
		return NULL;
	}

	fseek(file_ptr, 0L, SEEK_END);
	file_size = ftell(file_ptr);
	rewind(file_ptr);

	nvstate = ipa_buf_alloc(file_size);
	assert(nvstate);

	nvstate->len = fread(nvstate->data, sizeof(char), nvstate->data_len, file_ptr);
	fclose(file_ptr);
	printf("  loaded nvstate from file %s, size: %zu\n", path, nvstate->data_len);

	return nvstate;
}

void save_nvstate_to_file(char *path, struct ipa_buf *nvstate)
{
	FILE *file_ptr = NULL;

	file_ptr = fopen(path, "w");
	if (!file_ptr) {
		printf("  unable to save nvstate from file %s!\n", path);
		return;
	}

	fwrite(nvstate->data, sizeof(char), nvstate->data_len, file_ptr);
	fclose(file_ptr);
	printf("  saved nvstate to file %s, size: %zu\n", path, nvstate->data_len);
}

int main(int argc, char **argv)
{
	struct ipa_config cfg = { 0 };
	struct ipa_context *ctx;
	int opt;
	int rc;
	char *initial_eim_cfg_file = NULL;
	bool getopt_euicc_memory_reset = false;

	char *getopt_nvstate_path = DEFAULT_NVSTATE_PATH;
	struct ipa_buf *nvstate_load = NULL;
	struct ipa_buf *nvstate_save = NULL;

	printf("IPAd!\n");

	/* Populate configuration with default values */
	cfg.reader_num = 0;
	cfg.euicc_channel = 1;
	ipa_binary_from_hexstr(cfg.tac, sizeof(cfg.tac), DEFAULT_TAC);

	/* Overwrite configuration values with user defined parameters */
	while (1) {
		opt = getopt(argc, argv, "ht:e:r:c:SEf:mn:");
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
			cfg.iot_euicc_emu_enabled = true;
			break;
		case 'f':
			initial_eim_cfg_file = optarg;
			break;
		case 'm':
			getopt_euicc_memory_reset = true;
			break;
		case 'n':
			getopt_nvstate_path = optarg;
			break;
		default:
			printf("unhandled option: %c!\n", opt);
			break;
		};
	}

	printf("nvstate path: %s\n", getopt_nvstate_path);
	nvstate_load = load_nvstate_from_file(getopt_nvstate_path);

	/* Display current config */
	printf("config:\n");
	printf(" preferred_eim_id = %s\n", cfg.preferred_eim_id ? cfg.preferred_eim_id : "(first configured eIM)");
	printf(" reader_num = %d\n", cfg.reader_num);
	printf(" euicc_channel = %d\n", cfg.euicc_channel);
	printf(" eim_disable_ssl = %d\n", cfg.eim_disable_ssl);
	printf(" tac = %s\n", ipa_hexdump(cfg.tac, sizeof(cfg.tac)));
	printf(" iot_euicc_emu_enabled = %u\n", cfg.iot_euicc_emu_enabled);
	printf("\n");

	/* Create a new IPA context */
	ctx = ipa_new_ctx(&cfg, nvstate_load);
	if (!ctx) {
		IPA_LOGP(SMAIN, LERROR, "cannot create context!\n");
		rc = -EINVAL;
		goto error;
	}

	/* Initialize IPA */
	rc = ipa_init(ctx);
	if (rc < 0) {
		IPA_LOGP(SMAIN, LERROR, "IPAd initialization failed!\n");
		rc = -EINVAL;
		goto error;
	}

	/* Load initial eIM configuration */
	if (initial_eim_cfg_file) {
		struct ipa_buf *eim_cfg = load_ber_from_file(NULL, initial_eim_cfg_file);
		ipa_add_init_eim_cfg(ctx, eim_cfg);
		IPA_FREE(eim_cfg);
	} else if (getopt_euicc_memory_reset) {
		ipa_euicc_mem_rst(ctx, true, true, true);
	} else {
		rc = eim_init(ctx);
		if (rc < 0) {
			IPA_LOGP(SMAIN, LERROR, "eIM initialization failed!\n");
			rc = -EINVAL;
			goto error;
		}

		/* Run a single poll cycle and exit. */
		ipa_poll(ctx, false);
	}
error:
	nvstate_save = ipa_free_ctx(ctx);
	save_nvstate_to_file(getopt_nvstate_path, nvstate_save);
	IPA_FREE(nvstate_load);
	IPA_FREE(nvstate_save);
	return rc;
}
