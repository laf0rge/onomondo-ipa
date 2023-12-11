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
#include <EuiccPackageRequest.h>
#include <EuiccPackageResult.h>
#include <RetrieveNotificationsListRequest.h>
#include <RetrieveNotificationsListResponse.h>
#include "context.h"
#include "utils.h"
#include "proc_euicc_pkg_dwnld_exec.h"
#include "esipa_get_eim_pkg.h"
#include "es10b_load_euicc_pkg.h"
#include "es10b_retr_notif_from_lst.h"
#include "esipa_prvde_eim_pkg_rslt.h"
#include "es10b_rm_notif_from_lst.h"

int ipa_proc_eucc_pkg_dwnld_exec(struct ipa_context *ctx)
{
	struct ipa_esipa_get_eim_pkg_res *get_eim_pkg_res = NULL;
	struct ipa_es10b_load_euicc_pkg_req load_euicc_pkg_req = { 0 };
	struct ipa_es10b_load_euicc_pkg_res *load_euicc_pkg_res = NULL;
	struct ipa_es10b_retr_notif_from_lst_req retr_notif_from_lst_req = { 0 };
	struct ipa_es10b_retr_notif_from_lst_res *retr_notif_from_lst_res = NULL;
	struct ipa_esipa_prvde_eim_pkg_rslt_req prvde_eim_pkg_rslt_req = { 0 };
	struct ipa_esipa_prvde_eim_pkg_rslt_res *prvde_eim_pkg_rslt_res = NULL;

	struct RetrieveNotificationsListRequest__searchCriteria search_criteria = { 0 };
	int rc;

	/* Step #1-#2 */
	/* TODO: turn hardcoded eID into a parameter */
	get_eim_pkg_res =
	    ipa_esipa_get_eim_pkg(ctx, (uint8_t *) "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F");
	if (!get_eim_pkg_res)
		goto error;
	else if (get_eim_pkg_res->eim_pkg_err)
		goto error;
	else if (!get_eim_pkg_res->euicc_package_request)
		goto error;

	/* Step #3-#8 */
	load_euicc_pkg_req.req = *get_eim_pkg_res->euicc_package_request;
	load_euicc_pkg_res = ipa_es10b_load_euicc_pkg(ctx, &load_euicc_pkg_req);
	if (!load_euicc_pkg_res)
		goto error;
	else if (!load_euicc_pkg_res->res)
		goto error;

	/* Step #9 */
	/* TODO: can we somehow put this part inside the ipa_es10b_retr_notif_from_lst function? */
	/* TODO: this is a conditional step, only when the eUICC package contained PSMOs we retrieve notifications */
	search_criteria.choice.seqNumber =
	    load_euicc_pkg_res->res->choice.euiccPackageResultSigned.euiccPackageResultDataSigned.seqNumber;
	search_criteria.present = RetrieveNotificationsListRequest__searchCriteria_PR_seqNumber;
	retr_notif_from_lst_req.req.searchCriteria = &search_criteria;
	retr_notif_from_lst_res = ipa_es10b_retr_notif_from_lst(ctx, &retr_notif_from_lst_req);
	if (!retr_notif_from_lst_res)
		goto error;
	else if (retr_notif_from_lst_res->notif_lst_result_err)
		goto error;
	else if (!retr_notif_from_lst_res->notification_list)
		goto error;

	/* Step #10-#14 */
	prvde_eim_pkg_rslt_req.euicc_package_result = load_euicc_pkg_res->res;
	prvde_eim_pkg_rslt_req.notification_list = retr_notif_from_lst_res->notification_list;
	prvde_eim_pkg_rslt_res = ipa_esipa_prvde_eim_pkg_rslt(ctx, &prvde_eim_pkg_rslt_req);
	if (!prvde_eim_pkg_rslt_res)
		goto error;

	/* Step #15-17 (ES10b.RemoveNotificationFromList) */
	/* TODO: this may be called multiple times, depending on the acknowledgements in prvde_eim_pkg_rslt_res. */
	rc = ipa_es10b_rm_notif_from_lst(ctx,
					 load_euicc_pkg_res->res->choice.euiccPackageResultSigned.
					 euiccPackageResultDataSigned.seqNumber);
	if (rc < 0)
		goto error;

	/* TODO: Do we need to report an error to the eIM? */
error:
	ipa_esipa_get_eim_pkg_free(get_eim_pkg_res);
	ipa_es10b_load_euicc_pkg_res_free(load_euicc_pkg_res);
	ipa_es10b_retr_notif_from_lst_res_free(retr_notif_from_lst_res);
	ipa_esipa_prvde_eim_pkg_rslt_free(prvde_eim_pkg_rslt_res);

	return 0;
}
