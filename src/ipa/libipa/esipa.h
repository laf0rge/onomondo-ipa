#pragma once

#include <EsipaMessageFromEimToIpa.h>
#include <EsipaMessageFromIpaToEim.h>
struct ipa_buf;

#define IPA_LOGP_ESIPA(func, level, fmt, args...) \
	IPA_LOGP(SESIPA, level, "%s: " fmt, func, ## args)

#define IPA_ESIPA_URL_MAXLEN 1024
#define IPA_ESIPA_ASN_ENCODER_BUF_SIZE 5120

char *ipa_esipa_get_eim_url(struct ipa_context *ctx);
struct EsipaMessageFromEimToIpa *ipa_esipa_msg_to_ipa_dec(struct ipa_buf *msg_to_ipa_encoded, char *function_name,
							  enum EsipaMessageFromEimToIpa_PR epected_res_type);
struct ipa_buf *ipa_esipa_msg_to_eim_enc(struct EsipaMessageFromIpaToEim *msg_to_eim, char *function_name);
