/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

/*
 * This is some testcase code - it exercises the internals of liblo, so its not
 * a good example to learn from, see examples/ for example code
 */

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

#include "lo_types_internal.h"
#include "lo_internal.h"
#include "lo/lo.h"
#include "config.h"

#ifdef WIN32
#define PATHDELIM "\\"
#else
#define PATHDELIM "/"
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define TEST(cond) if (!(cond)) { fprintf(stderr, "FAILED " #cond \
					  " at %s:%d\n", __FILE__, __LINE__); \
				  exit(1); } \
		   else { printf("passed " #cond "\n"); }

union end_test32 {
    uint32_t i;
    char     c[4];
};

union end_test64 {
    uint64_t i;
    char     c[8];
};

static int done = 0;
static int bundle_count = 0;
static int pattern_count = 0;
static int reply_count = 0;
static int subtest_count = 0;
static int subtest_reply_count = 0;
static int error_okay = 0;

char testdata[5] = "ABCDE";

static int jitter_count = 0;
static float jitter_total = 0.0f;
static float jitter_max = 0.0f;
static float jitter_min = 1000.0f;

void exitcheck(void);
void test_deserialise(void);
void test_validation(lo_address a);
void test_multicast(lo_server_thread st);
void error(int num, const char *m, const char *path);
void rep_error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, lo_message data, void *user_data);

int foo_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int reply_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int lots_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int coerce_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int bundle_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int timestamp_handler(const char *path, const char *types, lo_arg **argv, 
                 int argc, lo_message data, void *user_data);

int jitter_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int pattern_handler(const char *path, const char *types, lo_arg **argv,
	            int argc, lo_message data, void *user_data);

int subtest_handler(const char *path, const char *types, lo_arg **argv,
	            int argc, lo_message data, void *user_data);

int subtest_reply_handler(const char *path, const char *types, lo_arg **argv,
	            int argc, lo_message data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data);

int test_varargs(lo_address a, const char *path, const char *types, ...);

int main()
{
    lo_blob btest = lo_blob_new(sizeof(testdata), testdata);
    lo_server_thread st, sta, stb;
    lo_server s = lo_server_new(NULL, error);
    lo_bundle b;
    lo_message m1, m2;
    char *server_url, *path, *protocol, *host, *port;
    const char *host2, *port2;
    lo_address a;
    uint8_t midi_data[4] = {0xff, 0xf7, 0xAA, 0x00};
    union end_test32 et32;
    union end_test64 et64;
    lo_timetag tt = {0x1, 0x80000000}, sched;
    int count;
    int proto;
    char cmd[256];

    test_deserialise();

    sta = lo_server_thread_new("7591", error);
    stb = lo_server_thread_new("7591", rep_error);
    if (stb) {
	fprintf(stderr, "FAILED: create bad server thread object!\n");
	exit(1);
    }
    lo_server_thread_free(sta);

    /* leak check */
    st = lo_server_thread_new(NULL, error);
    lo_server_thread_start(st);
#ifdef WIN32
    Sleep(4);
#else
    usleep(4000);
#endif
    lo_server_thread_stop(st);
    lo_server_thread_free(st);
    st = lo_server_thread_new(NULL, error);
    lo_server_thread_start(st);
    lo_server_thread_stop(st);
    lo_server_thread_free(st);
    st = lo_server_thread_new(NULL, error);
    lo_server_thread_free(st);
    st = lo_server_thread_new(NULL, error);
    lo_server_thread_free(st);
    st = lo_server_thread_new(NULL, error);

    a = lo_address_new_from_url("osc://localhost/");
    TEST(a != NULL);
    lo_address_free(a);

    a = lo_address_new_from_url("osc.://localhost/");
    TEST(a == NULL);


    atexit(exitcheck);

    printf("type tests\n");
    TEST(sizeof(float) == sizeof(int32_t));
    TEST(sizeof(double) == sizeof(int64_t));

    et32.i = 0x23242526U;
    et32.i = lo_htoo32(et32.i);
    if (et32.c[0] != 0x23 || et32.c[1] != 0x24 || et32.c[2] != 0x25 ||
	et32.c[3] != 0x26) {
	fprintf(stderr, "failed 32bit endian conversion test\n");
	fprintf(stderr, "0x23242526 -> %X\n", et32.i);
	exit(1);
    } else {
	printf("passed 32bit endian conversion test\n");
    }

    et64.i = 0x232425262728292AULL;
    et64.i = lo_htoo64(et64.i);
    if (et64.c[0] != 0x23 || et64.c[1] != 0x24 || et64.c[2] != 0x25 ||
	et64.c[3] != 0x26 || et64.c[4] != 0x27 || et64.c[5] != 0x28 ||
	et64.c[6] != 0x29 || et64.c[7] != 0x2A) {
	fprintf(stderr, "failed 64bit endian conversion\n");
	fprintf(stderr, "0x232425262728292A -> %llX\n", (long long unsigned int)et64.i);
	exit(1);
    } else {
	printf("passed 64bit endian conversion\n");
    }
    printf("\n");

    /* OSC URL tests */
    path = lo_url_get_path("osc.udp://localhost:9999/a/path/is/here");
    if (strcmp(path, "/a/path/is/here")) {
	printf("failed lo_url_get_path() test1\n");
	printf("'%s' != '/a/path/is/here'\n", path);
	exit(1);
    } else {
	printf("passed lo_url_get_path() test1\n");
    }
    free(path);

    protocol = lo_url_get_protocol("osc.udp://localhost:9999/a/path/is/here");
    if (strcmp(protocol, "udp")) {
	printf("failed lo_url_get_protocol() test1\n");
	printf("'%s' != 'udp'\n", protocol);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol() test1\n");
    }
    free(protocol);

    protocol = lo_url_get_protocol("osc.tcp://localhost:9999/a/path/is/here");
    if (strcmp(protocol, "tcp")) {
	printf("failed lo_url_get_protocol() test2\n");
	printf("'%s' != 'tcp'\n", protocol);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol() test2\n");
    }
    free(protocol);
    
    protocol = lo_url_get_protocol("osc.udp://[::ffff:localhost]:9999/a/path/is/here");
    if (strcmp(protocol, "udp")) {
	printf("failed lo_url_get_protocol() test1 (IPv6)\n");
	printf("'%s' != 'udp'\n", protocol);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol() test1 (IPv6)\n");
    }
    free(protocol);

    proto = lo_url_get_protocol_id("osc.udp://localhost:9999/a/path/is/here");
    if (proto != LO_UDP) {
	printf("failed lo_url_get_protocol_id() test1\n");
	printf("'%d' != LO_UDP\n", proto);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol_id() test1\n");
    }

    proto = lo_url_get_protocol_id("osc.tcp://localhost:9999/a/path/is/here");
    if (proto != LO_TCP) {
	printf("failed lo_url_get_protocol_id() test2\n");
	printf("'%d' != LO_TCP\n", proto);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol_id() test2\n");
    }
    
    proto = lo_url_get_protocol_id("osc.invalid://localhost:9999/a/path/is/here");
    if (proto != -1) {
	printf("failed lo_url_get_protocol_id() test3\n");
	printf("'%d' != -1\n", proto);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol_id() test3\n");
    }
    
    proto = lo_url_get_protocol_id("osc.udp://[::ffff:localhost]:9999/a/path/is/here");
    if (proto != LO_UDP) {
	printf("failed lo_url_get_protocol_id() test1 (IPv6)\n");
	printf("'%d' != LO_UDP\n", proto);
	exit(1);
    } else {
	printf("passed lo_url_get_protocol_id() test1 (IPv6)\n");
    }

    host = lo_url_get_hostname("osc.udp://foo.example.com:9999/a/path/is/here");
    if (strcmp(host, "foo.example.com")) {
	printf("failed lo_url_get_hostname() test1\n");
	printf("'%s' != 'foo.example.com'\n", host);
	exit(1);
    } else {
	printf("passed lo_url_get_hostname() test1\n");
    }
    free(host);

    host = lo_url_get_hostname("osc.udp://[0000::::0001]:9999/a/path/is/here");
    if (strcmp(host, "0000::::0001")) {
	printf("failed lo_url_get_hostname() test2 (IPv6)\n");
	printf("'%s' != '0000::::0001'\n", host);
	exit(1);
    } else {
	printf("passed lo_url_get_hostname() test2 (IPv6)\n");
    }
    free(host);

    port = lo_url_get_port("osc.udp://localhost:9999/a/path/is/here");
    if (strcmp(port, "9999")) {
	printf("failed lo_url_get_port() test1\n");
	printf("'%s' != '9999'\n", port);
	exit(1);
    } else {
	printf("passed lo_url_get_port() test1\n");
    }
    free(port);
    
    port = lo_url_get_port("osc.udp://[::ffff:127.0.0.1]:9999/a/path/is/here");
    if (strcmp(port, "9999")) {
	printf("failed lo_url_get_port() test1 (IPv6)\n");
	printf("'%s' != '9999'\n", port);
	exit(1);
    } else {
	printf("passed lo_url_get_port() test1 (IPv6)\n");
    }
    free(port);
    printf("\n");
    
    
    
    
    
    a = lo_address_new_from_url("osc.tcp://foo.example.com:9999/");
    host2 = lo_address_get_hostname(a);
    if (strcmp(host2, "foo.example.com")) {
	printf("failed lo_address_get_hostname() test\n");
	printf("'%s' != 'foo.example.com'\n", host2);
	exit(1);
    } else {
	printf("passed lo_address_get_hostname() test\n");
    }

    port2 = lo_address_get_port(a);
    if (strcmp(port2, "9999")) {
	printf("failed lo_address_get_port() test\n");
	printf("'%s' != '9999'\n", port2);
	exit(1);
    } else {
	printf("passed lo_address_get_port() test\n");
    }

    proto = lo_address_get_protocol(a);
    if (proto != LO_TCP) {
	printf("failed lo_address_get_protocol() test\n");
	printf("'%d' != '%d'\n", proto, LO_TCP);
	exit(1);
    } else {
	printf("passed lo_address_get_protocol() test\n");
    }

    server_url = lo_address_get_url(a);
    if (strcmp(server_url, "osc.tcp://foo.example.com:9999/")) {
	printf("failed lo_address_get_url() test\n");
	printf("'%s' != '%s'\n", server_url, "osc.tcp://foo.example.com:9999/");
	exit(1);
    } else {
	printf("passed lo_address_get_url() test\n");
    }
    free(server_url);
    lo_address_free( a );
    printf("\n");
    

    /* Test blod sizes */
    if (lo_blob_datasize(btest) != 5 || lo_blobsize(btest) != 12) {
	printf("blob is %d (%d) bytes long, should be 5 (12)\n",
               lo_blob_datasize(btest), lo_blobsize(btest));
	lo_arg_pp(LO_BLOB, btest);
	printf(" <- blob\n");
	exit(1);
    }
    
    
    
    /* Server method handler tests */
    server_url = lo_server_thread_get_url(st);
    a = lo_address_new_from_url(server_url);
    printf("Server URL: %s\n", server_url);
    free(server_url);

    /* add method that will match the path /foo/bar, with two numbers, coerced
     * to float and int */

    lo_server_thread_add_method(st, "/foo/bar", "fi", foo_handler, lo_server_thread_get_server(st));

    lo_server_thread_add_method(st, "/reply", "s", reply_handler, NULL);

    lo_server_thread_add_method(st, "/lotsofformats", "fisbmhtdSccTFNI",
				lots_handler, NULL);

    lo_server_thread_add_method(st, "/coerce", "dfhiSs",
				coerce_handler, NULL);

    lo_server_thread_add_method(st, "/bundle", NULL,
				bundle_handler, NULL);
    lo_server_thread_add_method(st, "/timestamp", NULL,
				timestamp_handler, NULL);
    lo_server_thread_add_method(st, "/jitter", "ti",
				jitter_handler, NULL);

    lo_server_thread_add_method(st, "/pattern/foo", NULL,
				pattern_handler, "foo");
    lo_server_thread_add_method(st, "/pattern/bar", NULL,
				pattern_handler, "bar");
    lo_server_thread_add_method(st, "/pattern/baz", NULL,
				pattern_handler, "baz");

    lo_server_thread_add_method(st, "/subtest", "i",
				subtest_handler, st);

    lo_server_thread_add_method(st, "/subtest-reply", "i",
				subtest_reply_handler, NULL);

    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);

    /* add method that will match the path /quit with no args */
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

    /* check that the thread restarts */
    lo_server_thread_start(st);
    lo_server_thread_stop(st);
    lo_server_thread_start(st);

    if (lo_send(a, "/foo/bar", "ff", 0.12345678f, 23.0f) == -1) {
	printf("OSC error A %d: %s\n", lo_address_errno(a), lo_address_errstr(a));
	exit(1);
    }

    if (lo_send(a, "/foo/bar", "ff", 0.12345678f, 23.0f) == -1) {
	printf("OSC error B %d: %s\n", lo_address_errno(a), lo_address_errstr(a));
	exit(1);
    }

    test_validation(a);
    test_multicast(st);

    lo_send(a, "/", "i", 242);
    lo_send(a, "/pattern/", "i", 243);

#ifndef _MSC_VER  /* MS compiler refuses to compile this case */
    lo_send(a, "/bar", "ff", 0.12345678f, 1.0/0.0);
#endif
    lo_send(a, "/lotsofformats", "fisbmhtdSccTFNI", 0.12345678f, 123, "123",
	    btest, midi_data, 0x0123456789abcdefULL, tt, 0.9999, "sym",
	    'X', 'Y');
    lo_send(a, "/coerce", "fdihsS", 0.1f, 0.2, 123, 124LL, "aaa", "bbb");
    lo_send(a, "/coerce", "ffffss", 0.1f, 0.2f, 123.0, 124.0, "aaa", "bbb");
    lo_send(a, "/coerce", "ddddSS", 0.1, 0.2, 123.0, 124.0, "aaa", "bbb");
    lo_send(a, "/a/b/c/d", "sfsff", "one", 0.12345678f, "three",
	    -0.00000023001f, 1.0);
    lo_send(a, "/a/b/c/d", "b", btest);

    TEST(test_varargs(a, "/lotsofformats", "fisbmhtdSccTFNI", 0.12345678f, 123,
                      "123", btest, midi_data, 0x0123456789abcdefULL, tt,
                      0.9999, "sym", 'X', 'Y', LO_ARGS_END) == 0);

#ifdef __GNUC__
    // Note: Lack of support for variable-argument macros in non-GCC compilers
    //       does not allow us to test for these conditions.

    // too many args
    TEST(test_varargs(a, "/lotsofformats", "f", 0.12345678f, 123,
                      "123", btest, midi_data, 0x0123456789abcdefULL, tt,
                      0.9999, "sym", 'X', 'Y', LO_ARGS_END) != 0);
    // too many types
    TEST(test_varargs(a, "/lotsofformats", "fisbmhtdSccTFNI", 0.12345678f, 123,
                      "123", btest, midi_data, 0x0123456789abcdefULL, tt, 0.5,
                      LO_ARGS_END) != 0);
#endif

    // test lo_message_add
    m1 = lo_message_new();
    TEST(lo_message_add(m1, "fisbmhtdSccTFNI", 0.12345678f, 123, "123",
                        btest, midi_data, 0x0123456789abcdefULL, tt, 0.9999, "sym",
                        'X', 'Y') == 0);
    lo_send_message(a, "/lotsofformats", m1);
    lo_message_free(m1);
 
    lo_blob_free(btest);

    lo_send(a, "/pattern/*", "s", "a");
    lo_send(a, "/pattern/ba[rz]", "s", "b");

    server_url = lo_server_thread_get_url(st);
    sprintf(cmd, "." PATHDELIM "subtest %s &", server_url);
    if (system(cmd) != 0) {
	fprintf(stderr, "Cannot execute subtest command\n");
	exit(1);
    }
    system(cmd);
    free(server_url);

#ifdef WIN32
    Sleep(2000);
#else
    sleep(2);
#endif
    TEST(reply_count == 3);
    TEST(pattern_count == 5);
    TEST(subtest_count == 2);
    TEST(subtest_reply_count == 22);
    printf("\n");

    {
        lo_timetag t = {10,0xFFFFFFFC};
        b = lo_bundle_new(t);
    }
    m1 = lo_message_new();
    lo_message_add_string(m1, "abcdefghijklmnopqrstuvwxyz");
    lo_message_add_string(m1, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    lo_bundle_add_message(b, "/bundle", m1);
    lo_send_bundle(a, b);

    /* This should be safe for multiple copies of the same message. */
    lo_bundle_free_messages(b);

    {
        lo_timetag t = {1,2};
        b = lo_bundle_new(t);
    }
    m1 = lo_message_new();
    lo_message_add_int32(m1, 23);
    lo_message_add_string(m1, "23");
    lo_bundle_add_message(b, "/bundle", m1);
    m2 = lo_message_new();
    lo_message_add_string(m2, "24");
    lo_message_add_int32(m2, 24);
    lo_bundle_add_message(b, "/bundle", m2);
    lo_bundle_add_message(b, "/bundle", m1);

/* 
    lo_send_bundle(a, b);
    if (a->errnum) {
	printf("error %d: %s\n", a->errnum, a->errstr);
	exit(1);
    }
*/
    TEST(lo_send_bundle(a, b) == 88);

    /* Test freeing out-of-order copies of messages in a bundle. */
    lo_bundle_free_messages(b);

    {
        lo_timetag t = {10,0xFFFFFFFE};
        b = lo_bundle_new(t);
    }
    m1 = lo_message_new();
    lo_message_add_string(m1, "abcdefghijklmnopqrstuvwxyz");
    lo_message_add_string(m1, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    lo_bundle_add_message(b, "/bundle", m1);
    lo_send_bundle(a, b);
    lo_message_free(m1);
    lo_bundle_free(b);

    lo_timetag_now(&sched);

    sched.sec += 5;
    b = lo_bundle_new(sched);
    m1 = lo_message_new();
    lo_message_add_string(m1, "future");
    lo_message_add_string(m1, "time");
    lo_message_add_string(m1, "test");
    lo_bundle_add_message(b, "/bundle", m1);

    lo_send_bundle(a, b);
    lo_message_free(m1);
    lo_bundle_free(b);

    lo_send_timestamped(a, sched, "/bundle", "s", "lo_send_timestamped() test");

    /* test bundle timestamp ends up in message struct (and doesn't end up in
       unbundled messages) */
    lo_timetag_now(&sched);    
    lo_send_timestamped(a, sched, "/timestamp", "it", 1, sched);
    lo_send(a, "/timestamp", "it", 0, sched);

#define JITTER_ITS 25
    /* jitter tests */
    {
	lo_timetag stamps[JITTER_ITS];
	lo_timetag now;
	int i;

	for (i=0; i<JITTER_ITS; i++) {
	    lo_timetag_now(&now);
	    stamps[i] = now;
	    stamps[i].sec += 1;
	    stamps[i].frac = rand();
	    lo_send_timestamped(a, stamps[i], "/jitter", "ti", stamps[i], i);
	}
    }

#ifdef WIN32
    Sleep(2000);
#else
    sleep(2);
#endif

    lo_address_free(a);

    TEST(lo_server_thread_events_pending(st));

    while (lo_server_thread_events_pending(st)) {
	printf("pending events, wait...\n");
#ifdef WIN32
	fflush(stdout);
	Sleep(1000);
#else
	sleep(1);
#endif
    }
    
    TEST(bundle_count == 7);
    printf("\n");

    printf("bundle timing jitter results:\n"
	   "max jitter = %fs\n"
	   "avg jitter = %fs\n"
           "min jitter = %fs\n\n",
           jitter_max, jitter_total/(float)jitter_count, jitter_min);

    server_url = lo_server_get_url(s);

    lo_server_add_method(s, NULL, NULL, generic_handler, NULL);
    a = lo_address_new_from_url(server_url);
    TEST(lo_server_recv_noblock(s, 0) == 0);
    printf("Testing noblock API on %s\n", server_url);
    lo_send(a, "/non-block-test", "f", 23.0);

    count = 0;
    while (!lo_server_recv_noblock(s, 10) && count++ < 1000) { }
    if (count >= 1000) {
	printf("lo_server_recv_noblock() test failed\n");

	exit(1);
    }

    /* Delete methods */
    lo_server_thread_del_method(st, "/coerce", "dfhiSs");
    lo_server_del_method(s, NULL, NULL);

    lo_address_free(a);
    lo_server_free(s);
    free(server_url);

#ifndef WIN32
    { /* UNIX domain tests */
	lo_address ua;
	lo_server us;
	char *addr;

	unlink("/tmp/testlo.osc");
	us = lo_server_new_with_proto("/tmp/testlo.osc", LO_UNIX, error);
	ua = lo_address_new_from_url("osc.unix:///tmp/testlo.osc");
	TEST(lo_server_get_protocol(us) == LO_UNIX);
	TEST(lo_send(ua, "/unix", "f", 23.0) == 16);
	TEST(lo_server_recv(us) == 16);
	addr = lo_server_get_url(us);
	TEST(!strcmp("osc.unix:////tmp/testlo.osc", addr));
	free(addr);
	lo_address_free(ua);
	ua = lo_address_new_with_proto(LO_UNIX, NULL, "/tmp/testlo.osc");
	TEST(lo_send(ua, "/unix", "f", 23.0) == 16);
	TEST(lo_server_recv(us) == 16);
	lo_server_free(us);
	lo_address_free(ua);
    }
#endif

    { /* TCP tests */
	lo_address ta;
	lo_server ts;
	char *addr;

	ts = lo_server_new_with_proto(NULL, LO_TCP, error);
	addr = lo_server_get_url(ts);
	ta = lo_address_new_from_url(addr);
	if (lo_address_errno(ta)) {
	    printf("err: %s\n", lo_address_errstr(ta));
	    exit(1);
	}
	TEST(lo_server_get_protocol(ts) == LO_TCP);
	TEST(lo_send(ta, "/tcp", "f", 23.0) == 16);
	TEST(lo_send(ta, "/tcp", "f", 23.0) == 16);
	TEST(lo_server_recv(ts) == 16);
	TEST(lo_server_recv(ts) == 16);
	free(addr);
	lo_server_free(ts);
	lo_address_free(ta);
    }

    server_url = lo_server_thread_get_url(st);
    a = lo_address_new_from_url(server_url);
    /* exit */
    lo_send(a, "/quit", NULL);
    lo_address_free(a);

    while (!done) {
#ifdef WIN32
    Sleep(1);
#else
	usleep(1000);
#endif
    }

    lo_server_thread_free(st);
    free(server_url);


    return 0;
}

void exitcheck(void)
{
    if (!done) {
	fprintf(stderr, "\ntest run not completed\n" PACKAGE_NAME
		" test FAILED\n");
    } else {
	printf(PACKAGE_NAME " test PASSED\n");
    }
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in %s: %s", num, path, msg);
    if (!error_okay)
        exit(1);
    else
        printf(" (expected)\n");
}

void rep_error(int num, const char *msg, const char *path)
{
    if (num != 9904) {
	error(num, msg, path);
    }
}

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, lo_message data, void *user_data)
{
    int i;

    printf("path: <%s>\n", path);
    for (i=0; i<argc; i++) {
	printf("arg %d '%c' ", i, types[i]);
	lo_arg_pp(types[i], argv[i]);
	printf("\n");
    }
    printf("\n");

    return 1;
}

int foo_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data)
{
    lo_server serv = (lo_server)user_data;
    lo_address src = lo_message_get_source(data);
    char *url = lo_address_get_url(src);
    char *server_url = lo_server_get_url(serv);
    printf("Address of us: %s\n", server_url);
    printf("%s <- f:%f, i:%d\n", path, argv[0]->f, argv[1]->i);
    if (lo_send_from(src, serv, LO_TT_IMMEDIATE, "/reply", "s", "a reply") == -1) {
	printf("OSC reply error %d: %s\nSending to %s\n", lo_address_errno(src), lo_address_errstr(src), url);
	exit(1);
    } else {
	printf("Reply sent to %s\n\n", url);
    }
    free(server_url);
    free(url);

    return 0;
}

