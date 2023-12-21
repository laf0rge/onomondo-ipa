#pragma once

#include <RetrieveNotificationsListRequest.h>
#include <RetrieveNotificationsListResponse.h>
#include <IpaEuiccDataRequest.h>
#include <SGP32-RetrieveNotificationsListResponse.h>
struct ipa_context;

struct ipa_es10b_retr_notif_from_lst_req {
	struct RetrieveNotificationsListRequest__searchCriteria search_criteria;

	/* When the caller has access to a search searchCriteria that originates from an IpaEuiccDataRequest, the
	 * pointer to it may be passed here. The function will then atomatically convert the searchCriteria into
	 * the native RetrieveNotificationsListRequest format. */
	const struct IpaEuiccDataRequest__searchCriteria *dr_search_criteria;
};

struct ipa_es10b_retr_notif_from_lst_res {
	struct RetrieveNotificationsListResponse *res;
	struct RetrieveNotificationsListResponse__notificationList *notification_list;
	struct SGP32_RetrieveNotificationsListResponse__notificationList *sgp32_notification_list;
	long notif_lst_result_err;
};

struct ipa_es10b_retr_notif_from_lst_res *ipa_es10b_retr_notif_from_lst(struct ipa_context *ctx,
									const struct ipa_es10b_retr_notif_from_lst_req
									*req);
void ipa_es10b_retr_notif_from_lst_res_free(struct ipa_es10b_retr_notif_from_lst_res *res);
