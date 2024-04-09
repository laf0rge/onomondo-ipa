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
#include "es10c_enable_prfle.h"
#include "es10c_disable_prfle.h"
#include "es10c_delete_prfle.h"
#include "es10c_get_prfle_info.h"

static void update_rollback_iccid(struct ipa_context *ctx)
{
	struct ipa_es10c_get_prfle_info_res *get_prfle_info_res = NULL;

	get_prfle_info_res = ipa_es10c_get_prfle_info(ctx, NULL);
	if (!get_prfle_info_res || get_prfle_info_res->prfle_info_list_err != 0) {
		IPA_LOGP(SIPA, LERROR, LERROR, "error reading profile info!\n");
		return;
	}

	if (get_prfle_info_res->res && get_prfle_info_res->currently_active_prfle->iccid) {
		IPA_FREE(ctx->rollback_iccid);
		ctx->rollback_iccid = IPA_BUF_FROM_ASN(get_prfle_info_res->currently_active_prfle->iccid);
		IPA_LOGP(SIPA, LINFO, "will use ICCD:%s in case of profile rollback.\n",
			 ipa_buf_hexdump(ctx->rollback_iccid));
	} else {
		IPA_FREE(ctx->rollback_iccid);
		ctx->rollback_iccid = NULL;
		IPA_LOGP(SIPA, LINFO, "no profile active, profile rollback not possible.\n");
	}

	ipa_es10c_get_prfle_info_res_free(get_prfle_info_res);
}

static int dec_load_euicc_pkg_res(struct ipa_es10b_load_euicc_pkg_res *res, const struct ipa_buf *es10b_res)
{
	struct EuiccPackageResult *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_EuiccPackageResult, es10b_res, "LoadEuiccPackage");
	if (!asn)
		return -EINVAL;

	res->res = asn;
	return 0;
}

