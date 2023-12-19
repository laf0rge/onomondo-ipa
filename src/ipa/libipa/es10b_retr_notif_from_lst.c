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
#include <RetrieveNotificationsListRequest.h>
#include <RetrieveNotificationsListResponse.h>
#include "es10b_retr_notif_from_lst.h"

static const struct num_str_map error_code_strings[] = {
	{ RetrieveNotificationsListResponse__notificationsListResultError_undefinedError, "undefinedError" },
	{ 0, NULL }
};

static int dec_retr_notif_from_lst_res(struct ipa_es10b_retr_notif_from_lst_res *res, struct ipa_buf *es10b_res)
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

	res->res = asn;
	return 0;
}

struct ipa_es10b_retr_notif_from_lst_res *ipa_es10b_retr_notif_from_lst(struct ipa_context *ctx,
									const struct ipa_es10b_retr_notif_from_lst_req
									*req)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_retr_notif_from_lst_res *res = IPA_ALLOC_ZERO(struct ipa_es10b_retr_notif_from_lst_res);
	int rc;

	es10b_req =
	    ipa_es10x_req_enc(&asn_DEF_RetrieveNotificationsListRequest, &req->req, "RetrieveNotificationsList");
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
	IPA_ES10X_RES_FREE(asn_DEF_RetrieveNotificationsListResponse, res);
}
