AC_PREREQ([2.69])
AC_INIT([xfce4-gpu-plugin], [1.0.0])
AC_CONFIG_SRCDIR([src/xfce4-gpu-plugin.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 foreign dist-bzip2])
AM_MAINTAINER_MODE([])
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT([disable-static])

PKG_CHECK_MODULES(LIBXFCE4PANEL, libxfce4panel-2.0 >= 4.14.0)
PKG_CHECK_MODULES(LIBXFCE4UTIL, libxfce4util-1.0 >= 4.14.0)
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.22.0)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.50.0)

PANEL_LIBDIR=`$PKG_CONFIG --variable=libdir libxfce4panel-2.0`
PLUGINSDIR="${PANEL_LIBDIR}/xfce4/panel/plugins"
DESKTOPDIR="${datadir}/xfce4/panel/plugins"

AC_SUBST([PLUGINSDIR])
AC_SUBST([DESKTOPDIR])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
