#pragma once

#include <stdbool.h>

#define IPA_LEN_FQDN 255
#define IPA_LEN_TAC 4
#define IPA_LEN_ALLOWED_CA 20
#define IPE_LEN_EIM_ID 256

struct ipa_context;
struct ipa_buf;

/*! IPAd Configuration */
struct ipa_config {
	/*! preferred eimId (optional. When set to NULL, the first eIM config item in the EimConfigurationData list is
	 *  used.) */
	char *preferred_eim_id;

	/*! current TAC (This struct member may be updated at any time after context creation.) */
	uint8_t tac[IPA_LEN_TAC];

	/*! The caller may choose to disable SSL in a test environment to simplify debugging. */
	bool eim_disable_ssl;

	/*! Configure the number of retries to apply in case a request (HTTP) to the eIM fails */
	unsigned int esipa_req_retries;

	/*! ID number of the cardreader that interfaces the eUICC */
	unsigned int reader_num;

	/*! Number of the logical channel that is used to communicate with the ISD-R */
	uint8_t euicc_channel;

	/*! Enable IoT eUICC emulation.
	 *  This IPAd also supports the use of consumer eUICCs, which have a slightly different interface. When the
	 *  IoT eUICC emulation is enabled, the IPAd will adapt the interface on ES10x function level so that the
	 *  consumer eUICC appears as an IoT eUICC on procedure level. */
	bool iot_euicc_emu_enabled;
};

struct ipa_context *ipa_new_ctx(struct ipa_config *cfg, struct ipa_buf *nvstate);
int ipa_init(struct ipa_context *ctx);
int eim_init(struct ipa_context *ctx);
int ipa_add_init_eim_cfg(struct ipa_context *ctx, struct ipa_buf *cfg);
int ipa_euicc_mem_rst(struct ipa_context *ctx, bool operatnl_profiles, bool test_profiles, bool default_smdp_addr);
int ipa_poll(struct ipa_context *ctx);
void ipa_close(struct ipa_context *ctx);
struct ipa_buf *ipa_free_ctx(struct ipa_context *ctx);