int reply_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data)
{
    lo_address src = lo_message_get_source(data);
    char *url = lo_address_get_url(src);
    printf("Reply received from %s\n", url);
    free(url);
    reply_count++;

    return 0;
}

int lots_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data)
{
    lo_blob b;
    unsigned char *d;

    if (strcmp(path, "/lotsofformats")) {
	fprintf(stderr, "path != /lotsofformats\n");
	exit(1);
    }
    printf("path = %s\n", path);
    TEST(types[0] == 'f' && argv[0]->f == 0.12345678f);
    TEST(types[1] == 'i' && argv[1]->i == 123);
    TEST(types[2] == 's' && !strcmp(&argv[2]->s, "123"));
    b = (lo_blob)argv[3];
    d = lo_blob_dataptr(b);
    TEST(types[3] == 'b' && lo_blob_datasize(b) == 5);
    TEST(d[0] == 'A' && d[1] == 'B' && d[2] == 'C' && d[3] == 'D' &&
	 d[4] == 'E');
    d = argv[4]->m;
    TEST(d[0] == 0xff && d[1] == 0xf7 && d[2] == 0xaa && d[3] == 0x00);
    TEST(types[5] == 'h' && argv[5]->h == 0x0123456789ABCDEFULL);
    TEST(types[6] == 't' && argv[6]->t.sec == 1 && \
	 argv[6]->t.frac == 0x80000000);
    TEST(types[7] == 'd' && argv[7]->d == 0.9999);
    TEST(types[8] == 'S' && !strcmp(&argv[8]->S, "sym"));
printf("char: %d\n", argv[9]->c);
    TEST(types[9] == 'c' && argv[9]->c == 'X');
    TEST(types[10] == 'c' && argv[10]->c == 'Y');
    TEST(types[11] == 'T');
    TEST(types[12] == 'F');
    TEST(types[13] == 'N');
    TEST(types[14] == 'I');

    printf("\n");

    return 0;
}

