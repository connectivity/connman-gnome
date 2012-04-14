
noinst_LIBRARIES = libcommon.a

libcommon_a_SOURCES = connman-dbus.c connman-dbus.h connman-dbus-glue.h \
				connman-client.h connman-client.c \
							instance.h instance.c \
				connman-agent.h connman-agent.c

BUILT_SOURCES = marshal.h marshal.c \
			connman-dbus-glue.h \
				instance-glue.h \
				connman-agent-glue.h

nodist_libcommon_a_SOURCES = connman-dbus-glue.h instance-glue.h connman-agent-glue.h

CLEANFILES = $(BUILT_SOURCES)

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@

EXTRA_DIST = marshal.list instance.xml connman-dbus.xml connman-agent.xml

MAINTAINERCLEANFILES = Makefile.in

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal $< --header > $@

marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal $< --body > $@

instance-glue.h: instance.xml
	$(DBUS_BINDING_TOOL) --prefix=instance --mode=glib-server --output=$@ $<

connman-dbus-glue.h: connman-dbus.xml
	$(DBUS_BINDING_TOOL) --prefix=connman --mode=glib-client --output=$@ $<

connman-agent-glue.h: connman-agent.xml
	$(DBUS_BINDING_TOOL) --prefix=connman_agent --mode=glib-server --output=$@ $<
