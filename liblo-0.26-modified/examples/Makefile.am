MAINTAINERCLEANFILES = Makefile.in

noinst_PROGRAMS = example_server example_client nonblocking_server_example
AM_CFLAGS = -Wall -I@top_srcdir@

example_server_SOURCES = example_server.c
example_server_LDADD = ../src/liblo.la

example_client_SOURCES = example_client.c
example_client_LDADD = ../src/liblo.la

nonblocking_server_example_SOURCES = nonblocking_server_example.c
nonblocking_server_example_LDADD = ../src/liblo.la
