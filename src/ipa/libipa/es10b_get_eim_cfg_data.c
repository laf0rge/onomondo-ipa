/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, 5.9.18: Function (ES10b): GetEimConfigurationData
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
#include <GetEimConfigurationDataRequest.h>
#include "context.h"
#include "utils.h"
#include "euicc.h"
#include "es10x.h"
#include "es10b_get_eim_cfg_data.h"

void convert_get_eim_cfg_data(struct ipa_es10b_eim_cfg_data *res)
{
	unsigned int i;
	asn_enc_rval_t rc;

	res->eim_cfg_data_list_count = res->res->eimConfigurationDataList.list.count;
	res->eim_cfg_data_list = IPA_ALLOC_N(sizeof(struct ipa_eim_cfg_data *) * res->eim_cfg_data_list_count);

	for (i = 0; i < res->eim_cfg_data_list_count; i++) {
		res->eim_cfg_data_list[i] = IPA_ALLOC_ZERO(struct ipa_eim_cfg_data);

		/* All members must be independently allocated since the caller may decide to take ownership of
		 * res->eim_cfg_data_list before freeing struct ipa_es10b_eim_cfg_data. */

		res->eim_cfg_data_list[i]->eim_id =
		    IPA_STR_FROM_ASN(&res->res->eimConfigurationDataList.list.array[i]->eimId);
		if (res->res->eimConfigurationDataList.list.array[i]->eimFqdn)
			res->eim_cfg_data_list[i]->eim_fqdn =
			    IPA_STR_FROM_ASN(res->res->eimConfigurationDataList.list.array[i]->eimFqdn);

		if (res->res->eimConfigurationDataList.list.array[i]->eimIdType) {
			res->eim_cfg_data_list[i]->eim_id_type = IPA_ALLOC(long);
			*res->eim_cfg_data_list[i]->eim_id_type =
			    *res->res->eimConfigurationDataList.list.array[i]->eimIdType;
		}
		if (res->res->eimConfigurationDataList.list.array[i]->counterValue) {
			res->eim_cfg_data_list[i]->counter_value = IPA_ALLOC(long);
			*res->eim_cfg_data_list[i]->counter_value =
			    *res->res->eimConfigurationDataList.list.array[i]->counterValue;
		}
		if (res->res->eimConfigurationDataList.list.array[i]->associationToken) {
			res->eim_cfg_data_list[i]->association_token = IPA_ALLOC(long);
			*res->eim_cfg_data_list[i]->association_token =
			    *res->res->eimConfigurationDataList.list.array[i]->associationToken;
		}

		if (res->res->eimConfigurationDataList.list.array[i]->eimPublicKeyData) {
			switch (res->res->eimConfigurationDataList.list.array[i]->eimPublicKeyData->present) {
			case EimConfigurationData__eimPublicKeyData_PR_eimPublicKey:
				res->eim_cfg_data_list[i]->eim_public_key_data.eim_public_key =
				    ipa_buf_alloc(IPA_ES10X_ASN_ENCODER_BUF_SIZE);
				rc = der_encode(&asn_DEF_SubjectPublicKeyInfo,
						&res->res->eimConfigurationDataList.list.array[i]->
						eimPublicKeyData->choice.eimPublicKey, ipa_asn1c_consume_bytes_cb,
						res->eim_cfg_data_list[i]->eim_public_key_data.eim_public_key);
				if (rc.encoded <= 0) {
					IPA_LOGP_ES10X("GetEimConfigurationData", LERROR,
						       "data format conversion failed, cannot re-encode eimPublicKey in eimPublicKeyData\n");
					IPA_FREE(res->eim_cfg_data_list[i]->eim_public_key_data.eim_public_key);
					res->eim_cfg_data_list[i]->eim_public_key_data.eim_public_key = NULL;
					return;
				}
				break;
			case EimConfigurationData__eimPublicKeyData_PR_eimCertificate:
				res->eim_cfg_data_list[i]->eim_public_key_data.eim_certificate =
				    ipa_buf_alloc(IPA_ES10X_ASN_ENCODER_BUF_SIZE);
				rc = der_encode(&asn_DEF_Certificate,
						&res->res->eimConfigurationDataList.list.array[i]->
						eimPublicKeyData->choice.eimCertificate, ipa_asn1c_consume_bytes_cb,
						res->eim_cfg_data_list[i]->eim_public_key_data.eim_certificate);
				if (rc.encoded <= 0) {
					IPA_LOGP_ES10X("GetEimConfigurationData", LERROR,
						       "data format conversion failed, cannot re-encode eimCertificate in eimPublicKeyData\n");
					IPA_FREE(res->eim_cfg_data_list[i]->eim_public_key_data.eim_certificate);
					res->eim_cfg_data_list[i]->eim_public_key_data.eim_certificate = NULL;
					return;
				}
				break;
			default:
				break;
			}
		}

		if (res->res->eimConfigurationDataList.list.array[i]->trustedPublicKeyDataTls) {
			switch (res->res->eimConfigurationDataList.list.array[i]->trustedPublicKeyDataTls->present) {
			case EimConfigurationData__trustedPublicKeyDataTls_PR_trustedEimPkTls:
				res->eim_cfg_data_list[i]->trusted_public_key_data_tls.trusted_eim_pk_tls =
				    ipa_buf_alloc(IPA_ES10X_ASN_ENCODER_BUF_SIZE);
				rc = der_encode(&asn_DEF_SubjectPublicKeyInfo,
						&res->res->eimConfigurationDataList.list.
						array[i]->trustedPublicKeyDataTls->choice.trustedEimPkTls,
						ipa_asn1c_consume_bytes_cb,
						res->eim_cfg_data_list[i]->
						trusted_public_key_data_tls.trusted_eim_pk_tls);
				if (rc.encoded <= 0) {
					IPA_LOGP_ES10X("GetEimConfigurationData", LERROR,
						       "data format conversion failed, cannot re-encode trustedEimPkTls in trustedPublicKeyDataTls\n");
					IPA_FREE(res->eim_cfg_data_list[i]->
						 trusted_public_key_data_tls.trusted_eim_pk_tls);
					res->eim_cfg_data_list[i]->trusted_public_key_data_tls.trusted_eim_pk_tls =
					    NULL;
					return;
				}
				break;
			case EimConfigurationData__trustedPublicKeyDataTls_PR_trustedCertificateTls:
				res->eim_cfg_data_list[i]->trusted_public_key_data_tls.trusted_certificate_tls =
				    ipa_buf_alloc(IPA_ES10X_ASN_ENCODER_BUF_SIZE);
				rc = der_encode(&asn_DEF_Certificate,
						&res->res->eimConfigurationDataList.list.
						array[i]->trustedPublicKeyDataTls->choice.trustedCertificateTls,
						ipa_asn1c_consume_bytes_cb,
						res->eim_cfg_data_list[i]->
						trusted_public_key_data_tls.trusted_certificate_tls);
				if (rc.encoded <= 0) {
					IPA_LOGP_ES10X("GetEimConfigurationData", LERROR,
						       "data format conversion failed, cannot re-encode trustedCertificateTls in trustedPublicKeyDataTls\n");
					IPA_FREE(res->eim_cfg_data_list[i]->
						 trusted_public_key_data_tls.trusted_certificate_tls);
					res->eim_cfg_data_list[i]->trusted_public_key_data_tls.trusted_certificate_tls =
					    NULL;
					return;
				}
				break;
			default:
				break;
			}
		}

		if (res->res->eimConfigurationDataList.list.array[i]->eimSupportedProtocol)
			res->eim_cfg_data_list[i]->eim_supported_protocol =
			    IPA_BUF_FROM_ASN(res->res->eimConfigurationDataList.list.array[i]->eimSupportedProtocol);
		if (res->res->eimConfigurationDataList.list.array[i]->euiccCiPKId)
			res->eim_cfg_data_list[i]->euicc_ci_pkid =
			    IPA_BUF_FROM_ASN(res->res->eimConfigurationDataList.list.array[i]->euiccCiPKId);
	}
}

