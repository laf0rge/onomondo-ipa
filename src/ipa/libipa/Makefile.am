AM_CPPFLAGS = \
	$(all_includes) \
	-I$(top_srcdir)/include \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/ipa \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	-Werror \
	$(NULL)

AM_LDFLAGS = \
	$(NULL)

noinst_LIBRARIES = libipa.a

libipa_a_SOURCES = \
	utils.c \
	log.c \
	$(NULL)

noinst_HEADERS = \
	utils.h \
	$(NULL)
