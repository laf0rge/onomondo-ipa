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

#define STORE_DATA_CLA 0x80
#define STORE_DATA_INS 0xE2
#define STORE_DATA_P1_LAST_BLOCK 0x11
#define STORE_DATA_P1_MORE_BLOCKS 0x91

#define GET_RESPONSE_CLA 0x00
#define GET_RESPONSE_INS 0xC0

#define MAX_BLOCKSIZE_TX 255
#define MAX_BLOCKSIZE_RX 255

struct req_apdu {
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t lc;
	uint8_t le;
	uint8_t data[255];
};

struct res_apdu {
	uint8_t le;
	uint8_t data[255];
	uint16_t sw;
};

struct ipa_buf *format_req_apdu(struct req_apdu *req_apdu)
{
	struct ipa_buf *buf_req = ipa_buf_alloc(5 + req_apdu->lc);

	buf_req->data[0] = req_apdu->cla;
	buf_req->data[1] = req_apdu->ins;
	buf_req->data[2] = req_apdu->p1;
	buf_req->data[3] = req_apdu->p2;

	if (req_apdu->lc > 0 && req_apdu->le == 0) {
		/* Send data (no response data expected) */
		buf_req->data[4] = req_apdu->lc;
		memcpy(buf_req->data + 5, req_apdu->data, req_apdu->lc);
		buf_req->len = 5 + req_apdu->lc;
	} else if (req_apdu->lc == 0 && req_apdu->le > 0) {
		/* Receive data (no data to send) */
		buf_req->data[4] = req_apdu->le;
		buf_req->len = 5;
	} else if (req_apdu->lc == 0 && req_apdu->le == 0) {
		/* No data to send and no receove data expected */
		buf_req->data[4] = 0;
		buf_req->len = 5;
	} else {
		/* The T=0 protocol does not support receiving and sending data
		 * at the same time. The caller must ensure that the APDU
		 * struct is filled in with reasonable values! */
		assert(NULL);
	}

	return buf_req;
}

int parse_res_apdu(struct res_apdu *res_apdu, struct ipa_buf *res_encoded)
{
	memset(res_apdu, 0, sizeof(*res_apdu));

	/* The encoded response should at least contain 2 byte status word */
	if (res_encoded->len < 2)
		return -EINVAL;

	res_apdu->le = res_encoded->len - 2;
	if (res_apdu->le)
		memcpy(res_apdu->data, res_encoded->data, res_apdu->le);

	res_apdu->sw = res_encoded->data[res_apdu->le] << 8;
	res_apdu->sw |= res_encoded->data[res_apdu->le + 1];

	return 0;
}

static int send_es10x_block(struct ipa_context *ctx, uint16_t *sw,
			    const struct ipa_buf *es10x_req, size_t offset,
			    uint8_t block_nr)
{
	size_t len_req;
	int rc;
	struct req_apdu req_apdu;
	struct res_apdu res_apdu;
	struct ipa_buf *buf_req;
	struct ipa_buf *buf_res;

	buf_res = ipa_buf_alloc(MAX_BLOCKSIZE_TX + 2);
	assert(buf_res);

	len_req = es10x_req->len - offset;

	/* fill in request APDU for STORE DATA
	 * (see also GSMA SGP.22, section 5.7.2) */
	req_apdu.cla = STORE_DATA_CLA;
	req_apdu.ins = STORE_DATA_INS;
	if (len_req > MAX_BLOCKSIZE_TX)
		req_apdu.p1 = STORE_DATA_P1_MORE_BLOCKS;
	else
		req_apdu.p1 = STORE_DATA_P1_LAST_BLOCK;
	req_apdu.p2 = block_nr;
	if (len_req > MAX_BLOCKSIZE_TX)
		req_apdu.lc = MAX_BLOCKSIZE_TX;
	else
		req_apdu.lc = (uint8_t) len_req;
	memcpy(req_apdu.data, es10x_req->data + offset, req_apdu.lc);

	/* transceive block */
	buf_req = format_req_apdu(&req_apdu);
	rc = ipa_scard_transceive(ctx->scard_ctx, buf_res, buf_req);
	if (rc < 0) {
		IPA_LOGP(SIPA, LERROR,
			 "unable to send ES10b block %u, offset=%zu\n",
			 block_nr, offset);
		goto exit;
	}

	/* parse response */
	rc = parse_res_apdu(&res_apdu, buf_res);
	if (rc < 0) {
		IPA_LOGP(SIPA, LERROR,
			 "invalid response while sending ES10b block %u, offset=%zu\n",
			 block_nr, offset);
		goto exit;
	}
	*sw = res_apdu.sw;

	IPA_LOGP(SIPA, LINFO,
		 "successfully sent ES10b block %u, offset=%zu, sw=%04x\n",
		 block_nr, offset, *sw);

	/* Return how many data we have sent. */
	rc = req_apdu.lc;
exit:
	IPA_FREE(buf_req);
	IPA_FREE(buf_res);
	return rc;
}

