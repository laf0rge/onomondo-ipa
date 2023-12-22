/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, section 3.3.1: Generic eUICC Package Download and Execution
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
#include "context.h"
#include "utils.h"
#include "esipa_get_eim_pkg.h"
#include "es10b_load_euicc_pkg.h"
#include "es10b_retr_notif_from_lst.h"
#include "esipa_prvde_eim_pkg_rslt.h"
#include "es10b_rm_notif_from_lst.h"
#include "proc_euicc_pkg_dwnld_exec.h"

static int remove_notifications(struct ipa_context *ctx, struct EimAcknowledgements *eim_acknowledgements)
{
	unsigned int i;
	int rc;

	/* In case the eIM Has not requested to remove any pending notifications, so there is nothing to do
	 * for us here. The eIM may use the "Notification Delivery to Notification Receivers" procedure later
	 * to remove the pending notifications later. */
	if (!eim_acknowledgements)
		return 0;

	for (i = 0; i < eim_acknowledgements->list.count; i++) {
		rc = ipa_es10b_rm_notif_from_lst(ctx, *eim_acknowledgements->list.array[i]);
		if (rc < 0)
			return -EINVAL;
	}

	return 0;
}

int ipa_proc_eucc_pkg_dwnld_exec(struct ipa_context *ctx, const struct EuiccPackageRequest *euicc_package_request)
{
	struct ipa_esipa_get_eim_pkg_res *get_eim_pkg_res = NULL;
	struct ipa_es10b_load_euicc_pkg_req load_euicc_pkg_req = { 0 };
	struct ipa_es10b_load_euicc_pkg_res *load_euicc_pkg_res = NULL;
	struct ipa_es10b_retr_notif_from_lst_req retr_notif_from_lst_req = { 0 };
	struct ipa_es10b_retr_notif_from_lst_res *retr_notif_from_lst_res = NULL;
	struct ipa_esipa_prvde_eim_pkg_rslt_req prvde_eim_pkg_rslt_req = { 0 };
	struct ipa_esipa_prvde_eim_pkg_rslt_res *prvde_eim_pkg_rslt_res = NULL;
	int rc;

	/* Step #3-#8 (ES10b.LoadEuiccPackage) */
	load_euicc_pkg_req.req = *euicc_package_request;
	load_euicc_pkg_res = ipa_es10b_load_euicc_pkg(ctx, &load_euicc_pkg_req);
	if (!load_euicc_pkg_res)
		goto error;
	else if (!load_euicc_pkg_res->res)
		goto error;

	/* Step #9 (ES10b.RetrieveNotificationsList) */
	/* TODO: this is a conditional step, only when the eUICC package contained PSMOs we retrieve notifications */
	retr_notif_from_lst_req.search_criteria.choice.seqNumber =
	    load_euicc_pkg_res->res->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.seqNumber;
	retr_notif_from_lst_req.search_criteria.present = RetrieveNotificationsListRequest__searchCriteria_PR_seqNumber;
	retr_notif_from_lst_res = ipa_es10b_retr_notif_from_lst(ctx, &retr_notif_from_lst_req);
	if (!retr_notif_from_lst_res)
		goto error;
	else if (retr_notif_from_lst_res->notif_lst_result_err)
		goto error;
	else if (!retr_notif_from_lst_res->notification_list)
		goto error;

	/* Step #10-#14 (ESipa.ProvideEimPackageResult) */
	prvde_eim_pkg_rslt_req.euicc_package_result = load_euicc_pkg_res->res;
	prvde_eim_pkg_rslt_req.notification_list = retr_notif_from_lst_res->notification_list;
	prvde_eim_pkg_rslt_res = ipa_esipa_prvde_eim_pkg_rslt(ctx, &prvde_eim_pkg_rslt_req);
	if (!prvde_eim_pkg_rslt_res)
		goto error;

	/* Step #15-17 (ES10b.RemoveNotificationFromList) */
	/* Remove the notification for the euiccPackageResult. */
	rc = ipa_es10b_rm_notif_from_lst(ctx,
					 load_euicc_pkg_res->res->choice.
					 euiccPackageResultSigned.euiccPackageResultDataSigned.seqNumber);
	if (rc < 0)
		goto error;
	/* Remove the notifications that the eIM has requested to remove in the provideEimPackageResultResponse. */
	rc = remove_notifications(ctx, prvde_eim_pkg_rslt_res->eim_acknowledgements);
	if (rc < 0)
		goto error;

	ipa_esipa_get_eim_pkg_free(get_eim_pkg_res);
	ipa_es10b_load_euicc_pkg_res_free(load_euicc_pkg_res);
	ipa_es10b_retr_notif_from_lst_res_free(retr_notif_from_lst_res);
	ipa_esipa_prvde_eim_pkg_rslt_free(prvde_eim_pkg_rslt_res);
	IPA_LOGP(SIPA, LINFO, "Generic eUICC Package Download and Execution succeded!\n");
	return 0;
error:
	/* TODO: Do we need to report an error to the eIM? */
	ipa_esipa_get_eim_pkg_free(get_eim_pkg_res);
	ipa_es10b_load_euicc_pkg_res_free(load_euicc_pkg_res);
	ipa_es10b_retr_notif_from_lst_res_free(retr_notif_from_lst_res);
	ipa_esipa_prvde_eim_pkg_rslt_free(prvde_eim_pkg_rslt_res);
	IPA_LOGP(SIPA, LERROR, "Generic eUICC Package Download and Execution failed!\n");
	return -EINVAL;
}
