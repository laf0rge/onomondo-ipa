#pragma once

#include <PrepareDownloadResponse.h>
#include <EsipaMessageFromEimToIpa.h>
#include <GetBoundProfilePackageOkEsipa.h>
struct ipa_context;

struct ipa_esipa_get_bnd_prfle_pkg_req {
	PrepareDownloadResponse_t *prep_dwnld_res;
};

struct ipa_esipa_get_bnd_prfle_pkg_res {
	struct EsipaMessageFromEimToIpa *msg_to_ipa;
	struct GetBoundProfilePackageOkEsipa *get_bnd_prfle_pkg_ok;
	long get_bnd_prfle_pkg_err;
};

struct ipa_esipa_get_bnd_prfle_pkg_res *ipa_esipa_get_bnd_prfle_pkg(struct ipa_context *ctx,
								    const struct ipa_esipa_get_bnd_prfle_pkg_req *req);
void ipa_esipa_get_bnd_prfle_pkg_res_free(struct ipa_esipa_get_bnd_prfle_pkg_res *res);
