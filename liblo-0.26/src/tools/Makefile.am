AUTOMAKE_OPTIONS = foreign

SUBDIRS = .

bin_PROGRAMS = oscsend oscdump
oscsend_SOURCES = oscsend.c
oscsend_CFLAGS = $(LIBLO_CFLAGS)
oscsend_LDADD = ../liblo.la
oscdump_SOURCES = oscdump.c
oscdump_CFLAGS = $(LIBLO_CFLAGS)
oscdump_LDADD = ../liblo.la
