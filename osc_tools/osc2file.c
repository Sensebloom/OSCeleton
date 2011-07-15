/*
 *  Copyright (C) 2004 Steve Harris, Uwe Koloska
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

/* Modifications made to make this write to file instead of stdout */

#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "lo/lo.h"

int done = 0;

int recording = 1;

lo_timetag start_time;

FILE * pFile;

void error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);

int stop_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);

int start_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);

int main(int argc, char * argv[])
{
	lo_timetag_now(&start_time);

	if (argc < 3)
	{
		printf("- Missing Arguments -\nUsage: osc2file port filename\n");
		exit(1);
	}

	char port[6];
	int ret;

	/* TODO: We could have some better port number checks here, rather than letting
	 * liblo handle it
	 */
	if ((ret = snprintf(port,6,"%s",argv[1])) < 0)
	{
		printf("- Invalid port -");
		exit(ret);
	}

	pFile = fopen(argv[2], "w");

	if(!pFile)
	{
		printf("- Couldn't open %s -", argv[2]);
		exit(ret);
	}

	printf("Initializing Server\n");

    /* start a new server on provided port */
    lo_server_thread st = lo_server_thread_new(port, error);

    /* add method that will match any path and args */
    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);

    /* add method that will match the path /quit with no args */
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

	/* add method that will match the path /start with no args */
	lo_server_thread_add_method(st, "/start", "", start_handler, NULL);

	/* add method that will match the path /stop with no args */
	lo_server_thread_add_method(st, "/stop", "", stop_handler, NULL);

    lo_server_thread_start(st);

    printf("Server Begin\n");
    printf("    send /start to begin recording\n");
    printf("    send /stop  to end recording\n");
    printf("    send /quit  to quit this tool\n\n");



    while (!done) {
#ifdef WIN32
    Sleep(1);
#else
	usleep(1000);
#endif
    }

    lo_server_thread_free(st);

    return 0;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
	/* data is the lo_message */
	if (!data)
		return 1;

	lo_timetag end_time;
	double diff;

	lo_timetag_now(&end_time);

	diff = lo_timetag_diff(end_time, start_time);

	lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);

	lo_message * msg = (lo_message *)data;

	lo_bundle_add_message(bundle, path, msg);

	const size_t data_len = lo_bundle_length(bundle);

	char *serial = (char *)lo_bundle_serialise(bundle, NULL, NULL);

	int i;

	if (recording)
	{
		char buff[255];

		/* Max out here */
		snprintf(buff,255,"%d %lf\n",data_len, diff);

		fputs(buff, pFile);

		fwrite((const void *)serial, sizeof(char),data_len, pFile);

		fputs("\n", pFile);

		fflush(pFile);
	}

	lo_timetag_now(&start_time);

    return 1;
}

int start_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    recording = 1;
    printf("Started Recording\n");
    lo_timetag_now(&start_time);

    return 0;
}

int stop_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    recording = 0;
    printf("Stopped Recording\n");
    lo_timetag_now(&start_time);

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");
    fflush(stdout);
    fclose(pFile);

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */
