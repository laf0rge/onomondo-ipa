/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.22, 5.7.7: Function (ES10b): GetEID
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
#include "length.h"
#include "utils.h"
#include "euicc.h"
#include "es10x.h"
#include "es10b_get_euicc_chlg.h"
#include "GetEuiccDataRequest.h"
#include "GetEuiccDataResponse.h"
#include "Octet16.h"

static int dec_get_euicc_data_res(uint8_t *eid, struct ipa_buf *es10b_res)
{
	struct GetEuiccDataResponse *asn = NULL;
	uint8_t eid_buf[IPA_LEN_EID];

	asn = ipa_es10x_res_dec(&asn_DEF_GetEuiccDataResponse, es10b_res, "GetEID");
	if (!asn)
		return -EINVAL;

	IPA_COPY_ASN_BUF(eid_buf, &asn->eidValue);
	memcpy(eid, eid_buf, sizeof(eid_buf));
	ASN_STRUCT_FREE(asn_DEF_GetEuiccDataResponse, asn);

	return 0;
}

int ipa_es10b_get_eid(struct ipa_context *ctx, uint8_t *eid)
{
	struct ipa_buf *es10b_req = NULL;
	struct ipa_buf *es10b_res = NULL;
	struct GetEuiccDataRequest get_euicc_data_req = { 0 };
	int rc = -EINVAL;

	get_euicc_data_req.tagList.buf = (uint8_t *) "\x5A";
	get_euicc_data_req.tagList.size = 1;
	es10b_req = ipa_es10x_req_enc(&asn_DEF_GetEuiccDataRequest, &get_euicc_data_req, "GetEID");
	if (!es10b_req) {
		IPA_LOGP_ES10X("GetEID", LERROR, "unable to encode ES10b request\n");
		goto error;
	}

	es10b_res = ipa_euicc_transceive_es10x(ctx, es10b_req);
	if (!es10b_res) {
		IPA_LOGP_ES10X("GetEID", LERROR, "no ES10b response\n");
		goto error;
	}

	rc = dec_get_euicc_data_res(eid, es10b_res);
	if (rc < 0)
		goto error;

error:
	IPA_FREE(es10b_req);
	IPA_FREE(es10b_res);
	return rc;
}