struct ipa_es10b_load_euicc_pkg_res *load_euicc_pkg(struct ipa_context *ctx,
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

struct EuiccResultData *iot_emo_do_enable_psmo(struct ipa_context *ctx, const struct Psmo__enable *enable_psmo)
{
	struct ipa_es10c_enable_prfle_req enable_prfle_req = { 0 };
	struct ipa_es10c_enable_prfle_res *enable_prfle_res = NULL;
	struct EuiccResultData *euicc_result_data = IPA_ALLOC_ZERO(struct EuiccResultData);

	/* With the IoT/PSMO interface we can only identify the profile via its ICCID */
	enable_prfle_req.req.profileIdentifier.present = EnableProfileRequest__profileIdentifier_PR_iccid;
	enable_prfle_req.req.profileIdentifier.choice.iccid = enable_psmo->iccid;

	/* There is no way to tell the eUICC via the IoT/PSMO interface when a refresh is required, in order to be safe
	 * here, we must assume that a refresh is always needed. */
	enable_prfle_req.req.refreshFlag = true;
	enable_prfle_res = ipa_es10c_enable_prfle(ctx, &enable_prfle_req);
	euicc_result_data->present = EuiccResultData_PR_enableResult;
	if (enable_prfle_res)
		euicc_result_data->choice.enableResult = enable_prfle_res->res->enableResult;
	else
		euicc_result_data->choice.enableResult = EnableProfileResult_undefinedError;

	ipa_es10c_enable_prfle_res_free(enable_prfle_res);
	return euicc_result_data;
}

struct EuiccResultData *iot_emo_do_disable_psmo(struct ipa_context *ctx, const struct Psmo__disable *disable_psmo)
{
	struct ipa_es10c_disable_prfle_req disable_prfle_req = { 0 };
	struct ipa_es10c_disable_prfle_res *disable_prfle_res = NULL;
	struct EuiccResultData *euicc_result_data = IPA_ALLOC_ZERO(struct EuiccResultData);

	/* With the IoT/PSMO interface we can only identify the profile via its ICCID */
	disable_prfle_req.req.profileIdentifier.present = DisableProfileRequest__profileIdentifier_PR_iccid;
	disable_prfle_req.req.profileIdentifier.choice.iccid = disable_psmo->iccid;

	/* There is no way to tell the eUICC via the IoT/PSMO interface when a refresh is required, in order to be safe
	 * here, we must assume that a refresh is always needed. */
	disable_prfle_req.req.refreshFlag = true;

	disable_prfle_res = ipa_es10c_disable_prfle(ctx, &disable_prfle_req);
	euicc_result_data->present = EuiccResultData_PR_disableResult;
	if (disable_prfle_res)
		euicc_result_data->choice.disableResult = disable_prfle_res->res->disableResult;
	else
		euicc_result_data->choice.disableResult = DisableProfileResult_undefinedError;

	ipa_es10c_disable_prfle_res_free(disable_prfle_res);
	return euicc_result_data;
}

struct EuiccResultData *iot_emo_do_delete_psmo(struct ipa_context *ctx, const struct Psmo__delete *delete_psmo)
{
	struct ipa_es10c_delete_prfle_req delete_prfle_req = { 0 };
	struct ipa_es10c_delete_prfle_res *delete_prfle_res = NULL;
	struct EuiccResultData *euicc_result_data = IPA_ALLOC_ZERO(struct EuiccResultData);

	/* With the IoT/PSMO interface we can only identify the profile via its ICCID */
	delete_prfle_req.req.present = DeleteProfileRequest_PR_iccid;
	delete_prfle_req.req.choice.iccid = delete_psmo->iccid;

	delete_prfle_res = ipa_es10c_delete_prfle(ctx, &delete_prfle_req);
	euicc_result_data->present = EuiccResultData_PR_deleteResult;
	if (delete_prfle_res)
		euicc_result_data->choice.deleteResult = delete_prfle_res->res->deleteResult;
	else
		euicc_result_data->choice.deleteResult = DeleteProfileResult_undefinedError;

	ipa_es10c_delete_prfle_res_free(delete_prfle_res);
	return euicc_result_data;
}

struct ipa_es10b_load_euicc_pkg_res *load_euicc_pkg_iot_emu(struct ipa_context *ctx,
							    const struct ipa_es10b_load_euicc_pkg_req *req)
{
	struct ipa_es10b_load_euicc_pkg_res *res = NULL;
	struct EuiccPackageResult *asn = NULL;
	const struct Psmo *psmo = NULL;
	struct EuiccResultData *psmo_result = NULL;
	struct ipa_buf eim_id = { 0 };
	struct ipa_buf euicc_sign_epr = { 0 };
	unsigned int i;

	IPA_LOGP_ES10X("LoadEuiccPackage", LINFO,
		       "IoT eUICC emulation active, executing ECOs and PSMOs by calling equivalent consumer eUICC ES10x functions...\n");

	/* Before executing an eUICC package, ensure that the rollback ICCID is up to date. */
	update_rollback_iccid(ctx);

	/* Setup an (emulated) EuiccPackageResult */
	res = IPA_ALLOC_ZERO(struct ipa_es10b_load_euicc_pkg_res);
	asn = IPA_ALLOC_ZERO(struct EuiccPackageResult);
	res->res = asn;
	asn->present = EuiccPackageResult_PR_euiccPackageResultSigned;
	ipa_buf_assign(&eim_id, ctx->eim_id, strlen(ctx->eim_id));
	IPA_COPY_IPA_BUF_TO_ASN(&asn->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.eimId, &eim_id);
	asn->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.counterValue = 0;
	asn->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.seqNumber = 0;
	ipa_buf_assign(&euicc_sign_epr, "", 0);	/* Return an empty signature as we are unable to sign anything here */
	IPA_COPY_IPA_BUF_TO_ASN(&asn->choice.euiccPackageResultSigned.euiccSignEPR, &euicc_sign_epr);

	/* Go through the list of PSMOs and ECOs and execute the corresponding iot_emo_do... functions */
	switch (req->req.euiccPackageSigned.euiccPackage.present) {
	case EuiccPackage_PR_psmoList:
		for (i = 0; i < req->req.euiccPackageSigned.euiccPackage.choice.psmoList.list.count; i++) {
			psmo = req->req.euiccPackageSigned.euiccPackage.choice.psmoList.list.array[i];
			psmo_result = NULL;
			switch (psmo->present) {
			case Psmo_PR_enable:
				psmo_result = iot_emo_do_enable_psmo(ctx, &psmo->choice.enable);
				break;
			case Psmo_PR_disable:
				psmo_result = iot_emo_do_disable_psmo(ctx, &psmo->choice.disable);
				break;
			case Psmo_PR_delete:
				psmo_result = iot_emo_do_delete_psmo(ctx, &psmo->choice.Delete);
				break;
			case Psmo_PR_listProfileInfo:
				/* TODO */
				assert(false);
				break;
			case Psmo_PR_getRAT:
				/* TODO */
				assert(false);
				break;
			case Psmo_PR_configureAutoEnable:
				/* TODO */
				assert(false);
				break;
			default:
				IPA_LOGP_ES10X("LoadEuiccPackage", LERROR, "ignoring invalid or unsupported PSMO!\n");
				break;
			}
			if (psmo_result)
				ASN_SEQUENCE_ADD(&asn->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.
						 euiccResult.list, psmo_result);
		}
		break;
	case EuiccPackage_PR_ecoList:
		/* TODO */
		assert(false);
		break;
	default:
		IPA_LOGP_ES10X("LoadEuiccPackage", LERROR,
			       "the eUICC package contained neither a psmoList, nor an ecoList!\n");
		goto error;
	}

	return res;
error:
	ipa_es10b_load_euicc_pkg_res_free(res);
	res = IPA_ALLOC_ZERO(struct ipa_es10b_load_euicc_pkg_res);
	asn = IPA_ALLOC_ZERO(struct EuiccPackageResult);
	res->res = asn;
	asn->present = EuiccPackageResult_PR_euiccPackageErrorUnsigned;
	asn->choice.euiccPackageErrorUnsigned;
	ipa_buf_assign(&eim_id, ctx->eim_id, strlen(ctx->eim_id));
	IPA_COPY_IPA_BUF_TO_ASN(&asn->choice.euiccPackageErrorUnsigned.eimId, &eim_id);
	return res;
}

/* Check if the euicc package that we have just executed has done any changes to the currently selected profile */
static bool check_for_profile_change(const struct EuiccPackageResult *res)
{
	unsigned int i;
	struct EuiccResultData *euicc_result_data;
	if (!res)
		return false;
	if (res->present != EuiccPackageResult_PR_euiccPackageResultSigned)
		return false;

	for (i = 0; i < res->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.euiccResult.list.count; i++) {
		euicc_result_data =
		    res->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.euiccResult.list.array[i];
		switch (euicc_result_data->present) {
		case EuiccResultData_PR_enableResult:
			if (euicc_result_data->choice.enableResult == EnableProfileResult_ok)
				return true;
			break;
		case EuiccResultData_PR_disableResult:
			if (euicc_result_data->choice.disableResult == DisableProfileResult_ok)
				return true;
			break;
		case EuiccResultData_PR_rollbackResult:
			if (euicc_result_data->choice.rollbackResult == RollbackProfileResult_ok)
				return true;
			break;
		}
	}

	return false;
}

/* Check if the euicc package contains an enable PSMO that has the rollback flag set */
static bool check_for_rollback_flag(const struct EuiccPackageRequest *req)
{
	unsigned int i;
	struct Psmo *psmo;
	if (!req)
		return false;
	if (req->euiccPackageSigned.euiccPackage.present != EuiccPackage_PR_psmoList)
		return false;

	for (i = 0; i < req->euiccPackageSigned.euiccPackage.choice.psmoList.list.count; i++) {
		psmo = req->euiccPackageSigned.euiccPackage.choice.psmoList.list.array[i];

		if (psmo->present == Psmo_PR_enable && psmo->choice.enable.rollbackFlag)
			return true;
	}

	return false;
}

/*! Function (ES10b): LoadEuiccPackage.
 *  \param[inout] ctx pointer to ipa_context.
 *  \param[in] req pointer to struct that holds the function parameters.
 *  \returns pointer newly allocated struct with function result, NULL on error. */
struct ipa_es10b_load_euicc_pkg_res *ipa_es10b_load_euicc_pkg(struct ipa_context *ctx,
							      const struct ipa_es10b_load_euicc_pkg_req *req)
{
	struct ipa_es10b_load_euicc_pkg_res *res;

	if (ctx->cfg->iot_euicc_emu_enabled)
		res = load_euicc_pkg_iot_emu(ctx, req);
	else
		res = load_euicc_pkg(ctx, req);

	res->profile_changed = check_for_profile_change(res->res);
	res->rollback_allowed = check_for_rollback_flag(&req->req);

	return res;
}

/*! Free results of function (ES10b): LoadEuiccPackage.
 *  \param[in] res pointer to function result. */
void ipa_es10b_load_euicc_pkg_res_free(struct ipa_es10b_load_euicc_pkg_res *res)
{
	IPA_ES10X_RES_FREE(asn_DEF_EuiccPackageResult, res);
}
