SUBDIRS = libipa \
	  libasn \
	  .

AM_CPPFLAGS = \
	$(all_includes) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/ipa/libasn \
	-I$(top_builddir) \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	$(LIBCURL_CFLAGS) \
	$(PCSC_CFLAGS) \
	$(NULL)

AM_LDFLAGS = \
	$(NULL)

bin_PROGRAMS = \
	ipa \
	$(NULL)

ipa_SOURCES = \
	main.c \
	http.c \
	scard.c \
	$(NULL)

noinst_HEADERS = \
	$(NULL)

ipa_LDADD = \
	libipa/libipa.a \
	libasn/libasn.a \
	$(LIBCURL_LIBS) \
	$(PCSC_LIBS) \
	$(NULL)
