EXTRA_DIST = \
	COPYING \
	ChangeLog \
	INSTALL \
	NEWS \
	PROTOCOL \
	README \
	VERSION \
	gaim-netsoul.spec \
	netsoul_config.h
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src pixmaps

BUILT_SOURCES = netsoul_config.h

$(OBJECTS): $(BUILT_SOURCES)

netsoul_config.h: pre_config.h
	$(sedpath) 's/#define PACKAGE/#define SNPP_PACKAGE/g' pre_config.h > $@