static int recv_es10x_block(struct ipa_context *ctx, uint16_t *sw,
			    struct ipa_buf *es10x_res, uint8_t block_len,
			    uint8_t block_nr)
{
	int rc;
	struct req_apdu req_apdu;
	struct res_apdu res_apdu;
	struct ipa_buf *buf_req;
	struct ipa_buf *buf_res;

	buf_res = ipa_buf_alloc(MAX_BLOCKSIZE_RX + 2);
	assert(buf_res);

	/* In case the expected block length exceeds our buffer limit, we must
	 * clip. This is no problem since it is always up to the caller to
	 * check by the return code how many bytes were actually transmited.
	 * The caller also must evaluate the status word to know if there are
	 * still bytes available in the GET RESPONSE buffer of the eUICC. */
	if (block_len > MAX_BLOCKSIZE_RX)
		block_len = MAX_BLOCKSIZE_RX;

	/* fill in request APDU for GET RESPONSE
	 * (see also ISO/IEC 7816-4, 7.6.1) */
	req_apdu.cla = GET_RESPONSE_CLA;
	req_apdu.ins = GET_RESPONSE_INS;
	req_apdu.p1 = 0x00;
	req_apdu.p2 = 0x00;
	req_apdu.lc = 0;
	req_apdu.le = block_len;

	/* receive block */
	buf_req = format_req_apdu(&req_apdu);
	rc = ipa_scard_transceive(ctx->scard_ctx, buf_res, buf_req);
	if (rc < 0) {
		IPA_LOGP(SIPA, LERROR,
			 "unable to receive ES10b block %u, offset=%zu\n",
			 block_nr, es10x_res->len);
		rc = -EIO;
		goto exit;
	}

	/* parse response */
	rc = parse_res_apdu(&res_apdu, buf_res);
	if (rc < 0) {
		IPA_LOGP(SIPA, LERROR,
			 "invalid response while sending ES10b block %u, offset=%zu\n",
			 block_nr, es10x_res->len);
		rc = -EINVAL;
		goto exit;
	}
	if (res_apdu.le != block_len) {
		IPA_LOGP(SIPA, LERROR,
			 "unexpected block length (expected:%u, got:%u) while sending ES10b block %u, offset=%zu\n",
			 block_len, res_apdu.le, block_nr, es10x_res->len);
		rc = -EINVAL;
		goto exit;
	}
	if (es10x_res->len + res_apdu.le > es10x_res->data_len) {
		IPA_LOGP(SIPA, LERROR,
			 "out of memory (have:%zu, needed:%zu) while sending ES10b block %u, offset=%zu\n",
			 es10x_res->data_len, es10x_res->len + res_apdu.le,
			 block_nr, es10x_res->len);
		rc = -EINVAL;
		goto exit;
	}

	memcpy(es10x_res->data + es10x_res->len, res_apdu.data, res_apdu.le);
	es10x_res->len += res_apdu.le;
	*sw = res_apdu.sw;

	IPA_LOGP(SIPA, LINFO,
		 "successfully received ES10b block %u, offset=%zu, sw=%04x\n",
		 block_nr, es10x_res->len, *sw);

	/* Return how many data we have received. */
	rc = res_apdu.le;
exit:
	IPA_FREE(buf_req);
	IPA_FREE(buf_res);
	return rc;
}

/*! Transceive eUICC/es10x APDU.
 *  \param[inout] scard_ctx smartcard reader context.
 *  \param[out] es10x_res buffer to store eUICC/es10x response.
 *  \param[out] es10x_res buffer with eUICC/es10x request.
 *  \returns 0 on success, -EIO on failure. */
int ipa_euicc_transceive_es10x(struct ipa_context *ctx,
			       struct ipa_buf *es10x_res,
			       const struct ipa_buf *es10x_req)
{

	uint16_t sw;
	uint8_t block_nr = 0;
	size_t offset = 0;
	int rc;

	while (1) {
		rc = send_es10x_block(ctx, &sw, es10x_req, offset, block_nr);
		if (rc < 0)
			return -EIO;
		offset += rc;
		block_nr++;

		/* Check if we are done */
		if (offset >= es10x_req->len)
			break;

		/* The eUICC should ACK each block with SW=9000, the last block
		 * be confirmied with 61xx to indicate that response data is
		 * available */
		if (sw != 0x9000 && offset < es10x_req->len) {
			IPA_LOGP(SIPA, LERROR,
				 "ES10b transmission aborted early by eUICC, sw=%04x\n",
				 sw);
			break;
		}

		/* We can only transmit a maximum amount of 255 blocks in one
		 * STORE DATA cycle. */
		if (block_nr == 255) {
			IPA_LOGP(SIPA, LERROR,
				 "ES10b request exceeds maximum transmission length (%zu)!\n",
				 es10x_req->len);
			return -EINVAL;
		}
	}

	/* When the transfer of the ES10b request is done, we expect the eUICC
	 * to answer with a response. */
	if (sw == 0x9000) {
		IPA_LOGP(SIPA, LERROR,
			 "ES10b transmission successful, but no response, sw=%04x\n",
			 sw);
		return -EINVAL;
	} else if ((sw & 0xff00) == 0x6100) {
		block_nr = 0;

		while (1) {
			rc = recv_es10x_block(ctx, &sw, es10x_res, sw & 0xff,
					      block_nr);
			if (rc < 0)
				return -EIO;
			block_nr++;

			if (sw == 0x9000) {
				IPA_LOGP(SIPA, LINFO,
					 "ES10b transmission successful, sw=%04x\n",
					 sw);
				return 0;
			}

			if ((sw & 0xff00) != 0x6100) {
				IPA_LOGP(SIPA, LINFO,
					 "ES10b transmission failed, sw=%04x\n",
					 sw);
				return -EINVAL;
			}
		}
	} else {
		IPA_LOGP(SIPA, LERROR, "ES10b transmission failed! sw=%04x\n",
			 sw);
		return -EINVAL;
	}

	return 0;
}
