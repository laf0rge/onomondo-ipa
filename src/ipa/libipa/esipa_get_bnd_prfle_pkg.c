/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, section 5.14.2: Function: (ESipa) GetBoundProfilePackage
 *
 */

#include <stdint.h>
#include <errno.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include "utils.h"
#include "length.h"
#include "context.h"
#include "esipa.h"
#include "SGP32-PrepareDownloadResponse.h"
#include "PrepareDownloadResponse.h"
#include <EsipaMessageFromIpaToEim.h>
#include <EsipaMessageFromEimToIpa.h>
#include <GetBoundProfilePackageRequestEsipa.h>
#include <GetBoundProfilePackageResponseEsipa.h>
#include "esipa_get_bnd_prfle_pkg.h"

static struct ipa_buf *enc_get_bnd_prfle_pkg_req(const struct ipa_esipa_get_bnd_prfle_pkg_req *req)
{
	struct EsipaMessageFromIpaToEim msg_to_eim = { 0 };

	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_getBoundProfilePackageRequestEsipa;

	switch (req->prep_dwnld_res->present) {
	case PrepareDownloadResponse_PR_downloadResponseOk:
		msg_to_eim.choice.getBoundProfilePackageRequestEsipa.prepareDownloadResponse.present =
		    SGP32_PrepareDownloadResponse_PR_downloadResponseOk;
		msg_to_eim.choice.getBoundProfilePackageRequestEsipa.prepareDownloadResponse.choice.downloadResponseOk =
		    req->prep_dwnld_res->choice.downloadResponseOk;
		msg_to_eim.choice.getBoundProfilePackageRequestEsipa.transactionId =
		    req->prep_dwnld_res->choice.downloadResponseOk.euiccSigned2.transactionId;
		break;
	case PrepareDownloadResponse_PR_downloadResponseError:
		msg_to_eim.choice.getBoundProfilePackageRequestEsipa.prepareDownloadResponse.present =
		    SGP32_PrepareDownloadResponse_PR_downloadResponseError;
		msg_to_eim.choice.getBoundProfilePackageRequestEsipa.prepareDownloadResponse.choice.
		    downloadResponseError = req->prep_dwnld_res->choice.downloadResponseError;
		msg_to_eim.choice.getBoundProfilePackageRequestEsipa.transactionId =
		    req->prep_dwnld_res->choice.downloadResponseError.transactionId;
		break;
	default:
		IPA_LOGP_ESIPA("GetBoundProfilePackage", LINFO,
			       "prepare download response is empty, cannot encode request\n");
		return NULL;
	}

	/* Encode */
	return ipa_esipa_msg_to_eim_enc(&msg_to_eim, "GetBoundProfilePackage");
}

static struct ipa_esipa_get_bnd_prfle_pkg_res *dec_get_bnd_prfle_pkg_res(const struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	struct ipa_esipa_get_bnd_prfle_pkg_res *res = NULL;

	msg_to_ipa =
	    ipa_esipa_msg_to_ipa_dec(msg_to_ipa_encoded, "GetBoundProfilePackage",
				     EsipaMessageFromEimToIpa_PR_getBoundProfilePackageResponseEsipa);
	if (!msg_to_ipa)
		return NULL;

	res = IPA_ALLOC_ZERO(struct ipa_esipa_get_bnd_prfle_pkg_res);
	res->msg_to_ipa = msg_to_ipa;

	switch (msg_to_ipa->choice.initiateAuthenticationResponseEsipa.present) {
	case GetBoundProfilePackageResponseEsipa_PR_getBoundProfilePackageOkEsipa:
		res->get_bnd_prfle_pkg_ok =
		    &msg_to_ipa->choice.getBoundProfilePackageResponseEsipa.choice.getBoundProfilePackageOkEsipa;
		break;
	case GetBoundProfilePackageResponseEsipa_PR_getBoundProfilePackageErrorEsipa:
		res->get_bnd_prfle_pkg_err =
		    msg_to_ipa->choice.getBoundProfilePackageResponseEsipa.choice.getBoundProfilePackageErrorEsipa;
		IPA_LOGP_ESIPA("GetBoundProfilePackage", LERROR, "function failed with error code %ld!\n",
			       res->get_bnd_prfle_pkg_err);
		break;
	default:
		IPA_LOGP_ESIPA("GetBoundProfilePackage", LERROR, "unexpected response content!\n");
		res->get_bnd_prfle_pkg_err = -1;
		break;
	}

	return res;
}

struct ipa_esipa_get_bnd_prfle_pkg_res *ipa_esipa_get_bnd_prfle_pkg(struct ipa_context *ctx,
								    const struct ipa_esipa_get_bnd_prfle_pkg_req *req)
{
	struct ipa_buf *esipa_req;
	struct ipa_buf *esipa_res;
	struct ipa_esipa_get_bnd_prfle_pkg_res *res = NULL;

	IPA_LOGP_ESIPA("GetBoundProfilePackage", LINFO, "Requesting bound profile package from eIM\n");

	esipa_req = enc_get_bnd_prfle_pkg_req(req);
	if (!esipa_req)
		goto error;

	esipa_res = ipa_esipa_req(ctx, esipa_req, "InitiateAuthentication");
	if (!esipa_res)
		goto error;

	res = dec_get_bnd_prfle_pkg_res(esipa_res);
	if (!res)
		goto error;

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return res;
}

void ipa_esipa_get_bnd_prfle_pkg_res_free(struct ipa_esipa_get_bnd_prfle_pkg_res *res)
{
	IPA_ESIPA_RES_FREE(res);
}
