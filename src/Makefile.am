EXTRA_DIST = \
	Makefile.mingw

plugindir = $(libdir)/purple-2

libnetsoul_la_LDFLAGS = -module -avoid-version -L/usr/lib

if PLUGINS

plugin_LTLIBRARIES = libnetsoul.la

libnetsoul_la_SOURCES = netsoul.c \
			netsoul.h \
			ns_buddy.c \
			ns_connection.c \
			ns_utils.c \
			ns_listen.c \
			ns_output.c \
			ns_chat.c \
			location_detector.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(libdir)/pidgin/\" \
	-DDATADIR=\"$(datadir)\" \
	$(GLIB_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(DEBUG_CFLAGS)

AM_CFLAGS=-DPURPLE_PLUGINS
