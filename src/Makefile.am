INCLUDES = \
	$(U2PS_CFLAGS) \
	-DU2PS_LOCALEDIR="\"$(U2PS_LOCALEDIR)\"" \
	-DPACKAGE="\"$(PACKAGE_NAME)\"" \
	-DVERSION="\"$(PACKAGE_VERSION)\"" \
	-DG_LOG_DOMAIN="\"$(PACKAGE_NAME)\""

bin_PROGRAMS = u2ps

u2ps_LDADD = \
	$(U2PS_LIBS)

u2ps_SOURCES = \
	main.c \
	mail.h \
	mail.c \
	util.h \
	util.c \
	shape_arabic.c