static int dec_get_eim_cfg_data(struct ipa_es10b_eim_cfg_data *eim_cfg_data, const struct ipa_buf *es10a_res)
{
	struct GetEimConfigurationDataResponse *asn = NULL;

	asn = ipa_es10x_res_dec(&asn_DEF_GetEimConfigurationDataResponse, es10a_res, "GetEimConfigurationData");
	if (!asn)
		return -EINVAL;

	eim_cfg_data->res = asn;
	return 0;
}

/*! Function (ES10b): GetEimConfigurationData.
 *  \param[inout] ctx pointer to ipa_context.
 *  \returns pointer newly allocated struct with function result, NULL on error. */
struct ipa_es10b_eim_cfg_data *ipa_es10b_get_eim_cfg_data(struct ipa_context *ctx)
{
	struct ipa_buf *es10a_req = NULL;
	struct ipa_buf *es10a_res = NULL;
	struct ipa_es10b_eim_cfg_data *eim_cfg_data = IPA_ALLOC_ZERO(struct ipa_es10b_eim_cfg_data);
	struct GetEimConfigurationDataRequest get_eim_cfg_data_req = { 0 };
	int rc;

	es10a_req =
	    ipa_es10x_req_enc(&asn_DEF_GetEimConfigurationDataRequest, &get_eim_cfg_data_req,
			      "GetEimConfigurationData");
	if (!es10a_req) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10a_res = ipa_euicc_transceive_es10x(ctx, es10a_req);
	if (!es10a_res) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_eim_cfg_data(eim_cfg_data, es10a_res);
	if (rc < 0)
		goto error;

