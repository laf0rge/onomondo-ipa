#pragma once

#include <GetEimConfigurationDataResponse.h>
struct ipa_context;

struct ipa_es10b_eim_cfg_data {
	struct GetEimConfigurationDataResponse *eim_cfg_data;
};

struct ipa_es10b_eim_cfg_data *ipa_es10b_get_eim_cfg_data(struct ipa_context *ctx);
void ipa_es10b_get_eim_cfg_data_free(struct ipa_es10b_eim_cfg_data *eim_cfg_data);
struct EimConfigurationData *ipa_es10b_get_eim_cfg_data_filter(struct ipa_es10b_eim_cfg_data *eim_cfg_data,
							       char *eim_id);
