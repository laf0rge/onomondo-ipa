/*
 * Author: Philipp Maier
 * See also: GSMA SGP.32, section 3.1.1.1: eIM Package Retrieval
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include "context.h"
#include "utils.h"
#include "esipa_get_eim_pkg.h"
#include "proc_cmn_mtl_auth.h"
#include "proc_cmn_cancel_sess.h"
#include "proc_direct_prfle_dwnld.h"
#include "proc_euicc_pkg_dwnld_exec.h"
#include "proc_euicc_data_req.h"
#include "proc_eim_pkg_retr.h"

int ipa_proc_eim_pkg_retr(struct ipa_context *ctx)
{
	struct ipa_esipa_get_eim_pkg_res *get_eim_pkg_res = NULL;

	/* Poll eIM */
	get_eim_pkg_res = ipa_esipa_get_eim_pkg(ctx, ctx->eid);
	if (!get_eim_pkg_res)
		goto error;
	else if (get_eim_pkg_res->eim_pkg_err)
		goto error;

	/* Relay package contents to suitable handler procedure */
	if (get_eim_pkg_res->euicc_package_request)
		ipa_proc_eucc_pkg_dwnld_exec(ctx, get_eim_pkg_res->euicc_package_request);
	else if (get_eim_pkg_res->ipa_euicc_data_request) {
		struct ipa_proc_euicc_data_req_pars euicc_data_req_pars = { 0 };
		euicc_data_req_pars.ipa_euicc_data_request = get_eim_pkg_res->ipa_euicc_data_request;
		ipa_proc_euicc_data_req(ctx, &euicc_data_req_pars);
	} else if (get_eim_pkg_res->dwnld_trigger_request) {
		struct ipa_proc_direct_prfle_dwnlod_pars direct_prfle_dwnlod_pars = { 0 };
		struct ipa_buf allowed_ca;

		if (!get_eim_pkg_res->dwnld_trigger_request->profileDownloadData) {
			/* TODO: Perhaps there is a way to continue anyway. The ProfileDownloadTriggerRequest still
			 * contains a eimTransactionId, which is also optional. Maybe this eimTransactionId can be
			 * used to retrieve some context from somewhere that may allow us to continue. */
			IPA_LOGP(SIPA, LERROR,
				 "the ProfileDownloadTriggerRequest does not contain ProfileDownloadData -- cannot continue!\n");
			goto error;
		}
		if (get_eim_pkg_res->dwnld_trigger_request->profileDownloadData->present !=
		    ProfileDownloadData_PR_activationCode) {
			/* TODO: Perhaps there is a way to continue anyway. The ProfileDownloadData may alternatively
			 * contain a contactDefaultSmdp flag or an contactSmds information. Maybe we can just ask the
			 * SM-DP/SM-DS for more context information? */
			IPA_LOGP(SIPA, LERROR,
				 "the ProfileDownloadData does not contain an activationCode -- cannot continue!\n");
			goto error;
		}
		ipa_buf_assign(&allowed_ca, ctx->cfg->allowed_ca, IPA_LEN_ALLOWED_CA);
		direct_prfle_dwnlod_pars.allowed_ca = &allowed_ca;
		direct_prfle_dwnlod_pars.tac = ctx->cfg->tac;
		direct_prfle_dwnlod_pars.ac =
		    IPA_STR_FROM_ASN(&get_eim_pkg_res->dwnld_trigger_request->profileDownloadData->
				     choice.activationCode);
		ipa_proc_direct_prfle_dwnlod(ctx, &direct_prfle_dwnlod_pars);
		IPA_FREE((void *)direct_prfle_dwnlod_pars.ac);
	} else {
		IPA_LOGP(SIPA, LERROR,
			 "the GetEimPackageResponse contains an unsupported request -- cannot continue!\n");
		goto error;
	}

	ipa_esipa_get_eim_pkg_free(get_eim_pkg_res);
	IPA_LOGP(SIPA, LINFO, "eIM Package Retrieval succeded!\n");
	return 0;
error:
	ipa_esipa_get_eim_pkg_free(get_eim_pkg_res);
	IPA_LOGP(SIPA, LINFO, "eIM Package Retrieval failed!\n");
	return -EINVAL;
}
