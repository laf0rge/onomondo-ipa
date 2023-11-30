/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 * See also: GSMA SGP.32, section 3.2.3.1: Direct Profile Download
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
#include "activation_code.h"
#include <AuthenticateClientRequestEsipa.h>
#include <AuthenticateClientResponseEsipa.h>
#include <PrepareDownloadResponse.h>
#include <CancelSessionReason.h>
#include <BoundProfilePackage.h>
#include <GetBoundProfilePackageOkEsipa.h>
#include "esipa_auth_clnt.h"
#include "proc_cmn_mtl_auth.h"
#include "proc_prfle_dwnld.h"
#include "proc_direct_prfle_dwnld.h"
#include "esipa_get_bnd_prfle_pkg.h"
#include "proc_cmn_cancel_sess.h"
#include "proc_prfle_inst.h"

int ipa_proc_direct_prfle_dwnlod(struct ipa_context *ctx, struct ipa_proc_direct_prfle_dwnlod_pars *pars)
{
	struct ipa_activation_code *activation_code = NULL;
	struct ipa_esipa_auth_clnt_res *auth_clnt_res = NULL;
	struct ipa_esipa_get_bnd_prfle_pkg_res *get_bnd_prfle_pkg_res = NULL;
	struct ipa_proc_cmn_mtl_auth_pars cmn_mtl_auth_pars = { 0 };
	struct ipa_proc_cmn_cancel_sess_pars cmn_cancel_sess_pars = { 0 };
	struct ipa_proc_prfle_dwnlod_pars prfle_dwnlod_pars = { 0 };
	struct ipa_proc_prfle_inst_pars prfle_inst_pars = { 0 };

	/* This procedure is called when the IPAd receives an eIM package with a download trigger request
	 * (which contains the activation code) */

	activation_code = ipa_activation_code_parse(pars->ac);
	ipa_activation_code_dump(activation_code, 0, SIPA, LDEBUG);
	if (!activation_code) {
		IPA_LOGP(SIPA, LERROR, "cannot continue, activation code invalid or missing!\n");
		goto error;
	}

	/* Execute sub procedure: Common Mutual Authentication Procedure */
	cmn_mtl_auth_pars.tac = pars->tac;
	cmn_mtl_auth_pars.allowed_ca = pars->allowed_ca;
	cmn_mtl_auth_pars.smdp_addr = activation_code->sm_dp_plus_address;
	auth_clnt_res = ipa_proc_cmn_mtl_auth(ctx, &cmn_mtl_auth_pars);
	if (!auth_clnt_res) {
		IPA_LOGP(SIPA, LERROR, "cannot continue, mutual authentication failed -- canceling session!\n");
		goto error;
	}

	/* TODO: Check if ProfileMetadata contains PPR(s) */

	/* Execute sub procedure: Sub-procedure Profile Download and Installation – Download Confirmation */
	prfle_dwnlod_pars.auth_clnt_ok_dpe = auth_clnt_res->auth_clnt_ok_dpe;
	get_bnd_prfle_pkg_res = ipa_proc_prfle_dwnlod(ctx, &prfle_dwnlod_pars);
	if (!get_bnd_prfle_pkg_res) {
		IPA_LOGP(SIPA, LERROR, "sub profile download has failed -- canceling session!\n");
		cmn_cancel_sess_pars.reason = CancelSessionReason_loadBppExecutionError;
		cmn_cancel_sess_pars.transaction_id = *auth_clnt_res->transaction_id;
		ipa_proc_cmn_cancel_sess(ctx, &cmn_cancel_sess_pars);
		goto error;
	}

	/* TODO: At this point we should ask the user for consent before we proceed with the profile installation.
	 * In case the user does not concent, we must abort by calling the common cancel session procedure.
	 * (we are on an IoT device, maybe have this consent pre-set in a config file? we could als have a mechanism
	 * where a callback is registered that is used to ask for concent?) */

	/* Execute sub procedure: Sub-procedure Profile Installation (See also section 3.1.3.3 of SGP.22) */
	prfle_inst_pars.bound_profile_package = &get_bnd_prfle_pkg_res->get_bnd_prfle_pkg_ok->boundProfilePackage;
	ipa_proc_prfle_inst(ctx, &prfle_inst_pars);

	/* TODO: Send back a ProfileDownloadTriggerResult to the eIM */

	/* TODO: Take care that the profile is enabled, in case automatic profile activation is granted */

error:
	ipa_activation_code_free(activation_code);
	ipa_esipa_auth_clnt_res_free(auth_clnt_res);
	ipa_esipa_get_bnd_prfle_pkg_res_free(get_bnd_prfle_pkg_res);
	return 0;
}
