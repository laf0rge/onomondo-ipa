/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, section 5.14.6: Function (ESipa): ProvideEimPackageResult
 *
 */

#include <stdint.h>
#include <onomondo/ipa/http.h>
#include <onomondo/ipa/log.h>
#include <onomondo/ipa/ipad.h>
#include <ProvideEimPackageResult.h>
#include <SGP32-RetrieveNotificationsListResponse.h>
#include <RetrieveNotificationsListResponse.h>
#include "utils.h"
#include "length.h"
#include "context.h"
#include "esipa.h"
#include "esipa_prvde_eim_pkg_rslt.h"

static void convert_notification_list(struct SGP32_RetrieveNotificationsListResponse__notificationList *lst_out,
				      const struct RetrieveNotificationsListResponse__notificationList *lst_in)
{
	unsigned int i;
	struct PendingNotification *pending_notif_item;
	struct SGP32_PendingNotification *sgp32_pending_notif_item;

	OtherSignedNotification_t *other_signed_notification;
	ProfileInstallationResultData_t *profile_Installation_result_data;
	EuiccSignPIR_t *euicc_sign_PIR;

	for (i = 0; i < lst_in->list.count; i++) {
		pending_notif_item = lst_in->list.array[i];

		switch (pending_notif_item->present) {
		case PendingNotification_PR_profileInstallationResult:
			profile_Installation_result_data =
			    &pending_notif_item->choice.profileInstallationResult.profileInstallationResultData;
			euicc_sign_PIR = &pending_notif_item->choice.profileInstallationResult.euiccSignPIR;

			sgp32_pending_notif_item = IPA_ALLOC(struct SGP32_PendingNotification);
			ASN_SEQUENCE_ADD(&lst_out->list, sgp32_pending_notif_item);
			sgp32_pending_notif_item->present = SGP32_PendingNotification_PR_profileInstallationResult;
			sgp32_pending_notif_item->choice.profileInstallationResult.profileInstallationResultData =
			    *profile_Installation_result_data;
			sgp32_pending_notif_item->choice.profileInstallationResult.euiccSignPIR = *euicc_sign_PIR;
			break;
		case PendingNotification_PR_otherSignedNotification:
			other_signed_notification = &pending_notif_item->choice.otherSignedNotification;

			sgp32_pending_notif_item = IPA_ALLOC(struct SGP32_PendingNotification);
			ASN_SEQUENCE_ADD(&lst_out->list, sgp32_pending_notif_item);
			sgp32_pending_notif_item->present = SGP32_PendingNotification_PR_otherSignedNotification;
			sgp32_pending_notif_item->choice.otherSignedNotification = *other_signed_notification;
			break;
		default:
			IPA_LOGP_ESIPA("ProvideEimPackageResult", LERROR, "skipping empty PendingNotification item\n");
			break;
		}
	}
}

static void free_converted_notification_list(struct SGP32_RetrieveNotificationsListResponse__notificationList *lst)
{
	int i;
	for (i = 0; i < lst->list.count; i++)
		IPA_FREE(lst->list.array[i]);
	IPA_FREE(lst->list.array);
}

static struct ipa_buf *enc_prvde_eim_pkg_rslt_req(const struct ipa_esipa_prvde_eim_pkg_rslt_req *req)
{
	struct EsipaMessageFromIpaToEim msg_to_eim = { 0 };
	struct ipa_buf *enc;
	bool notification_list_allocated = false;

	msg_to_eim.present = EsipaMessageFromIpaToEim_PR_provideEimPackageResult;

