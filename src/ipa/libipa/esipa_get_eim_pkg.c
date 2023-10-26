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
	asn_enc_rval_t rc;
	struct ipa_buf *buf_encoded = ipa_buf_alloc(1024);

	memset(&msg_to_eim, 0, sizeof(msg_to_eim));
	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_getEimPackageRequest;
	msg_to_eim.choice.getEimPackageRequest.eidValue.buf = eid_value;
	msg_to_eim.choice.getEimPackageRequest.eidValue.size = IPA_LEN_EID;

	assert(buf_encoded);
	rc = der_encode(&asn_DEF_EsipaMessageFromIpaToEim, &msg_to_eim,
			ipa_asn1c_consume_bytes_cb, buf_encoded);

	if (rc.encoded <= 0) {
		IPA_FREE(buf_encoded);
		return NULL;
	}

	return buf_encoded;
}

static struct ipa_eim_pkg *dec_profile_dwnld_trig_req(ProfileDownloadTriggerRequest_t * pdtr)
{
	struct ipa_eim_pkg *eim_pkg = NULL;
	struct ProfileDownloadData *pdd;

	if (!pdtr->profileDownloadData) {
		IPA_LOGP_ESIPA("GetEimPackage.ProfileDownloadTriggerRequest", LERROR,
			 "request did not contain any profile download data.\n");
		return NULL;
	}
	pdd = pdtr->profileDownloadData;

	switch (pdd->present) {
	case ProfileDownloadData_PR_activationCode:
		if (pdd->choice.activationCode.size >
		    sizeof(eim_pkg->u.ac.code)) {
			IPA_LOGP_ESIPA("GetEimPackage.ProfileDownloadTriggerRequest", LERROR,
				 "no space for over-long activation code!\n");
			goto error;
		}
		eim_pkg = IPA_ALLOC(struct ipa_eim_pkg);
		assert(eim_pkg);
		memcpy(eim_pkg->u.ac.code, pdd->choice.activationCode.buf,
		       pdd->choice.activationCode.size);
		eim_pkg->u.ac.code[pdd->choice.activationCode.size] = '\0';
		eim_pkg->type = IPA_EIM_PKG_AC;
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

static struct ipa_eim_pkg *dec_get_eim_pkg_req(struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	asn_dec_rval_t rc;
	struct ipa_eim_pkg *eim_pkg = NULL;

	if (msg_to_ipa_encoded->len == 0) {
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "eIM response contained no data!\n");
		return NULL;
	}

	rc = ber_decode(0, &asn_DEF_EsipaMessageFromEimToIpa,
			(void **)&msg_to_ipa, msg_to_ipa_encoded->data,
			msg_to_ipa_encoded->len);

	if (rc.code != RC_OK) {
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "cannot decode eIM response!\n");
		return NULL;
	}

#ifdef IPA_DEBUG_ASN1
	asn_fprint(stdout, &asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa);
#endif

	if (msg_to_ipa->present !=
	    EsipaMessageFromEimToIpa_PR_getEimPackageResponse) {
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "eIM response is not an eIM package\n");
		goto error;
	}

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
		eim_pkg = IPA_ALLOC(struct ipa_eim_pkg);
		assert(eim_pkg);
		eim_pkg->u.error =
		    msg_to_ipa->choice.getEimPackageResponse.choice.
		    eimPackageError;
		eim_pkg->type = IPA_EIM_PKG_ERR;
		break;
	default:
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "eIM package is empty!\n");
		break;
	}

error:
	ASN_STRUCT_FREE(asn_DEF_EsipaMessageFromEimToIpa, msg_to_ipa);
	return eim_pkg;
}

struct ipa_eim_pkg *ipa_esipa_get_eim_pkg(struct ipa_context *ctx, uint8_t *eid)
{

	struct ipa_buf *esipa_req;
	struct ipa_buf *esipa_res;
	struct ipa_eim_pkg *eim_pkg = NULL;
	int rc;

	IPA_LOGP_ESIPA("GetEimPackage", LINFO, "Requesting eIM package for eID:%s...\n",
		 ipa_hexdump(eid, IPA_LEN_EID));

	esipa_req = enc_get_eim_pkg_req(eid);
	esipa_res = ipa_buf_alloc(IPA_LIMIT_HTTP_REQ);
	rc = ipa_http_req(ctx->http_ctx, esipa_res, esipa_req, ipa_esipa_get_eim_url(ctx));
	if (rc < 0) {
		IPA_LOGP_ESIPA("GetEimPackage", LERROR, "eIM package request failed!\n");
		goto error;
	}

	eim_pkg = dec_get_eim_pkg_req(esipa_res);

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return eim_pkg;
}

void ipa_esipa_get_eim_pkg_dump(struct ipa_eim_pkg *eim_pkg, uint8_t indent,
				enum log_subsys log_subsys, enum log_level log_level)
{
	char indent_str[256];

	memset(indent_str, ' ', indent);
	indent_str[indent] = '\0';

	IPA_LOGP(log_subsys, log_level, "%seIM package: \n", indent_str);

	if (!eim_pkg) {
		IPA_LOGP(log_subsys, log_level, "%s (none)\n", indent_str);
		return;
	}

	switch (eim_pkg->type) {
	case IPA_EIM_PKG_AC:
		IPA_LOGP(log_subsys, log_level, "%s activation-code: \"%s\"\n",
			 indent_str, eim_pkg->u.ac.code);
		break;
	case IPA_EIM_PKG_ERR:
		IPA_LOGP(log_subsys, log_level, "%s error-code: \"%d\"\n",
			 indent_str, eim_pkg->u.error);
		break;
	default:
		IPA_LOGP(log_subsys, log_level,
			 "%s (unknown eIM package type)\n", indent_str);
		break;
	}

}

void ipa_esipa_get_eim_pkg_free(struct ipa_eim_pkg *eim_pkg)
{
	if (!eim_pkg)
		return;

	/* we do not have any dynamically allocated members yet */
	switch (eim_pkg->type) {
	case IPA_EIM_PKG_AC:
	case IPA_EIM_PKG_ERR:
	default:
		break;
	}

	IPA_FREE(eim_pkg);
}
