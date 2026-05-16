plugindir = @PLUGINSDIR@

plugin_LTLIBRARIES = libxfce4-gpu-plugin.la

libxfce4_gpu_plugin_la_SOURCES = \
	xfce4-gpu-plugin.c

libxfce4_gpu_plugin_la_CFLAGS = \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	-DGETTEXT_PACKAGE=\"xfce4-gpu-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

libxfce4_gpu_plugin_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'

libxfce4_gpu_plugin_la_LIBADD = \
	$(LIBXFCE4PANEL_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(GLIB_LIBS)

desktopdir = @DESKTOPDIR@

desktop_DATA = xfce4-gpu-plugin.desktop

xfce4-gpu-plugin.desktop: xfce4-gpu-plugin.desktop.in
	$(AM_V_GEN) sed \
		-e 's|@PLUGIN_PATH@|$(plugindir)|' \
		$< > $@

EXTRA_DIST = \
	xfce4-gpu-plugin.desktop.in

CLEANFILES = $(desktop_DATA)
