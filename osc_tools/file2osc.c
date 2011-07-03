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

/* Modified to read osc bundles from a file and output them */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_MAX 255

#include "lo/lo.h"

int main(int argc, char *argv[])
{
	int ret;
	FILE * fp;

	if (argc < 3)
	{
		printf("- Missing Arguments -\nUsage: file2osc filename address port\n");
		exit(1);
	}

	int countar;

	fp = fopen(argv[1], "r");

	if(!fp)
	{
		printf("- Couldn't open %s -", argv[1]);
		exit(ret);
	}

    lo_address addr = lo_address_new(argv[2], argv[3]);

    char str[200];

    if(!fp) return 1; // bail out if file not found

    /*
    while(fgets(str,sizeof(str),fp) != NULL)
    {
      // strip trailing '\n' if it exists
      int len = strlen(str)-1;
      if(str[len] == '\n')
         str[len] = 0;
      printf("\n%s", str);
    }*/
    int len = 0;
    double sec = 0;

    size_t result;

    while (1 == 1)
    {
		if ((ret = fscanf(fp, "%d %lf\n", &len, &sec)) != 2)
		{
			printf("Couldn't read file any further\n");
			fclose(fp);
			exit(0);
		}

		printf("length %d sec %f\n", len, sec);

		char * buffer = (char *) malloc (sizeof(char)*len);

		result = fread (buffer,sizeof(char),len, fp);

		if (result != len)
		{fputs ("Reading error",stderr); fclose(fp); exit (3);}

		printf("readit %.*s\n", len, buffer);

		usleep((unsigned long) (1000000 * sec));
		send_data(addr, NULL, buffer, len);

    }

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */

