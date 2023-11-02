/*
 * Author: Philipp Maier
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
#include "es10b.h"
#include "es10b_get_euicc_info.h"
#include <GetEuiccInfo1Request.h>
#include <GetEuiccInfo2Request.h>
#include <EUICCInfo1.h>
#include <EUICCInfo2.h>

static struct ipa_buf *enc_get_euicc_info1(void)
{
	struct GetEuiccInfo1Request asn = { 0 };
	asn_enc_rval_t rc;
	struct ipa_buf *buf_encoded = ipa_buf_alloc(32);

	assert(buf_encoded);
	rc = der_encode(&asn_DEF_GetEuiccInfo1Request, &asn, ipa_asn1c_consume_bytes_cb, buf_encoded);

	if (rc.encoded <= 0) {
		IPA_FREE(buf_encoded);
		return NULL;
	}

	return buf_encoded;
}

static int dec_get_euicc_info1(struct ipa_es10b_euicc_info *euicc_info, struct ipa_buf *es10b_res)
{
	asn_dec_rval_t rc;
	struct EUICCInfo1 *asn = NULL;
	int i;

	rc = ber_decode(0, &asn_DEF_EUICCInfo1, (void **)&asn, es10b_res->data, es10b_res->len);
	if (rc.code != RC_OK) {
		IPA_LOGP_ES10B("GetEuiccInfo1Request", LERROR, "cannot decode eUICC response! (invalid asn1c)\n");
		ASN_STRUCT_FREE(asn_DEF_EUICCInfo1, asn);
		return -EINVAL;
	}
#ifdef IPA_DEBUG_ASN1
	ipa_asn1c_dump(&asn_DEF_EUICCInfo1, asn, 0, SESIPA, LINFO);
#endif

	IPA_COPY_ASN_BUF(euicc_info->svn, &asn->svn);

	for (i = 0; i < asn->euiccCiPKIdListForVerification.list.count; i++) {
		euicc_info->ci_pkid_verf[i] = IPA_BUF_FROM_ASN(asn->euiccCiPKIdListForVerification.list.array[i]);
		euicc_info->ci_pkid_verf_count = i + 1;
	}

	for (i = 0; i < asn->euiccCiPKIdListForSigning.list.count; i++) {
		euicc_info->ci_pkid_sign[i] = IPA_BUF_FROM_ASN(asn->euiccCiPKIdListForSigning.list.array[i]);
		euicc_info->ci_pkid_sign_count = i + 1;
	}

	ASN_STRUCT_FREE(asn_DEF_EUICCInfo1, asn);
	return 0;
}

struct ipa_es10b_euicc_info *get_euicc_info1(struct ipa_context *ctx)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_euicc_info *euicc_info;
	int rc;

	euicc_info = IPA_ALLOC(struct ipa_es10b_euicc_info);
	memset(euicc_info, 0, sizeof(*euicc_info));

	/* Request minimal set of the eUICC information */
	es10b_req = enc_get_euicc_info1();
	if (!es10b_req) {
		IPA_LOGP_ES10B("GetEuiccInfo1Request", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10B("GetEuiccInfo1Request", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_euicc_info1(euicc_info, es10b_res);

	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return euicc_info;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_get_euicc_info_free(euicc_info);
	return NULL;
}

static int dec_get_euicc_info2(struct ipa_es10b_euicc_info *euicc_info, struct ipa_buf *es10b_res)
{
	asn_dec_rval_t rc;
	struct EUICCInfo2 *asn = NULL;
	int i;

	rc = ber_decode(0, &asn_DEF_EUICCInfo2, (void **)&asn, es10b_res->data, es10b_res->len);
	if (rc.code != RC_OK) {
		IPA_LOGP_ES10B("GetEuiccInfo2Request", LERROR, "cannot decode eUICC response! (invalid asn1c)\n");
		ASN_STRUCT_FREE(asn_DEF_EUICCInfo2, asn);
		return -EINVAL;
	}
#ifdef IPA_DEBUG_ASN1
	ipa_asn1c_dump(&asn_DEF_EUICCInfo2, asn, 0, SESIPA, LINFO);
#endif

	IPA_COPY_ASN_BUF(euicc_info->svn, &asn->svn);

	for (i = 0; i < asn->euiccCiPKIdListForVerification.list.count; i++) {
		euicc_info->ci_pkid_verf[i] = IPA_BUF_FROM_ASN(asn->euiccCiPKIdListForVerification.list.array[i]);
		euicc_info->ci_pkid_verf_count = i + 1;
	}

	for (i = 0; i < asn->euiccCiPKIdListForSigning.list.count; i++) {
		euicc_info->ci_pkid_sign[i] = IPA_BUF_FROM_ASN(asn->euiccCiPKIdListForSigning.list.array[i]);
		euicc_info->ci_pkid_sign_count = i + 1;
	}

	IPA_COPY_ASN_BUF(euicc_info->profile_version, &asn->profileVersion);
	IPA_COPY_ASN_BUF(euicc_info->firmware_version, &asn->euiccFirmwareVer);
	if (asn->ts102241Version) {
		IPA_COPY_ASN_BUF(euicc_info->ts_102241_version, asn->ts102241Version);
		euicc_info->ts_102241_version_present = true;
	}
	if (asn->globalplatformVersion) {
		IPA_COPY_ASN_BUF(euicc_info->gp_version, asn->globalplatformVersion);
		euicc_info->gp_version_present = true;
	}
	IPA_COPY_ASN_BUF(euicc_info->pp_version, &asn->ppVersion);

	euicc_info->ext_card_resource = IPA_BUF_FROM_ASN(&asn->extCardResource);
	euicc_info->uicc_capability = IPA_BUF_FROM_ASN(&asn->uiccCapability);
	euicc_info->rsp_capability = IPA_BUF_FROM_ASN(&asn->rspCapability);

	if (asn->euiccCategory) {
		euicc_info->euicc_category = *asn->euiccCategory;
		euicc_info->euicc_category_present = true;
	}

	if (asn->forbiddenProfilePolicyRules) {
		euicc_info->forb_profile_policy_rules = IPA_BUF_FROM_ASN(asn->forbiddenProfilePolicyRules);
		euicc_info->forb_profile_policy_rules_present = true;
	}

	euicc_info->sas_acreditation_number = IPA_STR_FROM_ASN(&asn->sasAcreditationNumber);

	if (asn->certificationDataObject) {
		euicc_info->cert_platform_label = IPA_STR_FROM_ASN(&asn->certificationDataObject->platformLabel);
		euicc_info->cert_discovery_base_url = IPA_STR_FROM_ASN(&asn->certificationDataObject->discoveryBaseURL);
		euicc_info->cert_data_present = true;
	}

	ASN_STRUCT_FREE(asn_DEF_EUICCInfo2, asn);
	return 0;
}

static struct ipa_buf *enc_get_euicc_info2(void)
{
	struct GetEuiccInfo2Request asn = { 0 };
	asn_enc_rval_t rc;
	struct ipa_buf *buf_encoded = ipa_buf_alloc(32);

	assert(buf_encoded);
	rc = der_encode(&asn_DEF_GetEuiccInfo2Request, &asn, ipa_asn1c_consume_bytes_cb, buf_encoded);

	if (rc.encoded <= 0) {
		IPA_FREE(buf_encoded);
		return NULL;
	}

	return buf_encoded;
}

struct ipa_es10b_euicc_info *get_euicc_info2(struct ipa_context *ctx)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct ipa_es10b_euicc_info *euicc_info;
	int rc;

	euicc_info = IPA_ALLOC(struct ipa_es10b_euicc_info);
	memset(euicc_info, 0, sizeof(*euicc_info));
	euicc_info->full = true;

	/* Request full set of the eUICC information */
	es10b_req = enc_get_euicc_info2();
	if (!es10b_req) {
		IPA_LOGP_ES10B("GetEuiccInfo2Request", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10B("GetEuiccInfo2Request", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_euicc_info2(euicc_info, es10b_res);

	if (rc < 0)
		goto error;

	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return euicc_info;
error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	ipa_es10b_get_euicc_info_free(euicc_info);
	return NULL;
}

/*! Request eUICC info from eUICC.
 *  \param[in] ctx pointer to IPA context.
 *  \param[in] full set to true to request EUICCInfo2 instead of EUICCInfo1.
 *  \returns struct with parsed eUICC info on success, NULL on failure. */
struct ipa_es10b_euicc_info *ipa_es10b_get_euicc_info(struct ipa_context *ctx, bool full)
{
	if (full)
		return get_euicc_info2(ctx);
	else
		return get_euicc_info1(ctx);
}

/*! Dump eUICC info.
 *  \param[in] euicc_info pointer to struct that holds the eUICC info. */
void ipa_es10b_get_euicc_info_dump(struct ipa_es10b_euicc_info *euicc_info,
				   uint8_t indent, enum log_subsys log_subsys, enum log_level log_level)
{
	char indent_str[256];
	int i;
	struct ipa_buf *entry;

	memset(indent_str, ' ', indent);
	indent_str[indent] = '\0';

	IPA_LOGP(log_subsys, log_level, "%seUICC info: \n", indent_str);

	if (!euicc_info) {
		IPA_LOGP(log_subsys, log_level, "%s (none)\n", indent_str);
		return;
	}

	IPA_LOGP(log_subsys, log_level, "%s svn: \"%s\"\n", indent_str,
		 ipa_hexdump(euicc_info->svn, sizeof(euicc_info->svn)));
	IPA_LOGP(log_subsys, log_level, "%s CI PKID List (for verification):\n", indent_str);
	if (euicc_info->ci_pkid_verf_count == 0)
		IPA_LOGP(log_subsys, log_level, "%s  (empty)\n", indent_str);
	for (i = 0; i < euicc_info->ci_pkid_verf_count; i++) {
		entry = euicc_info->ci_pkid_verf[i];
		IPA_LOGP(log_subsys, log_level, "%s  %d: \"%s\"\n", indent_str, i, ipa_buf_hexdump(entry));
	}

	IPA_LOGP(log_subsys, log_level, "%s CI PKID List (for signing):\n", indent_str);
	if (euicc_info->ci_pkid_sign_count == 0)
		IPA_LOGP(log_subsys, log_level, "%s  (empty)\n", indent_str);
	for (i = 0; i < euicc_info->ci_pkid_sign_count; i++) {
		entry = euicc_info->ci_pkid_sign[i];
		IPA_LOGP(log_subsys, log_level, "%s  %d: \"%s\"\n", indent_str, i, ipa_buf_hexdump(entry));
	}

	if (!euicc_info->full)
		return;

	IPA_LOGP(log_subsys, log_level, "%s profile version: \"%s\"\n",
		 indent_str, ipa_hexdump(euicc_info->profile_version, sizeof(euicc_info->profile_version)));
	IPA_LOGP(log_subsys, log_level, "%s firmware version: \"%s\"\n",
		 indent_str, ipa_hexdump(euicc_info->firmware_version, sizeof(euicc_info->firmware_version)));
	if (euicc_info->ts_102241_version_present) {
		IPA_LOGP(log_subsys, log_level,
			 "%s TS 102241 version: \"%s\"\n", indent_str,
			 ipa_hexdump(euicc_info->ts_102241_version, sizeof(euicc_info->ts_102241_version)));
	} else {
		IPA_LOGP(log_subsys, log_level, "%s TS 102241 version: (not included)\n", indent_str);
	}
	if (euicc_info->gp_version_present) {
		IPA_LOGP(log_subsys, log_level, "%s GP version: \"%s\"\n",
			 indent_str, ipa_hexdump(euicc_info->gp_version, sizeof(euicc_info->gp_version)));
	} else {
		IPA_LOGP(log_subsys, log_level, "%s GP version: (not included)\n", indent_str);
	}
	IPA_LOGP(log_subsys, log_level, "%s PP version: \"%s\"\n", indent_str,
		 ipa_hexdump(euicc_info->pp_version, sizeof(euicc_info->pp_version)));

	IPA_LOGP(log_subsys, log_level, "%s ext. card resource: \"%s\"\n",
		 indent_str, ipa_buf_hexdump(euicc_info->ext_card_resource));

	IPA_LOGP(log_subsys, log_level, "%s uicc capability: \"%s\"\n",
		 indent_str, ipa_buf_hexdump(euicc_info->uicc_capability));

	IPA_LOGP(log_subsys, log_level, "%s rsp capability: \"%s\"\n",
		 indent_str, ipa_buf_hexdump(euicc_info->rsp_capability));

	if (euicc_info->euicc_category_present) {
		IPA_LOGP(log_subsys, log_level, "%s rsp category: \"%ld\"\n", indent_str, euicc_info->euicc_category);
	}

	if (euicc_info->forb_profile_policy_rules_present) {
		IPA_LOGP(log_subsys, log_level,
			 "%s forbidden policy rules: \"%s\"\n", indent_str,
			 ipa_buf_hexdump(euicc_info->forb_profile_policy_rules));
	} else {
		IPA_LOGP(log_subsys, log_level, "%s forbidden policy rules: (not included)\n", indent_str);
	}

	IPA_LOGP(log_subsys, log_level, "%s SAS arcreditation number: \"%s\"\n",
		 indent_str, euicc_info->sas_acreditation_number);

	if (euicc_info->cert_data_present) {
		IPA_LOGP(log_subsys, log_level,
			 "%s cert platform label: \"%s\"\n", indent_str, euicc_info->cert_platform_label);
		IPA_LOGP(log_subsys, log_level,
			 "%s cert discovery base URL: \"%s\"\n", indent_str, euicc_info->cert_discovery_base_url);
	} else {
		IPA_LOGP(log_subsys, log_level, "%s cert platform label: (not included)\n", indent_str);
		IPA_LOGP(log_subsys, log_level, "%s cert discovery base URL: (not included)\n", indent_str);
	}
}

/*! Free eUICC info.
 *  \param[inout] euicc_info pointer to struct that holds the eUICC info. */
void ipa_es10b_get_euicc_info_free(struct ipa_es10b_euicc_info *euicc_info)
{
	int i;

	if (!euicc_info)
		return;

	for (i = 0; i < euicc_info->ci_pkid_verf_count; i++)
		IPA_FREE(euicc_info->ci_pkid_verf[i]);

	for (i = 0; i < euicc_info->ci_pkid_sign_count; i++)
		IPA_FREE(euicc_info->ci_pkid_sign[i]);

	IPA_FREE(euicc_info->ext_card_resource);
	IPA_FREE(euicc_info->uicc_capability);
	IPA_FREE(euicc_info->rsp_capability);
	IPA_FREE(euicc_info->forb_profile_policy_rules);
	IPA_FREE(euicc_info->sas_acreditation_number);
	IPA_FREE(euicc_info->cert_platform_label);
	IPA_FREE(euicc_info->cert_discovery_base_url);

	IPA_FREE(euicc_info);
}