	if (req->eim_pkg_err != 0) {
		msg_to_eim.choice.provideEimPackageResult.present = ProvideEimPackageResult_PR_eimPackageError;
		msg_to_eim.choice.provideEimPackageResult.choice.eimPackageError = req->eim_pkg_err;
	} else if (req->euicc_package_result && req->notification_list) {
		msg_to_eim.choice.provideEimPackageResult.present = ProvideEimPackageResult_PR_ePRAndNotifications;
		msg_to_eim.choice.provideEimPackageResult.choice.ePRAndNotifications.euiccPackageResult =
		    *req->euicc_package_result;
		msg_to_eim.choice.provideEimPackageResult.choice.ePRAndNotifications.notificationList.present =
		    SGP32_RetrieveNotificationsListResponse_PR_notificationList;
		convert_notification_list(&msg_to_eim.choice.provideEimPackageResult.choice.ePRAndNotifications.
					  notificationList.choice.notificationList, req->notification_list);
		notification_list_allocated = true;
	} else if (req->euicc_package_result) {
		msg_to_eim.choice.provideEimPackageResult.present = ProvideEimPackageResult_PR_euiccPackageResult;
		msg_to_eim.choice.provideEimPackageResult.choice.euiccPackageResult = *req->euicc_package_result;
	} else if (req->ipa_euicc_data_resp) {
		msg_to_eim.choice.provideEimPackageResult.present = ProvideEimPackageResult_PR_ipaEuiccDataResponse;
		msg_to_eim.choice.provideEimPackageResult.choice.ipaEuiccDataResponse = *req->ipa_euicc_data_resp;
	} else if (req->prfle_dwnld_trig_req_rslt) {
		msg_to_eim.choice.provideEimPackageResult.present =
		    ProvideEimPackageResult_PR_profileDownloadTriggerResult;
		msg_to_eim.choice.provideEimPackageResult.choice.profileDownloadTriggerResult =
		    *req->prfle_dwnld_trig_req_rslt;
	} else {
		/* The struct should at least contain one of the above information element. In case the caller fails
		 * to fill out any of those, we fall back to setting eimPackageError to undefined. This will at least
		 * tell the eIM that something is wrong. */
		IPA_LOGP_ESIPA("ProvideEimPackageResult", LERROR,
			       "empty provideEimPackageResult request, setting eimPackageError to undefined\n");
		msg_to_eim.choice.provideEimPackageResult.present = ProvideEimPackageResult_PR_eimPackageError;
		msg_to_eim.choice.provideEimPackageResult.choice.eimPackageError =
		    ProvideEimPackageResult__eimPackageError_undefinedError;
	}

	enc = ipa_esipa_msg_to_eim_enc(&msg_to_eim, "ProvideEimPackageResult");

	if (notification_list_allocated)
		free_converted_notification_list(&msg_to_eim.choice.provideEimPackageResult.choice.ePRAndNotifications.
						 notificationList.choice.notificationList);

	return enc;
}

struct ipa_esipa_prvde_eim_pkg_rslt_res *dec_prvde_eim_pkg_rslt_res(const struct ipa_buf *msg_to_ipa_encoded)
{
	struct EsipaMessageFromEimToIpa *msg_to_ipa = NULL;
	struct ipa_esipa_prvde_eim_pkg_rslt_res *res = NULL;

	msg_to_ipa =
	    ipa_esipa_msg_to_ipa_dec(msg_to_ipa_encoded, "ProvideEimPackageResult",
				     EsipaMessageFromEimToIpa_PR_provideEimPackageResultResponse);
	if (!msg_to_ipa)
		return NULL;

	res = IPA_ALLOC_ZERO(struct ipa_esipa_prvde_eim_pkg_rslt_res);
	res->msg_to_ipa = msg_to_ipa;

	/* Optional, may be NULL */
	res->eim_acknowledgements = msg_to_ipa->choice.provideEimPackageResultResponse.eimAcknowledgements;

	return res;
}

struct ipa_esipa_prvde_eim_pkg_rslt_res *ipa_esipa_prvde_eim_pkg_rslt(struct ipa_context *ctx,
								      const struct ipa_esipa_prvde_eim_pkg_rslt_req
								      *req)
{
	struct ipa_buf *esipa_req = NULL;
	struct ipa_buf *esipa_res = NULL;
	struct ipa_esipa_prvde_eim_pkg_rslt_res *res = NULL;

	IPA_LOGP_ESIPA("ProvideEimPackageResult", LINFO,
		       "Providing eUICC package result and eUICC notifications to eIM\n");

	esipa_req = enc_prvde_eim_pkg_rslt_req(req);
	if (!esipa_req)
		goto error;

	esipa_res = ipa_esipa_req(ctx, esipa_req, "ProvideEimPackageResult");
	if (!esipa_res)
		goto error;

	res = dec_prvde_eim_pkg_rslt_res(esipa_res);
	if (!res)
		goto error;

error:
	IPA_FREE(esipa_req);
	IPA_FREE(esipa_res);
	return res;
}

void ipa_esipa_prvde_eim_pkg_rslt_free(struct ipa_esipa_prvde_eim_pkg_rslt_res *res)
{
	IPA_ESIPA_RES_FREE(res);
}
