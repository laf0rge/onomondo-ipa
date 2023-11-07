#include <stdint.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include "utils.h"
#include "length.h"
#include "context.h"
#include "esipa.h"
#include "esipa_get_eim_pkg.h"
#include <EsipaMessageFromIpaToEim.h>
#include <EsipaMessageFromEimToIpa.h>
#include <GetEimPackageRequest.h>
#include <ProfileDownloadTriggerRequest.h>

static struct ipa_buf *enc_get_eim_pkg_req(uint8_t *eid_value)
{
	struct EsipaMessageFromIpaToEim msg_to_eim;

	memset(&msg_to_eim, 0, sizeof(msg_to_eim));
	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_getEimPackageRequest;
	msg_to_eim.choice.getEimPackageRequest.eidValue.buf = eid_value;
	msg_to_eim.choice.getEimPackageRequest.eidValue.size = IPA_LEN_EID;

	return ipa_esipa_msg_to_eim_enc(&msg_to_eim, "GetEimPackage");
}

static struct ipa_esipa_eim_pkg *dec_profile_dwnld_trig_req(ProfileDownloadTriggerRequest_t * pdtr)
{
	struct ipa_esipa_eim_pkg *eim_pkg = NULL;
	struct ProfileDownloadData *pdd;

	if (!pdtr->profileDownloadData) {
		IPA_LOGP_ESIPA("GetEimPackage.ProfileDownloadTriggerRequest", LERROR,
			       "request did not contain any profile download data.\n");
		return NULL;
	}
	pdd = pdtr->profileDownloadData;

	switch (pdd->present) {
	case ProfileDownloadData_PR_activationCode:
		if (pdd->choice.activationCode.size > sizeof(eim_pkg->u.ac.code)) {
			IPA_LOGP_ESIPA("GetEimPackage.ProfileDownloadTriggerRequest", LERROR,
				       "no space for over-long activation code!\n");
			goto error;
		}
		eim_pkg = IPA_ALLOC(struct ipa_esipa_eim_pkg);
		assert(eim_pkg);
		memcpy(eim_pkg->u.ac.code, pdd->choice.activationCode.buf, pdd->choice.activationCode.size);
		eim_pkg->u.ac.code[pdd->choice.activationCode.size] = '\0';
		eim_pkg->type = IPA_ESIPA_EIM_PKG_AC;
		break;
	case ProfileDownloadData_PR_contactDefaultSmdp:
		/* TODO (open question: is this an optional feature and do we have to support it?) */
		assert(NULL);
		break;
	case ProfileDownloadData_PR_contactSmds:
		/* TODO (open question: is this an optional feature and do we have to support it?) */
		assert(NULL);
		break;
	default:
		IPA_LOGP_ESIPA("GetEimPackage.ProfileDownloadTriggerRequest", LERROR,
			       "Profile download data is empty!\n");
		break;
	}

error:
	return eim_pkg;
}

static struct ipa_esipa_eim_pkg *dec_get_eim_pkg_req(struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	struct ipa_esipa_eim_pkg *eim_pkg = NULL;

	msg_to_ipa =
	    ipa_esipa_msg_to_ipa_dec(msg_to_ipa_encoded, "GetEimPackage",
				     EsipaMessageFromEimToIpa_PR_getEimPackageResponse);
	if (!msg_to_ipa)
		return NULL;

	switch (msg_to_ipa->choice.getEimPackageResponse.present) {
	case GetEimPackageResponse_PR_euiccPackageRequest:
		/* TODO */
		assert(NULL);
		break;
	case GetEimPackageResponse_PR_ipaEuiccDataRequest:
		/* TODO */
		assert(NULL);
		break;
	case GetEimPackageResponse_PR_profileDownloadTriggerRequest:
		eim_pkg =
		    dec_profile_dwnld_trig_req(&msg_to_ipa->choice.getEimPackageResponse.choice.
					       profileDownloadTriggerRequest);
		break;
	case GetEimPackageResponse_PR_eimPackageError:
		eim_pkg = IPA_ALLOC(struct ipa_esipa_eim_pkg);
		assert(eim_pkg);
		eim_pkg->u.error = msg_to_ipa->choice.getEimPackageResponse.choice.eimPackageError;
		eim_pkg->type = IPA_ESIPA_EIM_PKG_ERR;
		break;
	default:
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "eIM package is empty!\n");
		break;
	}

	ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa);
	return eim_pkg;
}

struct ipa_esipa_eim_pkg *ipa_esipa_get_eim_pkg(struct ipa_context *ctx, uint8_t *eid)
{
	struct ipa_buf *esipa_req;
	struct ipa_buf *esipa_res;
	struct ipa_esipa_eim_pkg *eim_pkg = NULL;

	IPA_LOGP_ESIPA("GetEimPackage", LINFO, "Requesting eIM package for eID: %s\n", ipa_hexdump(eid, IPA_LEN_EID));

	esipa_req = enc_get_eim_pkg_req(eid);
	esipa_res = ipa_esipa_req(ctx, esipa_req, "InitiateAuthentication");
	if (!esipa_res)
		goto error;

	eim_pkg = dec_get_eim_pkg_req(esipa_res);

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return eim_pkg;
}

void ipa_esipa_get_eim_pkg_free(struct ipa_esipa_eim_pkg *eim_pkg)
{
	if (!eim_pkg)
		return;

	/* we do not have any dynamically allocated members yet */
	switch (eim_pkg->type) {
	case IPA_ESIPA_EIM_PKG_AC:
	case IPA_ESIPA_EIM_PKG_ERR:
	default:
		break;
	}

	IPA_FREE(eim_pkg);
}