int coerce_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data)
{
    printf("path = %s\n", path);
    TEST(types[0] == 'd' && fabs(argv[0]->d - 0.1) < FLT_EPSILON);
    TEST(types[1] == 'f' && fabs(argv[1]->f - 0.2) < FLT_EPSILON);
    TEST(types[2] == 'h' && argv[2]->h == 123);
    TEST(types[3] == 'i' && argv[3]->i == 124);
    TEST(types[4] == 'S' && !strcmp(&argv[4]->S, "aaa"));
    TEST(types[5] == 's' && !strcmp(&argv[5]->s, "bbb"));
    printf("\n");

    return 0;
}

int bundle_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data)
{
    bundle_count++;
    printf("received bundle\n");

    return 0;
}

int timestamp_handler(const char *path, const char *types, lo_arg **argv, 
		      int argc, lo_message data, void *user_data)
{
    int bundled = argv[0]->i;

    lo_timetag ts, arg_ts;
    ts = lo_message_get_timestamp(data);
    arg_ts = argv[1]->t;

    if (bundled) {
      TEST((ts.sec == arg_ts.sec) && (ts.frac == arg_ts.frac));
    }
    else {
      TEST(ts.sec == LO_TT_IMMEDIATE.sec && ts.frac == LO_TT_IMMEDIATE.frac);
    }
    return 0;
}

