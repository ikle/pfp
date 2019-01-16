/*
 *  PCI finger-print detector
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int pfp_match (FILE *f)
{
	struct pfp_rule *r, *pattern;
	size_t count, rank;

	if ((r = pfp_scan ()) == NULL) {
		perror ("pfp scan");
		goto no_scan;
	}

	if ((pattern = pfp_parse (f)) == NULL) {
		perror ("pfp parse");
		goto no_parse;
	}

	count = pfp_rule_count (pattern);
	rank  = pfp_rule_match (r, pattern);

	if (verbose) {
		printf ("pattern size = %zd\n", count);
		printf ("match rank = %zd\n", rank);
	}

	pfp_rule_free (pattern);
	pfp_rule_free (r);
	return rank == count;
no_parse:
	pfp_rule_free (r);
no_scan:
	return -1;
}

static int do_match (void)
{
	int ret = pfp_match (stdin);

	return ret < 0 ? 1 : ret == 0 ? 2 : 0;
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

	if (argc == 2 && strcmp (argv[1], "match") == 0)
		return do_match ();

	fprintf (stderr, "usage:\n"
			 "\tpfp [-v] scan > out\n"
			 "\tpfp [-v] parse < in\n"
			 "\tpfp [-v] match < in\n");
	return 1;
}