	convert_get_eim_cfg_data(eim_cfg_data);

	IPA_FREE(es10a_req);
	IPA_FREE(es10a_res);
	return eim_cfg_data;
error:
	IPA_FREE(es10a_req);
	IPA_FREE(es10a_res);
	ipa_es10b_get_eim_cfg_data_free(eim_cfg_data);
	return NULL;
}

/*! Free results of function (ES10b): GetEimConfigurationData.
 *  \param[in] res pointer to function result. */
void ipa_es10b_get_eim_cfg_data_free(struct ipa_es10b_eim_cfg_data *res)
{
	if (!res)
		return;

	ipa_es10b_get_eim_cfg_data_free_list(res->eim_cfg_data_list, res->eim_cfg_data_list_count);
	IPA_ES10X_RES_FREE(asn_DEF_GetEimConfigurationDataResponse, res);
}

/*! Filter one EimConfigurationData list item from GetEimConfigurationDataResponse.
 *  \param[in] res pointer to function result that contains the GetEimConfigurationDataResponse.
 *  \param[in] eim_id eimId to look for. When set to NULL, the first item of the EimConfigurationData list is returned.
 *  \returns pointer to EimConfigurationData item, NULL on error. */
struct EimConfigurationData *ipa_es10b_get_eim_cfg_data_filter(struct ipa_es10b_eim_cfg_data *res, char *eim_id)
{
	long i;

	if (!res || !res->res) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR,
			       "cannot filter non existent EimConfigurationData list\n");
		return NULL;
	}

	if (res->res->eimConfigurationDataList.list.count < 1) {
		IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "cannot filter empty EimConfigurationData list\n");
		return NULL;
	}

	/* In case no eim_id is specified, just pick the first item from the list */
	if (!eim_id || eim_id[0] == '\0') {
		return res->res->eimConfigurationDataList.list.array[0];
	}

	for (i = 0; i < res->res->eimConfigurationDataList.list.count; i++) {
		if (IPA_ASN_STR_CMP_BUF
		    (&res->res->eimConfigurationDataList.list.array[i]->eimId, eim_id, strlen(eim_id))) {
			return res->res->eimConfigurationDataList.list.array[i];
		}
	}

	IPA_LOGP_ES10X("GetEimConfigurationData", LERROR, "cannot find eimId %s in EimConfigurationData list\n",
		       eim_id);
	return NULL;
}

/*! Log contents of an struct ipa_eim_cfg_data item.
 *  \param[in] eim_cfg_data pointer to struct ipa_eim_cfg_data item.
 *  \param[in] indent indentation level of the generated output.
 *  \param[in] log_subsys log subsystem to generate the output for.
 *  \param[in] log_level log level to generate the output for. */
void ipa_es10b_get_eim_cfg_data_dump_item(struct ipa_eim_cfg_data *eim_cfg_data, uint8_t indent,
					  enum log_subsys log_subsys, enum log_level log_level)
{
	char indent_str[256];

	memset(indent_str, ' ', indent);
	indent_str[indent] = '\0';

	IPA_LOGP(log_subsys, log_level, "%seIM configuration: \n", indent_str);

	if (!eim_cfg_data) {
		IPA_LOGP(log_subsys, log_level, "%s (none)\n", indent_str);
		return;
	}

	IPA_LOGP(log_subsys, log_level, "%s eimId: \"%s\"\n", indent_str, eim_cfg_data->eim_id);
	if (eim_cfg_data->eim_fqdn)
		IPA_LOGP(log_subsys, log_level, "%s eimFqdn: \"%s\"\n", indent_str, eim_cfg_data->eim_fqdn);
	else
		IPA_LOGP(log_subsys, log_level, "%s eimFqdn: (none)\n", indent_str);

	if (eim_cfg_data->eim_id_type)
		IPA_LOGP(log_subsys, log_level, "%s eimIdType: \"%ld\"\n", indent_str, *eim_cfg_data->eim_id_type);
	else
		IPA_LOGP(log_subsys, log_level, "%s eimIdType: (none)\n", indent_str);

