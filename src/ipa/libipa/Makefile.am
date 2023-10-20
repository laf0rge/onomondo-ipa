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
	eim_package.c \
	es10b_get_euicc_challenge.c \
	es10b_get_euicc_info.c \
	euicc.c \
	ipad.c \
	utils.c \
	log.c \
	$(NULL)

noinst_HEADERS = \
	activation_code.h \
	context.h \
	eim_package.h \
	es10b.h \
	es10b_get_euicc_challenge.h \
	es10b_get_euicc_info.h \
	euicc.h \
	length.h \
	utils.h \
	$(NULL)
