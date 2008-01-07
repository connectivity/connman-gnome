
bin_PROGRAMS = connman-applet

connman_applet_SOURCES = main.c status.h status.c

connman_applet_LDADD = $(top_builddir)/common/libcommon.a \
					@GTK_LIBS@ @DBUS_LIBS@

if MAINTAINER_MODE
icondir = $(abs_top_srcdir)/icons
else
icondir = $(datadir)/icons/hicolor/22x22/apps
endif

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@ -DICONDIR=\""$(icondir)"\"

INCLUDES = -I$(top_srcdir)/common

autostartdir = $(sysconfdir)/xdg/autostart

autostart_in_files = connman-applet.desktop.in

autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(autostart_DATA)

EXTRA_DIST = $(autostart_in_files)

MAINTAINERCLEANFILES = Makefile.in
