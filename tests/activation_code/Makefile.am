AM_CPPFLAGS = \
	$(all_includes) \
	-I$(top_srcdir)/include \
	$(NULL)

AM_CFLAGS = \
	-Wall \
	-ggdb3 \
	$(NULL)

EXTRA_DIST = \
	activation_code_test.ok \
	$(NULL)

noinst_PROGRAMS = \
	activation_code_test \
	$(NULL)

activation_code_test_SOURCES = \
	activation_code_test.c \
	$(NULL)

activation_code_test_LDADD = \
	$(top_srcdir)/src/ipa/libipa/libipa.a \
	$(NULL)
