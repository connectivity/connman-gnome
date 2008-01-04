
bin_PROGRAMS = connman-applet

connman_applet_SOURCES = main.c status.h status.c

connman_applet_LDADD = @GTK_LIBS@ @DBUS_LIBS@

if MAINTAINER_MODE
icondir = $(abs_top_srcdir)/icons
else
icondir = $(datadir)/icons/hicolor/22x22/apps
endif

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@ -DICONDIR=\""$(icondir)"\"

MAINTAINERCLEANFILES = Makefile.in