	if (eim_cfg_data->counter_value)
		IPA_LOGP(log_subsys, log_level, "%s counterValue: \"%ld\"\n", indent_str, *eim_cfg_data->counter_value);
	else
		IPA_LOGP(log_subsys, log_level, "%s counterValue: (none)\n", indent_str);

	if (eim_cfg_data->association_token)
		IPA_LOGP(log_subsys, log_level, "%s associationToken: \"%ld\"\n", indent_str,
			 *eim_cfg_data->association_token);
	else
		IPA_LOGP(log_subsys, log_level, "%s associationToken: (none)\n", indent_str);

	IPA_LOGP(log_subsys, log_level, "%s eimPublicKeyData->eimPublicKey: %s\n", indent_str,
		 ipa_buf_hexdump(eim_cfg_data->eim_public_key_data.eim_public_key));
	IPA_LOGP(log_subsys, log_level, "%s eimPublicKeyData->eimCertificate:\n", indent_str);
	ipa_buf_hexdump_multiline(eim_cfg_data->eim_public_key_data.eim_certificate, 32, indent + 2, log_subsys,
				  log_level);

	IPA_LOGP(log_subsys, log_level, "%s trustedPublicKeyDataTls->trustedEimPkTls: %s\n", indent_str,
		 ipa_buf_hexdump(eim_cfg_data->trusted_public_key_data_tls.trusted_eim_pk_tls));
	IPA_LOGP(log_subsys, log_level, "%s trustedPublicKeyDataTls->trustedCertificateTls:\n", indent_str);
	ipa_buf_hexdump_multiline(eim_cfg_data->trusted_public_key_data_tls.trusted_certificate_tls, 32, indent + 2,
				  log_subsys, log_level);

	IPA_LOGP(log_subsys, log_level, "%s eimSupportedProtocol: %s\n", indent_str,
		 ipa_buf_hexdump(eim_cfg_data->eim_supported_protocol));
	IPA_LOGP(log_subsys, log_level, "%s euiccCiPKId: %s\n", indent_str,
		 ipa_buf_hexdump(eim_cfg_data->euicc_ci_pkid));
}

/*! Log contents of eim_cfg_data_list in struct ipa_es10b_eim_cfg_data.
 *  \param[in] eim_cfg_data_list pointer to pointer array that holds the list items.
 *  \param[in] eim_cfg_data_list_count number of list items.
 *  \param[in] indent indentation level of the generated output.
 *  \param[in] log_subsys log subsystem to generate the output for.
 *  \param[in] log_level log level to generate the output for. */
void ipa_es10b_get_eim_cfg_data_dump_list(struct ipa_eim_cfg_data **eim_cfg_data_list, long eim_cfg_data_list_count,
					  uint8_t indent, enum log_subsys log_subsys, enum log_level log_level)
{
	unsigned int i;
	for (i = 0; i < eim_cfg_data_list_count; i++) {
		ipa_es10b_get_eim_cfg_data_dump_item(eim_cfg_data_list[i], indent, log_subsys, log_level);
	}
}

/*! Free list of struct ipa_eim_cfg_data.
 *  \param[in] eim_cfg_data_list pointer to pointer array that holds the list items.
 *  \param[in] eim_cfg_data_list_count number of list items. */
void ipa_es10b_get_eim_cfg_data_free_list(struct ipa_eim_cfg_data **eim_cfg_data_list, long eim_cfg_data_list_count)
{
	long i;

	if (!eim_cfg_data_list)
		return;
	if (!eim_cfg_data_list_count)
		return;

	for (i = 0; i < eim_cfg_data_list_count; i++) {
		IPA_FREE(eim_cfg_data_list[i]->eim_id);
		IPA_FREE(eim_cfg_data_list[i]->eim_fqdn);

		IPA_FREE(eim_cfg_data_list[i]->eim_id_type);
		IPA_FREE(eim_cfg_data_list[i]->counter_value);
		IPA_FREE(eim_cfg_data_list[i]->association_token);

		IPA_FREE(eim_cfg_data_list[i]->eim_public_key_data.eim_public_key);
		IPA_FREE(eim_cfg_data_list[i]->eim_public_key_data.eim_certificate);

		IPA_FREE(eim_cfg_data_list[i]->trusted_public_key_data_tls.trusted_eim_pk_tls);
		IPA_FREE(eim_cfg_data_list[i]->trusted_public_key_data_tls.trusted_certificate_tls);

		IPA_FREE(eim_cfg_data_list[i]->eim_supported_protocol);
		IPA_FREE(eim_cfg_data_list[i]->euicc_ci_pkid);

		IPA_FREE(eim_cfg_data_list[i]);
	}
	IPA_FREE(eim_cfg_data_list);
}
