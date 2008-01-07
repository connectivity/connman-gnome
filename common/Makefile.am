
noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = client.h client.c

noinst_PROGRAMS = test-client

test_client_LDADD = libcommon.a @GTK_LIBS@ @DBUS_LIBS@

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in
