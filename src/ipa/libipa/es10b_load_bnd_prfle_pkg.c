/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.22, 5.7.6: Function (ES10b): LoadBoundProfilePackage
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
#include "length.h"
#include "utils.h"
#include "euicc.h"
#include "es10x.h"
#include "es10b_load_bnd_prfle_pkg.h"

static int dec_prfle_inst_res(struct ipa_es10b_load_bnd_prfle_pkg_res *res, const struct ipa_buf *es10b_res)
{
	struct ProfileInstallationResult *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_ProfileInstallationResult, es10b_res, "LoadBoundProfilePackage");
	if (!asn)
		return -EINVAL;

	res->res = asn;
	return 0;
}

/*! Function (ES10b): LoadBoundProfilePackage.
 *  \param[inout] ctx pointer to ipa_context.
 *  \param[in] segment pointer to (encrypted) ES8+ TLV segment.
 *  \param[in] segment_len length of segment data.
 *  \returns pointer newly allocated struct with function result, NULL on error. */
struct ipa_es10b_load_bnd_prfle_pkg_res *ipa_es10b_load_bnd_prfle_pkg(struct ipa_context *ctx, const uint8_t *segment,
								      size_t segment_len)
{
	struct ipa_buf es10b_req;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_load_bnd_prfle_pkg_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_load_bnd_prfle_pkg_res);
	int rc;

	/* The ES10b LoadBoundProfilePackage function is a pseudo function that only exists as a placeholder in the
	 * specification. In practice it is just a STORE DATA command that carries a profile segment without any
	 * envelope around. (See also SGP.22, section 5.7.6 and SGP.22, section 2.5.5 */

	ipa_buf_assign(&es10b_req, segment, segment_len);
	es10b_res = ipa_euicc_transceive_es10x(ctx, &es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10X("LoadBoundProfilePackage", LERROR, "no ES10b response\n");
		goto error;
	}

	/* The ES10b LoadBoundProfilePackage function only returns a response on the final function call of a sequence
	 * This response contains the ProfileInstallationResult. */
	if (es10b_res->len) {
		rc = dec_prfle_inst_res(res, es10b_res);
		if (rc < 0)
			goto error;
	}

	IPA_FREE(es10b_res);
	return res;
error:
	IPA_FREE(es10b_res);
	IPA_FREE(res);
	return NULL;
}

/*! Free results of function (ES10b): LoadBoundProfilePackage.
 *  \param[in] res pointer to function result. */
void ipa_es10b_load_bnd_prfle_res_free(struct ipa_es10b_load_bnd_prfle_pkg_res *res)
{
	IPA_ES10X_RES_FREE(asn_DEF_ProfileInstallationResult, res);
}