int jitter_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 lo_message data, void *user_data)
{
    lo_timetag now;
    float jitter;

    lo_timetag_now(&now);
    jitter = fabs(lo_timetag_diff(now, argv[0]->t));
    jitter_count++;
    //printf("jitter: %f\n", jitter);
    printf("%d expected: %x:%x received %x:%x\n", argv[1]->i, argv[0]->t.sec,
	   argv[0]->t.frac, now.sec, now.frac);
    jitter_total += jitter;
    if (jitter > jitter_max) jitter_max = jitter;
    if (jitter < jitter_min) jitter_min = jitter;

    return 0;
}

int pattern_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, lo_message data, void *user_data)
{
    pattern_count++;
    printf("pattern matched %s\n", (char *)user_data);

    return 0;
}

int subtest_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, lo_message data, void *user_data)
{
    lo_address a = lo_message_get_source(data);

    subtest_count++;
    printf("got subtest message %d\n", subtest_count);
    lo_send_from(a, lo_server_thread_get_server(user_data),
                 LO_TT_IMMEDIATE, "/subtest", "i", subtest_count);

    return 0;
}

int subtest_reply_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, lo_message data, void *user_data)
{
    subtest_reply_count++;
    //printf("got subtest reply message %d\n", subtest_reply_count);

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message data, void *user_data)
{
    done = 1;

    return 0;
}

