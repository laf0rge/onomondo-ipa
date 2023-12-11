#pragma once

struct ipa_es10b_retr_notif_from_lst_req {
	struct RetrieveNotificationsListRequest req;
};

struct ipa_es10b_retr_notif_from_lst_res {
	struct RetrieveNotificationsListResponse *res;
	struct RetrieveNotificationsListResponse__notificationList *notification_list;
	long notif_lst_result_err;
};

struct ipa_es10b_retr_notif_from_lst_res *ipa_es10b_retr_notif_from_lst(struct ipa_context *ctx,
									const struct ipa_es10b_retr_notif_from_lst_req
									*req);
void ipa_es10b_retr_notif_from_lst_res_free(struct ipa_es10b_retr_notif_from_lst_res *res);
