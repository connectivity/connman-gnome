
noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = common.h client.h client.c \
				instance.h instance.c

noinst_PROGRAMS = test-client

test_client_LDADD = libcommon.a @GTK_LIBS@ @DBUS_LIBS@

BUILT_SOURCES = instance-glue.h

nodist_libcommon_a_SOURCES = $(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@

EXTRA_DIST = instance.xml

MAINTAINERCLEANFILES = Makefile.in

instance-glue.h: instance.xml
	$(DBUS_BINDING_TOOL) --prefix=instance --mode=glib-server --output=$@ $<
