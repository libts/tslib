/*
 *  tslib/tests/ts_conf.c
 *
 *  Copyright (C) 2018 Martin Kepplinger
 *
 * This file is part of tslib.
 *
 * ts_print_mt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ts_print_mt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ts_print_mt.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * Test the ts_config_filter API. Change filters at runtime.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#include "tslib.h"
#include "testutils.h"

static void usage(char **argv)
{
	ts_print_ascii_logo(16);
	printf("%s", tslib_version());
	printf("\n");
	printf("ts_conf tests changing filters and their parameters at runtime\n");
	printf("Usage: %s [--version] [--help] [--idev <input_dev>]\n",
		argv[0]);
}

static void print_conf(struct ts_module_conf *conf)
{
#if 1
	int forward = 0;
	struct ts_module_conf *conf_tmp;

	if (conf->next && !conf->prev)
		forward = 1;

	if (conf->prev && !conf->next)
		forward = 0;

	printf("ts_conf test: module %s\n", conf->name);

	if (forward) {
		while (conf->next) {
			conf = conf->next;
			printf("ts_conf test: module %s\n", conf->name);
		}
	} else {
		while (conf->prev) {
			conf = conf->prev;
			printf("ts_conf test: module %s\n", conf->name);
		}
	}

#endif
}

static int menu(struct tsdev *ts)
{
	int choice;
	int ret = 0;
	struct ts_module_conf *conf;

	printf("tslib filter configuration program\n");
	do {
		printf("\n");
		printf("1. reload ts.conf\n");
		printf("2. read and print ts.conf\n");
		printf("3. Exit\n");
		scanf("%d",&choice);

		switch (choice) {
		case 1:
			ret = ts_reconfig(ts);
			if (ret < 0)
				goto done;
			break;
		case 2:
			conf = ts_conf_get(ts, NULL);
			print_conf(conf);
			break;
		case 3:
			printf("Goodbye\n");
			break;
		default: printf("Unknown choice.\n");
			break;
		}
	} while (choice != 3);

done:
	ts_close(ts);

	return ret;
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	char *tsdevice = NULL;
	struct ts_lib_version_data *ver = ts_libversion();

#ifndef TSLIB_VERSION_MT /* < 1.10 */
	printf("You are running an old version of tslib. Please upgrade.\n");
#endif

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "version",      no_argument,       NULL, 'v' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hvi:", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(argv);
			return 0;

		case 'v':
			printf("%s\n", tslib_version());
			return 0;

		case 'i':
			tsdevice = optarg;
			break;

		default:
			usage(argv);
			return 0;
		}

		if (errno) {
			char str[9];

			sprintf(str, "option ?");
			str[7] = c & 0xff;
			perror(str);
		}
	}

	ts = ts_setup(tsdevice, 0);
	if (!ts) {
		perror("ts_setup");
		return errno;
	}

	printf("libts %06X opened device %s\n",
	       ver->version_num, ts_get_eventpath(ts));

	return menu(ts);
}
