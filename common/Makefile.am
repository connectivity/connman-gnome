
noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = connman-dbus.c connman-dbus.h \
				connman-client.h connman-client.c \
							instance.h instance.c

noinst_PROGRAMS = connman-demo test-client

connman_demo_SOURCES = demo.c
connman_demo_LDADD = libcommon.a @GTK_LIBS@ @DBUS_LIBS@

test_client_LDADD = libcommon.a @GTK_LIBS@ @DBUS_LIBS@

BUILT_SOURCES = marshal.h marshal.c \
			connman-dbus-glue.h \
				instance-glue.h

nodist_libcommon_a_SOURCES = $(BUILT_SOURCES)

CLEANFILES = $(BUILT_SOURCES)

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@

EXTRA_DIST = marshal.list instance.xml connman-client.xml

MAINTAINERCLEANFILES = Makefile.in

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal $< --header > $@

marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal $< --body > $@

instance-glue.h: instance.xml
	$(DBUS_BINDING_TOOL) --prefix=instance --mode=glib-server --output=$@ $<

connman-dbus-glue.h: connman-dbus.xml
	$(DBUS_BINDING_TOOL) --prefix=connman --mode=glib-client --output=$@ $<
