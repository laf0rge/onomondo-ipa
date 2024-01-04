/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.22, 5.7.10: Function (ES10b): RetrieveNotificationsList
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
#include "esipa.h"
#include "es10b_retr_notif_from_lst.h"

static const struct num_str_map error_code_strings[] = {
	{ RetrieveNotificationsListResponse__notificationsListResultError_undefinedError, "undefinedError" },
	{ 0, NULL }
};

static int dec_retr_notif_from_lst_res(struct ipa_es10b_retr_notif_from_lst_res *res, const struct ipa_buf *es10b_res)
{
	struct RetrieveNotificationsListResponse *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_RetrieveNotificationsListResponse, es10b_res, "RetrieveNotificationsList");
	if (!asn)
		return -EINVAL;

	switch (asn->present) {
	case RetrieveNotificationsListResponse_PR_notificationList:
		res->notification_list = &asn->choice.notificationList;
		break;
	case RetrieveNotificationsListResponse_PR_notificationsListResultError:
		res->notif_lst_result_err = asn->choice.notificationsListResultError;
		IPA_LOGP_ES10X("RetrieveNotificationsList", LERROR, "function failed with error code %ld=%s!\n",
			       res->notif_lst_result_err, ipa_str_from_num(error_code_strings,
									   res->notif_lst_result_err, "(unknown)"));
		break;
	default:
		IPA_LOGP_ES10X("RetrieveNotificationsList", LERROR, "unexpected response content!\n");
		res->notif_lst_result_err = -1;
	}

	if (res->notification_list) {
		res->sgp32_notification_list =
		    IPA_ALLOC(struct SGP32_RetrieveNotificationsListResponse__notificationList);
		memset(res->sgp32_notification_list, 0, sizeof(*res->sgp32_notification_list));
		ipa_convert_notification_list(res->sgp32_notification_list, res->notification_list);
	} else {
		res->sgp32_notification_list = NULL;
	}

	res->res = asn;
	return 0;
}

static struct ipa_buf *enc_retr_notif_from_lst_req(const struct ipa_es10b_retr_notif_from_lst_req *req)
{
	struct RetrieveNotificationsListRequest asn = { 0 };
	struct RetrieveNotificationsListRequest__searchCriteria search_criteria = { 0 };
	struct ipa_buf *es10b_req = NULL;

	if (req->dr_search_criteria) {
		/* Convert from foreigen searchCriteria (see comment in header file) */
		switch (req->dr_search_criteria->present) {
		case IpaEuiccDataRequest__searchCriteria_PR_seqNumber:
			search_criteria.present = RetrieveNotificationsListRequest__searchCriteria_PR_seqNumber;
			search_criteria.choice.seqNumber = req->dr_search_criteria->choice.seqNumber;
			break;
		case IpaEuiccDataRequest__searchCriteria_PR_profileManagementOperation:
			search_criteria.present =
			    RetrieveNotificationsListRequest__searchCriteria_PR_profileManagementOperation;
			search_criteria.choice.profileManagementOperation =
			    req->dr_search_criteria->choice.profileManagementOperation;
			break;
		case IpaEuiccDataRequest__searchCriteria_PR_euiccPackageResults:
			IPA_LOGP_ES10X("RetrieveNotificationsList", LERROR,
				       "unsupported searchCriteria in IpaEuiccDataRequest!\n");
			search_criteria.present = RetrieveNotificationsListRequest__searchCriteria_PR_NOTHING;
			break;
		default:
			IPA_LOGP_ES10X("RetrieveNotificationsList", LERROR,
				       "empty searchCriteria in IpaEuiccDataRequest!\n");
			search_criteria.present = RetrieveNotificationsListRequest__searchCriteria_PR_NOTHING;
			break;
		}
	} else {
		/* Use native search_criteria as provided by caller */
		search_criteria = req->search_criteria;
	}

	if (req->search_criteria.present != RetrieveNotificationsListRequest__searchCriteria_PR_NOTHING)
		asn.searchCriteria = &search_criteria;
	es10b_req = ipa_es10x_req_enc(&asn_DEF_RetrieveNotificationsListRequest, &asn, "RetrieveNotificationsList");

	return es10b_req;
}

struct ipa_es10b_retr_notif_from_lst_res *ipa_es10b_retr_notif_from_lst(struct ipa_context *ctx, const struct ipa_es10b_retr_notif_from_lst_req
									*req)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_retr_notif_from_lst_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_retr_notif_from_lst_res);
	int rc;

	es10b_req = enc_retr_notif_from_lst_req(req);
	if (!es10b_req) {
		IPA_LOGP_ES10X("RetrieveNotificationsList", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10X("RetrieveNotificationsList", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_retr_notif_from_lst_res(res, es10b_res);
	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return res;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_retr_notif_from_lst_res_free(res);
	return NULL;
}

void ipa_es10b_retr_notif_from_lst_res_free(struct ipa_es10b_retr_notif_from_lst_res *res)
{
	if (!res)
		return;
	ipa_free_converted_notification_list(res->sgp32_notification_list);
	IPA_FREE(res->sgp32_notification_list);
	IPA_ES10X_RES_FREE(asn_DEF_RetrieveNotificationsListResponse, res);
}
