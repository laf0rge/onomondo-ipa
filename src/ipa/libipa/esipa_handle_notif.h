#pragma once

#include <ProfileInstallationResult.h>
struct ipa_context;

struct ipa_esipa_handle_notif_req {
	const struct ProfileInstallationResult *profile_installation_result;
};

int ipa_esipa_handle_notif(struct ipa_context *ctx, const struct ipa_esipa_handle_notif_req *req);
