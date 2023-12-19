AM_CPPFLAGS = \
	$(all_includes) \
	-I$(top_srcdir)/include \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/ipa \
	-I$(top_srcdir)/src/ipa/libasn \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	$(NULL)

AM_LDFLAGS = \
	$(NULL)

noinst_LIBRARIES = libipa.a

libipa_a_SOURCES = \
	activation_code.c \
	es10a_get_euicc_cfg_addr.c \
	es10b_auth_serv.c \
	es10b_cancel_session.c \
	es10b_get_certs.c \
	es10b_get_eim_cfg_data.c \
	es10b_get_euicc_chlg.c \
	es10b_get_euicc_info.c \
	es10b_load_bnd_prfle_pkg.c \
	es10b_load_euicc_pkg.c \
	es10b_prep_dwnld.c \
	es10b_retr_notif_from_lst.c \
	es10b_rm_notif_from_lst.c \
	es10c_get_eid.c \
	es10x.c \
	esipa_auth_clnt.c \
	esipa.c \
	esipa_cancel_session.c \
	esipa_get_bnd_prfle_pkg.c \
	esipa_get_eim_pkg.c \
	esipa_handle_notif.c \
	esipa_init_auth.c \
	esipa_prvde_eim_pkg_rslt.c \
	euicc.c \
	ipad.c \
	log.c \
	proc_cmn_cancel_sess.c \
	proc_cmn_mtl_auth.c \
	proc_direct_prfle_dwnld.c \
	proc_euicc_data_req.c \
	proc_euicc_pkg_dwnld_exec.c \
	proc_prfle_dwnld.c \
	proc_prfle_inst.c \
	utils.c \
	$(NULL)

noinst_HEADERS = \
	activation_code.h \
	context.h \
	es10a_get_euicc_cfg_addr.h \
	es10b_auth_serv.h \
	es10b_cancel_session.h \
	es10b_get_certs.h \
	es10b_get_eim_cfg_data.h \
	es10b_get_euicc_chlg.h \
	es10b_get_euicc_info.h \
	es10b_load_bnd_prfle_pkg.h \
	es10b_load_euicc_pkg.h \
	es10b_prep_dwnld.h \
	es10b_retr_notif_from_lst.h \
	es10b_rm_notif_from_lst.h \
	es10c_get_eid.h \
	es10x.h \
	esipa_auth_clnt.h \
	esipa_cancel_session.h \
	esipa_get_bnd_prfle_pkg.h \
	esipa_get_eim_pkg.h \
	esipa.h \
	esipa_handle_notif.h \
	esipa_init_auth.h \
	esipa_prvde_eim_pkg_rslt.h \
	euicc.h \
	length.h \
	proc_cmn_cancel_sess.h \
	proc_cmn_mtl_auth.h \
	proc_direct_prfle_dwnld.h \
	proc_euicc_data_req.h \
	proc_euicc_pkg_dwnld_exec.h \
	proc_prfle_dwnld.h \
	proc_prfle_inst.h \
	utils.h \
	$(NULL)
