/*
 * PCI Finger-Print Detector
 *
 * Copyright (c) 2016-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ftw.h>

#include "pfp-parser.h"
#include "pfp-scanner.h"

int verbose;

static int do_scan (void)
{
	struct pfp_rule *r;

	if ((r = pfp_scan (1, NULL)) == NULL) {
		perror ("pfp scan");
		return 1;
	}

	if ((r = pfp_rule_sort (r)) == NULL) {
		perror ("pfp sort");
		return 1;
	}

	pfp_rule_show (r, stdout);
	pfp_rule_free (r);

	return 0;
}

static int parse_slot (const char *slot, struct pfp_sbdf *o)
{
	if (sscanf (slot, "%x:%hhx:%hhx.%hho",
		    &o->segment, &o->bus, &o->device, &o->function) == 4)
		return 1;

	o->segment = 0;

	if (sscanf (slot, "%hhx:%hhx.%hho",
		    &o->bus, &o->device, &o->function) == 3)
		return 1;

	o->bus = 0;
	return sscanf (slot, "%hhx.%hho", &o->device, &o->function) == 2;
}

static int do_path (const char *slot)
{
	struct pfp_sbdf sbdf;
	struct pfp_rule *list;
	const struct pfp_rule *r;

	if (!parse_slot (slot, &sbdf)) {
		fprintf (stderr, "pfp path: cannot parse SBDF\n");
		return 1;
	}

	if ((list = pfp_scan (0, NULL)) == NULL) {
		perror ("pfp path");
		return 1;
	}

	if ((r = pfp_rule_search (list, &sbdf)) == NULL || r->path == NULL)
		goto no_device;

	printf ("%s\n", r->path);
	pfp_rule_free (list);
	return 0;
no_device:
	pfp_rule_free (list);
	return 1;
}

static int do_lookup (const char *path, const char *class)
{
	struct pfp_rule *list;
	const struct pfp_rule *o;
	const char *p;

	if ((list = pfp_scan (1, class)) == NULL) {
		perror ("pfp path");
		return 1;
	}

	for (o = list; o != NULL; o = o->next)
		if (o->path != NULL && strcmp (path, o->path) == 0)
			break;

	if (o == NULL || o->name == NULL || (p = strchr (o->name, ' ')) == NULL)
		goto no_name;

	printf ("%s\n", p + 1);
	pfp_rule_free (list);
	return 0;
no_name:
	pfp_rule_free (list);
	return 1;
}

static int do_parse (void)
{
	struct pfp_rule *r;

	if ((r = pfp_parse (stdin)) == NULL) {
		perror ("pfp parse");
		return 1;
	}

	if ((r = pfp_rule_sort (r)) == NULL) {
		perror ("pfp sort");
		return 1;
	}

	pfp_rule_show (r, stdout);
	pfp_rule_free (r);

	return 0;
}

static int pfp_match (FILE *f, size_t *rank, size_t *count)
{
	struct pfp_rule *r, *pattern;

	if ((r = pfp_scan (0, NULL)) == NULL) {
		perror ("pfp scan");
		goto no_scan;
	}

	if ((pattern = pfp_parse (f)) == NULL) {
		perror ("pfp parse");
		goto no_parse;
	}

	*count = pfp_rule_count (pattern);
	*rank  = pfp_rule_match (r, pattern);

	pfp_rule_free (pattern);
	pfp_rule_free (r);
	return 1;
no_parse:
	pfp_rule_free (r);
no_scan:
	return 0;
}

static struct ctx {
	size_t rank;
	char *path;
} walk_ctx;

static int match_walker (const char *path, const struct stat *sb, int type)
{
	const char *dot;
	FILE *f;
	size_t rank, count;

	if (type != FTW_F)
		return 0;

	if ((dot = strrchr (path, '.')) == NULL || strcmp (dot, ".pfp") != 0)
		return 0;

	if ((f = fopen (path, "r")) == NULL)
		goto no_open;

	if (!pfp_match (f, &rank, &count))
		goto no_match;

	if (rank == count && walk_ctx.rank < rank) {
		walk_ctx.rank = rank;
		free (walk_ctx.path);
		walk_ctx.path = strdup (path);
	}

	if (verbose > 0)
		printf ("%s: %zd/%zd\n", path, rank, count);

	fclose (f);
	return 0;
no_match:
	fclose (f);
no_open:
	return -1;
}

static int do_match_dirs (char *argv[])
{
	for (; argv[0] != NULL; ++argv)
		if (ftw (argv[0], match_walker, 1000) < 0)
			return 1;

	if (walk_ctx.path == NULL)
		return 2;

	printf ("%s\n", walk_ctx.path);
	free (walk_ctx.path);
	return 0;
}

static int do_match (char *argv[])
{
	size_t rank, count;

	if (argv[0] != NULL)
		return do_match_dirs (argv);

	if (!pfp_match (stdin, &rank, &count))
		return 1;

	if (verbose > 0)
		printf ("match rank = %zd/%zd\n", rank, count);

	return rank != count ? 2 : 0;
}

int main (int argc, char *argv[])
{
	while (argc > 1 && strcmp (argv[1], "-v") == 0) {
		--argc, ++argv;
		++verbose;
	}

	if (argc == 2 && strcmp (argv[1], "scan") == 0)
		return do_scan ();

	if (argc == 3 && strcmp (argv[1], "path") == 0)
		return do_path (argv[2]);

	if (argc == 4 && strcmp (argv[1], "lookup") == 0)
		return do_lookup (argv[2], argv[3]);

	if (argc == 2 && strcmp (argv[1], "parse") == 0)
		return do_parse ();

	if (argc >= 2 && strcmp (argv[1], "match") == 0)
		return do_match (argv + 2);

	fprintf (stderr, "usage:\n"
			 "\tpfp [-v] scan > out\n"
			 "\tpfp [-v] path SBDF\n"
			 "\tpfp [-v] lookup PATH CLASS\n"
			 "\tpfp [-v] parse < in\n"
			 "\tpfp [-v] match < in\n"
			 "\tpfp [-v] match rule-directory ...\n");
	return 1;
}
