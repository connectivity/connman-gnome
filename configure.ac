AC_PREREQ(2.50)
AC_INIT()

AM_INIT_AUTOMAKE(connman-gnome, 0.5)
AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)

AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/share/locale",
				[Directory for the localization files])

COMPILER_FLAGS

AC_LANG_C

AC_PROG_CC
AC_PROG_CC_PIE
AC_PROG_INSTALL
AC_PROG_RANLIB

GETTEXT_PACKAGE=connman
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
					[The name of the gettext domain])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],
			[enable compiling with debugging information]), [
	if (test "${enableval}" = "yes" &&
				test "${ac_cv_prog_cc_g}" = "yes"); then
		CFLAGS="$CFLAGS -g -O0 -DDEBUG"
	fi
])

AC_ARG_ENABLE(pie, AC_HELP_STRING([--enable-pie],
			[enable position independent executables flag]), [
	if (test "${enableval}" = "yes" &&
				test "${ac_cv_prog_cc_pie}" = "yes"); then
		CFLAGS="$CFLAGS -fPIE"
		LDFLAGS="$LDFLAGS -pie"
	fi
])

PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= 0.73, dummy=yes,
				AC_MSG_ERROR(dbus-glib > = 0.73 is required))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

DBUS_BINDING_TOOL="dbus-binding-tool"
AC_SUBST(DBUS_BINDING_TOOL)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.8, dummy=yes,
				AC_MSG_ERROR(gtk+ >= 2.8 is required))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AC_OUTPUT(Makefile common/Makefile applet/Makefile properties/Makefile
					icons/Makefile po/Makefile.in)
