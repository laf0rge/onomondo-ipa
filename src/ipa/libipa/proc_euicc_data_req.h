#pragma once

#include <IpaEuiccDataRequest.h>
struct ipa_context;

struct ipa_proc_euicc_data_req_pars {
	const struct IpaEuiccDataRequest *ipa_euicc_data_request;
};

int ipa_proc_euicc_data_req(struct ipa_context *ctx, const struct ipa_proc_euicc_data_req_pars *pars);
