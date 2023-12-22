/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.22, section 3.1.3.3: Sub-procedure Profile Installation
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <onomondo/ipa/mem.h>
#include <onomondo/ipa/utils.h>
#include <onomondo/ipa/log.h>
#include "context.h"
#include "utils.h"
#include "es10x.h"
#include "es10b_load_bnd_prfle_pkg.h"
#include "esipa_handle_notif.h"
#include "es10b_rm_notif_from_lst.h"
#include "proc_prfle_inst.h"

/* We receive the Initialize Secure Channel Request, its decoded form, so we must encode it again */
struct ipa_buf *enc_init_sec_chan_req(const struct InitialiseSecureChannelRequest *init_sec_chan_req)
{
	struct ipa_buf *init_sec_chan_req_encoded = ipa_buf_alloc(IPA_ES10X_ASN_ENCODER_BUF_SIZE);
	asn_enc_rval_t rc;

#ifdef IPA_DEBUG_ASN1
	IPA_LOGP(SIPA, LDEBUG, "encoding Initialise Secure Channel Request:\n");
	ipa_asn1c_dump(&asn_DEF_InitialiseSecureChannelRequest, init_sec_chan_req, 1, SIPA, LDEBUG);
#endif

	assert(init_sec_chan_req_encoded);
	rc = der_encode(&asn_DEF_InitialiseSecureChannelRequest, init_sec_chan_req, ipa_asn1c_consume_bytes_cb,
			init_sec_chan_req_encoded);

	if (rc.encoded <= 0) {
		IPA_LOGP(SIPA, LDEBUG, "cannot encode Initialise Secure Channel Request!\n");
		IPA_FREE(init_sec_chan_req_encoded);
		return NULL;
	}

	IPA_LOGP(SIPA, LDEBUG, "encoded Initialise Secure Channel Request: %s\n",
		 ipa_buf_hexdump(init_sec_chan_req_encoded));

	return init_sec_chan_req_encoded;
}

/* Return codes: < 0 = error, 0 = ok, 1 = Result was present, notification sent */
int handle_load_bnd_prfle_pkg_res(struct ipa_context *ctx, struct ipa_es10b_load_bnd_prfle_pkg_res *res,
				  long *seq_number)
{
	struct ipa_esipa_handle_notif_req handle_notif_req = { 0 };
	int rc;

	*seq_number = 0;

	/* No response is present, this is the normal case while a sequence of LoadBoundProfilePackage functions is
	 * executed. */
	if (!res->res) {
		ipa_es10b_load_bnd_prfle_res_free(res);
		return 0;
	}

	/* A respond is present, this is either the normal ending of the installation sequence or the eUICC has aborted
	 * the the installation. In both situations we forward the ProfileInstallationResult to the eIM. */
	handle_notif_req.profile_installation_result = res->res;
	rc = ipa_esipa_handle_notif(ctx, &handle_notif_req);
	*seq_number = res->res->profileInstallationResultData.notificationMetadata.seqNumber;
	ipa_es10b_load_bnd_prfle_res_free(res);
	if (rc < 0)
		return -EINVAL;
	return 1;
}

