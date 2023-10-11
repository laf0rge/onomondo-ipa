include Makefile.am.libasncodec

AM_CPPFLAGS = \
	$(all_includes) \
	-I$(top_srcdir)/include \
	-I$(top_builddir) \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	$(NULL)

AM_LDFLAGS = \
	-lm \
	$(NULL)

noinst_LIBRARIES = libasn.a

libasn_a_SOURCES = $(ASN_MODULE_SRCS)

noinst_HEADERS = $(ASN_MODULE_HDRS)

regen:
	asn1c -fcompound-names \
	$(top_srcdir)/asn1/PKIX1Explicit88.asn \
	$(top_srcdir)/asn1/PKIX1Implicit88.asn \
	$(top_srcdir)/asn1/RSPDefinitions.asn \
	$(top_srcdir)/asn1/SGP32Definitions.asn
	sed -i '/ASN_MODULE_CFLAGS=/Q' Makefile.am.libasncodec