int test_varargs(lo_address a, const char *path, const char *types, ...)
{
    va_list ap;
    lo_message m = lo_message_new();
    int error;
    va_start(ap, types);
    if ((error=lo_message_add_varargs(m, types, ap))==0)
        lo_send_message(a, path, m);
    else
        printf("lo_message_add_varargs returned %d\n", error);
    lo_message_free(m);
    return error<0;
}

void replace_char(char *str, size_t size, const char find, const char replace)
{
    char *p = str;
    while(size--)
    {
        if (find == *p) { *p = replace; }
        ++p;
    }
}

void test_deserialise()
{
    char *buf, *buf2, *tmp;
    const char *types = NULL, *path;
    lo_arg **argv = NULL;
    size_t len, size;
    char data[256];
    int result = 0;

    lo_blob btest = lo_blob_new(sizeof(testdata), testdata);
    uint8_t midi_data[4] = {0xff, 0xf7, 0xAA, 0x00};
    lo_timetag tt = {0x1, 0x80000000};
    lo_blob b = NULL;

    // build a message
    lo_message msg = lo_message_new();
    TEST(0 == lo_message_get_argc(msg));
    lo_message_add_float(msg, 0.12345678f);             // 0  f
    lo_message_add_int32(msg, 123);                     // 1  i
    lo_message_add_string(msg, "123");                  // 2  s
    lo_message_add_blob(msg, btest);                    // 3  b
    lo_message_add_midi(msg, midi_data);                // 4  m
    lo_message_add_int64(msg, 0x0123456789abcdefULL);   // 5  h
    lo_message_add_timetag(msg, tt);                    // 6  t
    lo_message_add_double(msg, 0.9999);                 // 7  d
    lo_message_add_symbol(msg, "sym");                  // 8  S
    lo_message_add_char(msg, 'X');                      // 9  c
    lo_message_add_char(msg, 'Y');                      // 10 c
    lo_message_add_true(msg);                           // 11 T
    lo_message_add_false(msg);                          // 12 F
    lo_message_add_nil(msg);                            // 13 N
    lo_message_add_infinitum(msg);                      // 14 I

    // test types, args
    TEST(15 == lo_message_get_argc(msg));
    types = lo_message_get_types(msg);
    TEST(NULL != types);
    argv = lo_message_get_argv(msg);
    TEST(NULL != argv);
    TEST('f' == types[0] && fabs(argv[0]->f - 0.12345678f) < FLT_EPSILON);
    TEST('i' == types[1] && 123 == argv[1]->i);
    TEST('s' == types[2] && !strcmp(&argv[2]->s, "123"));
    TEST('b' == types[3]);
    b = (lo_blob)argv[3];
    TEST(lo_blob_datasize(b) == sizeof(testdata));
    TEST(12 == lo_blobsize(b));
    TEST(!memcmp(lo_blob_dataptr(b), &testdata, sizeof(testdata)));
    TEST('m' == types[4]  && !memcmp(&argv[4]->m, midi_data, 4));
    TEST('h' == types[5]  && 0x0123456789abcdefULL == argv[5]->h);
    TEST('t' == types[6]  && 1 == argv[6]->t.sec && 0x80000000 == argv[6]->t.frac);
    TEST('d' == types[7]  && fabs(argv[7]->d - 0.9999) < FLT_EPSILON);
    TEST('S' == types[8]  && !strcmp(&argv[8]->s, "sym"));
    TEST('c' == types[9]  && 'X' == argv[9]->c);
    TEST('c' == types[10] && 'Y' == argv[10]->c);
    TEST('T' == types[11] && NULL == argv[11]);
    TEST('F' == types[12] && NULL == argv[12]);
    TEST('N' == types[13] && NULL == argv[13]);
    TEST('I' == types[14] && NULL == argv[14]);

    // serialise it
    len = lo_message_length(msg, "/foo");
    printf("serialise message_length=%d\n", (int)len);
    buf = calloc(len, sizeof(char));
    size = 0;
    tmp = lo_message_serialise(msg, "/foo", buf, &size);
    TEST(tmp == buf && size == len && 92 == len);
    lo_message_free(msg);

    // deserialise it
    printf("deserialise\n");
    path = lo_get_path(buf, len);
    TEST(NULL != path && !strcmp(path, "/foo"));
    msg = lo_message_deserialise(buf, size, NULL);
    TEST(NULL != msg);

    // repeat same test as above
    TEST(15 == lo_message_get_argc(msg));
    types = lo_message_get_types(msg);
    TEST(NULL != types);
    argv = lo_message_get_argv(msg);
    TEST(NULL != argv);
    TEST('f' == types[0] && fabs(argv[0]->f - 0.12345678f) < FLT_EPSILON);
    TEST('i' == types[1] && 123 == argv[1]->i);
    TEST('s' == types[2] && !strcmp(&argv[2]->s, "123"));
    TEST('b' == types[3]);
    b = (lo_blob)argv[3];
    TEST(lo_blob_datasize(b) == sizeof(testdata));
    TEST(12 == lo_blobsize(b));
    TEST(!memcmp(lo_blob_dataptr(b), &testdata, sizeof(testdata)));
    TEST('m' == types[4]  && !memcmp(&argv[4]->m, midi_data, 4));
    TEST('h' == types[5]  && 0x0123456789abcdefULL == argv[5]->h);
    TEST('t' == types[6]  && 1 == argv[6]->t.sec && 0x80000000 == argv[6]->t.frac);
    TEST('d' == types[7]  && fabs(argv[7]->d - 0.9999) < FLT_EPSILON);
    TEST('S' == types[8]  && !strcmp(&argv[8]->s, "sym"));
    TEST('c' == types[9]  && 'X' == argv[9]->c);
    TEST('c' == types[10] && 'Y' == argv[10]->c);
    TEST('T' == types[11] && NULL == argv[11]);
    TEST('F' == types[12] && NULL == argv[12]);
    TEST('N' == types[13] && NULL == argv[13]);
    TEST('I' == types[14] && NULL == argv[14]);

    // serialise it again, compare
    len = lo_message_length(msg, "/foo");
    printf("serialise message_length=%d\n", (int)len);
    buf2 = calloc(len, sizeof(char));
    size = 0;
    tmp = lo_message_serialise(msg, "/foo", buf2, &size);
    TEST(tmp == buf2 && size == len && 92 == len);
    TEST(!memcmp(buf, buf2, len));
    lo_message_free(msg);

    lo_blob_free(btest);
    free(buf);
    free(buf2);

    // deserialise failure tests with invalid message data

    msg = lo_message_deserialise(data, 0, &result); // 0 size
    TEST(NULL == msg && LO_ESIZE == result);

    snprintf(data, 256, "%s", "/foo"); // unterminated path string
    msg = lo_message_deserialise(data, 4, &result);
    TEST(NULL == msg && LO_EINVALIDPATH == result);

    snprintf(data, 256, "%s", "/f_o"); // non-0 in pad area
    msg = lo_message_deserialise(data, 4, &result);
    TEST(NULL == msg && LO_EINVALIDPATH == result);

    snprintf(data, 256, "%s", "/t__"); // types missing
    replace_char(data, 4, '_', '\0');
    msg = lo_message_deserialise(data, 4, &result);
    TEST(NULL == msg && LO_ENOTYPE == result);

    snprintf(data, 256, "%s%s", "/t__", "____"); // types empty
    replace_char(data, 8, '_', '\0');
    msg = lo_message_deserialise(data, 8, &result);
    TEST(NULL == msg && LO_EBADTYPE == result);

    snprintf(data, 256, "%s%s", "/t__", ",f_"); // short message
    replace_char(data, 7, '_', '\0');
    msg = lo_message_deserialise(data, 7, &result);
    TEST(NULL == msg && LO_EINVALIDTYPE == result);

    snprintf(data, 256, "%s%s", "/t__", "ifi_"); // types missing comma
    replace_char(data, 8, '_', '\0');
    msg = lo_message_deserialise(data, 8, &result);
    TEST(NULL == msg && LO_EBADTYPE == result);

    snprintf(data, 256, "%s%s", "/t__", ",ifi"); // types unterminated
    replace_char(data, 8, '_', '\0');
    msg = lo_message_deserialise(data, 8, &result);
    TEST(NULL == msg && LO_EINVALIDTYPE == result);

    snprintf(data, 256, "%s%s", "/t__", ",ii_"); // not enough arg data
    replace_char(data, 8, '_', '\0');
    msg = lo_message_deserialise(data, 12, &result);
    TEST(NULL == msg && LO_EINVALIDARG == result);

    snprintf(data, 256, "%s%s", "/t__", ",ii_"); // not enough arg data again
    replace_char(data, 8, '_', '\0');
    msg = lo_message_deserialise(data, 15, &result);
    TEST(NULL == msg && LO_EINVALIDARG == result);

    snprintf(data, 256, "%s%s", "/t__", ",f__"); // too much arg data
    replace_char(data, 8, '_', '\0');
    msg = lo_message_deserialise(data, 16, &result);
    TEST(NULL == msg && LO_ESIZE == result);

    snprintf(data, 256, "%s%s", "/t__", ",bs_"); // blob longer than msg length
    replace_char(data, 8, '_', '\0');
    *(uint32_t *)(data + 8) = lo_htoo32((uint32_t)99999);
    msg = lo_message_deserialise(data, 256, &result);
    TEST(NULL == msg && LO_EINVALIDARG == result);
}

