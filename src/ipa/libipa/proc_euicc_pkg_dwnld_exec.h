#pragma once

#include <EuiccPackageRequest.h>
struct ipa_context;

int ipa_proc_eucc_pkg_dwnld_exec(struct ipa_context *ctx, const struct EuiccPackageRequest *euicc_package_request);
int ipa_proc_eucc_pkg_dwnld_exec_onset(struct ipa_context *ctx);
