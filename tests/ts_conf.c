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

static void print_module_line(struct ts_module_conf *conf)
{
	if (conf->raw)
		printf("module_raw %s " YELLOW "%s" RESET "\n",
			conf->name, conf->params);
	else
		printf("%d: module %s " YELLOW "%s" RESET "\n",
			conf->nr, conf->name, conf->params);
}

static struct ts_module_conf *get_first(struct ts_module_conf *conf)
{
	struct ts_module_conf *conf_first = NULL;

	while (conf) {
		if (!conf->prev)
			conf_first = conf;

		conf = conf->prev;
	}
	conf = conf_first;

	return conf;
}

static void print_conf(struct ts_module_conf *conf)
{
	conf = get_first(conf);

	while (conf) {
		print_module_line(conf);
		conf = conf->next;
	}
}

static void edit_params(struct ts_module_conf *conf)
{
	int choice;
	char buf[1024] = { '\0' };
	int ret;
	int c;

	print_conf(conf);
	printf("Write parameters for module Nr.: ");
	ret = scanf("%d", &choice);
	if (ret <= 0) {
		perror("scanf");
		return;
	}

	conf = get_first(conf);

	while (conf) {
		if (conf->nr == choice) {
			printf("%s\n", conf->params);
			while ((c = getchar()) != '\n' && c != EOF) { }
			ret = scanf("%[^\n]s", buf);
			if (ret < 0)
				return;
			sprintf(conf->params, "%s", buf);
		}
		conf = conf->next;
	}
}

static int add_line_after(struct ts_module_conf *conf)
{
	struct ts_module_conf *new_filter;
	struct ts_module_conf *conf_last = NULL;
	int nr;
	int found = 0;
	int ret;
	int c;

	printf("add module after nr: ");
	ret = scanf("%d", &nr);
	if (ret <= 0) {
		perror("scanf");
		return -1;
	}

	conf = get_first(conf);

	while (conf) {
		if (conf->nr == nr) {
			found = 1;
			break;
		}
		if (!conf->next)
			conf_last = conf;
		conf = conf->next;
	}
	if (!found) {
		fprintf(stderr, "ts_conf: module not found\n");
		return -1;
	}
	if (conf_last)
		conf = conf_last;

	new_filter = calloc(1, sizeof(struct ts_module_conf));
	if (!new_filter)
		return -1;

	new_filter->name = calloc(1, 1024);
	if (!new_filter->name)
		goto err;

	new_filter->params = calloc(1, 1024);
	if (!new_filter->params)
		goto err;

	printf("new module name without parameters: ");
	ret = scanf("%s", new_filter->name);
	if (ret <= 0) {
		perror("scanf");
		goto err;
	}
	printf("parameters: ");
	while ((c = getchar()) != '\n' && c != EOF) { }
	ret = scanf("%[^\n]s", new_filter->params);
	if (ret < 0)
		goto err;
	new_filter->nr = ++nr;

	new_filter->prev = conf;
	new_filter->next = conf->next;
	conf->next = new_filter;

	while (conf) {
		if (conf->prev && conf->nr == conf->prev->nr)
			conf->nr++;
		conf = conf->next;
	}

	return 0;

err:
	if (new_filter) {
		free(new_filter->name);
		free(new_filter->params);
		free(new_filter);
	}

	return -1;
}

static void remove_line(struct ts_module_conf *conf)
{
	int nr;
	struct ts_module_conf *conf_first = NULL;
	struct ts_module_conf *conf_last = NULL;
	int found = 0;
	int ret;

	printf("remove module nr: ");
	ret = scanf("%d", &nr);
	if (ret <= 0) {
		perror("scanf");
		return;
	}

	conf = conf_first = get_first(conf);

	while (conf) {
		if (conf->nr == nr) {
			found = 1;
			break;
		}
		if (!conf->next)
			conf_last = conf;
		conf = conf->next;
	}
	if (!found) {
		fprintf(stderr, "ts_conf: module not found\n");
		return;
	}
	if (conf_last)
		conf = conf_last;


	if (conf->prev)
		conf->prev->next = conf->next;

	if (conf->next)
		conf->next->prev = conf->prev;

	free(conf->name);
	free(conf->params);
	free(conf);

	conf = conf_first;
	while (conf) {
		if (conf->prev && conf->nr == conf->prev->nr)
			conf->nr++;
		conf = conf->next;
	}
}

static int menu(struct tsdev *ts)
{
	int choice;
	int ret = 0;
	struct ts_module_conf *conf = NULL;

	printf("tslib: edit the configuration file and loaded filters\n");
	do {
		printf("\n");
		printf("1. manually reload ts.conf\n");
		printf("2. show ts.conf modules\n");
		printf("3. add one module\n");
		printf("4. change module parameters\n");
		printf("5. remove one module\n");
		printf("6. Exit\n");
		ret = scanf("%d",&choice);
		if (ret <= 0) {
			perror("scanf");
			return -1;
		}

		switch (choice) {
		case 1:
			ret = ts_reconfig(ts);
			if (ret < 0)
				goto done;
			break;
		case 2:
			conf = ts_conf_get(ts);
			print_conf(conf);

			/*
			 * ts_conf_set() frees resources.
			 * writing is unnecessarily done too.
			 */
			ret = ts_conf_set(conf);
			if (ret < 0)
				goto done;
			break;
		case 3:
			conf = ts_conf_get(ts);
			print_conf(conf);

			if (add_line_after(conf))
				goto done;

			print_conf(conf);
			ret = ts_conf_set(conf);
			if (ret < 0)
				goto done;

			ret = ts_reconfig(ts);
			if (ret < 0)
				goto done;
			break;
		case 4:
			conf = ts_conf_get(ts);
			print_conf(conf);

			edit_params(conf);

			print_conf(conf);
			ret = ts_conf_set(conf);
			if (ret < 0)
				goto done;

			ret = ts_reconfig(ts);
			if (ret < 0)
				goto done;
			break;
		case 5:
			conf = ts_conf_get(ts);
			print_conf(conf);

			remove_line(conf);

			print_conf(conf);
			ret = ts_conf_set(conf);
			if (ret < 0)
				goto done;

			ret = ts_reconfig(ts);
			if (ret < 0)
				goto done;
			break;
		case 6:
			printf("Goodbye\n");
			break;
		default: printf("Unknown choice.\n");
			break;
		}
	} while (choice != 6);

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