void test_validation(lo_address a)
{
    /* packet crafted to crash a lo_server when no input validation is performed */
    char mem[] = {"/\0\0\0,bs\0,\x00\x0F\x42\x3F"}; // OSC:  "/" ",bs" 999999
    int eok = error_okay;
    int sock = a->socket;

    /* This code won't work with MSVC because the lo_client_sockets data structure
     * is not explicitly made available to external programs.  We could expose it
     * in debug mode, perhaps, but let's just skip this test for now.  (Can be tested
     * on Windows using MingW.) */
#ifdef _MSC_VER
    return;
#else

    printf("validation\n");

    if (sock == -1)
        sock = lo_client_sockets.udp;
    if (sock == -1) {
        fprintf(stderr, "Couldn't get socket in test_validation(), %s:%d\n", __FILE__, __LINE__);
        exit(1);
    }

    error_okay = 1;
    if (sendto(sock, &mem, sizeof(mem), MSG_NOSIGNAL,
               a->ai->ai_addr, a->ai->ai_addrlen)==-1) {
        fprintf(stderr, "Error sending packet in test_validation(), %s:%d\n", __FILE__, __LINE__);
    }
#ifndef WIN32
    usleep(10000);
#else
    Sleep(10);
#endif
    error_okay = eok;
#endif
}

void test_multicast(lo_server_thread st)
{
    /* test multicast server and sender */
    /* message is sent from st otherwise reply doesn't work */
    lo_server ms = lo_server_new_multicast("224.0.1.1", "15432", error);
    lo_address ma = lo_address_new("224.0.1.1", "15432");
    lo_address_set_ttl(ma, 1);
    lo_server_add_method(ms, "/foo/bar", "fi", foo_handler, ms);
    lo_server_add_method(ms, "/reply", "s", reply_handler, NULL);
    if (lo_send_from(ma, lo_server_thread_get_server(st), LO_TT_IMMEDIATE,
                     "/foo/bar", "ff", 0.12345678f, 23.0f) == -1) {
        printf("multicast send error %d: %s\n", lo_address_errno(ma), lo_address_errstr(ma));
        exit(1);
    }
    TEST(lo_server_recv(ms)==24);
    lo_server_free(ms);
    lo_address_free(ma);
}

/* vi:set ts=8 sts=4 sw=4: */
