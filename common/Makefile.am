
noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = common.h client.h client.c \
				instance.h instance.c

noinst_PROGRAMS = test-client

test_client_LDADD = libcommon.a @GTK_LIBS@ @DBUS_LIBS@

BUILT_SOURCES = marshal.h marshal.c instance-glue.h client-glue.h

nodist_libcommon_a_SOURCES = $(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@

EXTRA_DIST = marshal.list instance.xml client.xml

MAINTAINERCLEANFILES = Makefile.in

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal $< --header > $@

marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal $< --body > $@

instance-glue.h: instance.xml
	$(DBUS_BINDING_TOOL) --prefix=instance --mode=glib-server --output=$@ $<

client-glue.h: client.xml
	$(DBUS_BINDING_TOOL) --prefix=client --mode=glib-client --output=$@ $<
