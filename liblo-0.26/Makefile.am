SUBDIRS = src examples lo build @DOXYGEN@

EXTRA_DIST = libtool ltmain.sh autogen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblo.pc

test: all
	(cd src && make test)
