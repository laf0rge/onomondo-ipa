/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, 5.9.1: Function (ES10b): LoadEuiccPackage
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/scard.h>
#include <onomondo/ipa/log.h>
#include "context.h"
#include "utils.h"
#include "euicc.h"
#include "es10x.h"
#include "es10b_load_euicc_pkg.h"

static int dec_load_euicc_pkg_res(struct ipa_es10b_load_euicc_pkg_res *res, const struct ipa_buf *es10b_res)
{
	struct EuiccPackageResult *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_EuiccPackageResult, es10b_res, "LoadEuiccPackage");
	if (!asn)
		return -EINVAL;

	res->res = asn;
	return 0;
}

struct ipa_es10b_load_euicc_pkg_res *ipa_es10b_load_euicc_pkg(struct ipa_context *ctx,
							      const struct ipa_es10b_load_euicc_pkg_req *req)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_load_euicc_pkg_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_load_euicc_pkg_res);
	int rc;

	es10b_req = ipa_es10x_req_enc(&asn_DEF_EuiccPackageRequest, &req->req, "LoadEuiccPackage");
	if (!es10b_req) {
		IPA_LOGP_ES10X("LoadEuiccPackage", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10X("LoadEuiccPackage", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_load_euicc_pkg_res(res, es10b_res);
	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return res;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_load_euicc_pkg_res_free(res);
	return NULL;
}

void ipa_es10b_load_euicc_pkg_res_free(struct ipa_es10b_load_euicc_pkg_res *res)
{
	IPA_ES10X_RES_FREE(asn_DEF_EuiccPackageResult, res);
}
