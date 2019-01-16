/*
 *  PCI finger-print detector
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

	if ((r = pfp_scan ()) == NULL) {
		perror ("pfp scan");
		return 1;
	}

	pfp_rule_show (r, stdout);
	pfp_rule_free (r);

	return 0;
}

static int do_parse (void)
{
	struct pfp_rule *r;

	if ((r = pfp_parse (stdin)) == NULL) {
		perror ("pfp parse");
		return 1;
	}

	pfp_rule_show (r, stdout);
	pfp_rule_free (r);

	return 0;
}

static int pfp_match (FILE *f, size_t *rank, size_t *count)
{
	struct pfp_rule *r, *pattern;

	if ((r = pfp_scan ()) == NULL) {
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
	FILE *f;
	size_t rank, count;

	if (type != FTW_F)
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

	if (verbose)
		printf ("match rank = %zd/%zd\n", rank, count);

	return rank != count ? 2 : 0;
}

int main (int argc, char *argv[])
{
	if (argc > 1 && strcmp (argv[1], "-v") == 0) {
		--argc, ++argv;
		verbose = 1;
	}

	if (argc == 2 && strcmp (argv[1], "scan") == 0)
		return do_scan ();

	if (argc == 2 && strcmp (argv[1], "parse") == 0)
		return do_parse ();

	if (argc >= 2 && strcmp (argv[1], "match") == 0)
		return do_match (argv + 2);

	fprintf (stderr, "usage:\n"
			 "\tpfp [-v] scan > out\n"
			 "\tpfp [-v] parse < in\n"
			 "\tpfp [-v] match < in\n"
			 "\tpfp [-v] match rule-directory ...\n");
	return 1;
}
