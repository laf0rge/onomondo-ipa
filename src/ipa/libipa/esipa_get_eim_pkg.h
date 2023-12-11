#pragma once

#include <stdint.h>
#include "length.h"

struct ipa_context;

struct ipa_esipa_get_eim_pkg_res {
	struct EsipaMessageFromEimToIpa *msg_to_ipa;
	struct EuiccPackageRequest *euicc_package_request;
	long eim_pkg_err;
};

struct ipa_esipa_get_eim_pkg_res *ipa_esipa_get_eim_pkg(struct ipa_context *ctx, const uint8_t *eid);
void ipa_esipa_get_eim_pkg_free(struct ipa_esipa_get_eim_pkg_res *res);
