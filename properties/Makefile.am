
bin_PROGRAMS = connman-properties

connman_properties_SOURCES = main.c advanced.h advanced.c 80203.c 80211.c

connman_properties_LDADD = $(top_builddir)/common/libcommon.a \
						@GTK_LIBS@ @DBUS_LIBS@

if MAINTAINER_MODE
icondir = $(abs_top_srcdir)/icons
else
icondir = $(datadir)/icons/hicolor/22x22/apps
endif

AM_CFLAGS = @DBUS_CFLAGS@ @GTK_CFLAGS@ -DICONDIR=\""$(icondir)"\"

INCLUDES = -I$(top_srcdir)/common

desktopdir = $(datadir)/applications

desktop_in_files = connman-properties.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(desktop_DATA)

EXTRA_DIST = $(desktop_in_files)

MAINTAINERCLEANFILES = Makefile.in