int ipa_proc_prfle_inst(struct ipa_context *ctx, struct ipa_proc_prfle_inst_pars *pars)
{
	const struct InitialiseSecureChannelRequest *init_sec_chan_req = NULL;
	struct ipa_es10b_load_bnd_prfle_pkg_res *load_bnd_prfle_pkg_res = NULL;
	struct ipa_buf *init_sec_chan_req_encoded = NULL;
	unsigned int i;
	int rc;
	long seq_number = 0;

	/* Step #1-#2 (ES8+.InitialiseSecureChannel) */
	init_sec_chan_req = &pars->bound_profile_package->initialiseSecureChannelRequest;
	init_sec_chan_req_encoded = enc_init_sec_chan_req(init_sec_chan_req);
	load_bnd_prfle_pkg_res =
	    ipa_es10b_load_bnd_prfle_pkg(ctx, init_sec_chan_req_encoded->data, init_sec_chan_req_encoded->len);
	if (!load_bnd_prfle_pkg_res) {
		IPA_LOGP(SIPA, LERROR, "failed to transfer InitialiseSecureChannelRequest!\n");
		IPA_FREE(init_sec_chan_req_encoded);
		goto error;
	}
	IPA_FREE(init_sec_chan_req_encoded);
	rc = handle_load_bnd_prfle_pkg_res(ctx, load_bnd_prfle_pkg_res, &seq_number);
	if (rc)
		goto error;

	/* Step #3 (ES8+.ConfigureISDP) */
	for (i = 0; i < pars->bound_profile_package->firstSequenceOf87.list.count; i++) {
		IPA_LOGP(SIPA, LDEBUG, "transferring ES8+.ConfigureISDP segments...\n");
		load_bnd_prfle_pkg_res =
		    ipa_es10b_load_bnd_prfle_pkg(ctx, pars->bound_profile_package->firstSequenceOf87.list.array[i]->buf,
						 pars->bound_profile_package->firstSequenceOf87.list.array[i]->size);
		if (!load_bnd_prfle_pkg_res) {
			IPA_LOGP(SIPA, LERROR, "failed to transfer ES8+.ConfigureISDP segments!\n");
			goto error;
		}
		rc = handle_load_bnd_prfle_pkg_res(ctx, load_bnd_prfle_pkg_res, &seq_number);
		if (rc)
			goto error;
	}

	/* Step #4 (ES8+.StoreMetadata) */
	for (i = 0; i < pars->bound_profile_package->sequenceOf88.list.count; i++) {
		IPA_LOGP(SIPA, LDEBUG, "transferring ES8+.StoreMetadata segments...\n");
		load_bnd_prfle_pkg_res =
		    ipa_es10b_load_bnd_prfle_pkg(ctx, pars->bound_profile_package->sequenceOf88.list.array[i]->buf,
						 pars->bound_profile_package->sequenceOf88.list.array[i]->size);
		if (!load_bnd_prfle_pkg_res) {
			IPA_LOGP(SIPA, LERROR, "failed to transfer ES8+.StoreMetadata segments!\n");
			goto error;
		}
		rc = handle_load_bnd_prfle_pkg_res(ctx, load_bnd_prfle_pkg_res, &seq_number);
		if (rc)
			goto error;
	}

	/* Step #5 (ES8+.ReplaceSessionKeys", optional) */
	if (pars->bound_profile_package->secondSequenceOf87) {
		for (i = 0; i < pars->bound_profile_package->secondSequenceOf87->list.count; i++) {
			IPA_LOGP(SIPA, LDEBUG, "transferring ES8+.ReplaceSessionKeys segments...\n");
			load_bnd_prfle_pkg_res =
			    ipa_es10b_load_bnd_prfle_pkg(ctx,
							 pars->bound_profile_package->secondSequenceOf87->list.
							 array[i]->buf,
							 pars->bound_profile_package->secondSequenceOf87->list.
							 array[i]->size);
			if (!load_bnd_prfle_pkg_res) {
				IPA_LOGP(SIPA, LERROR, "failed to transfer ES8+.ReplaceSessionKeys segments!\n");
				goto error;
			}
			rc = handle_load_bnd_prfle_pkg_res(ctx, load_bnd_prfle_pkg_res, &seq_number);
			if (rc)
				goto error;
		}
	}

	/* Step #6 (ES8+.LoadProfileElements) */
	for (i = 0; i < pars->bound_profile_package->sequenceOf86.list.count; i++) {
		IPA_LOGP(SIPA, LDEBUG, "transferring ES8+.LoadProfileElements segments...\n");
		load_bnd_prfle_pkg_res =
		    ipa_es10b_load_bnd_prfle_pkg(ctx, pars->bound_profile_package->sequenceOf86.list.array[i]->buf,
						 pars->bound_profile_package->sequenceOf86.list.array[i]->size);
		if (!load_bnd_prfle_pkg_res) {
			IPA_LOGP(SIPA, LERROR, "failed to transfer ES8+.LoadProfileElements segments!\n");
			goto error;
		}
		rc = handle_load_bnd_prfle_pkg_res(ctx, load_bnd_prfle_pkg_res, &seq_number);
		if (rc < 0) {
			goto error;
		} else if (rc == 0 && i == pars->bound_profile_package->sequenceOf86.list.count - 1) {
			IPA_LOGP(SIPA, LERROR, "eUICC didn't respond with ProfileInstallationResult!\n");
			goto error;
		}
	}

	/* Step #11 (ES10b.RemoveNotificationFromList) */
	rc = ipa_es10b_rm_notif_from_lst(ctx, seq_number);
	if (rc < 0)
		goto error;

	IPA_LOGP(SIPA, LINFO, "profile installation succeded!\n");
	return 0;
error:
	IPA_LOGP(SIPA, LERROR, "profile installation failed!\n");
	return -EINVAL;
}
