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

static int do_match (void)
{
	struct pfp_rule *r, *pattern;
	size_t count, rank;

	if ((r = pfp_scan ()) == NULL) {
		perror ("pfp scan");
		goto no_scan;
	}

	if ((pattern = pfp_parse (stdin)) == NULL) {
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
	return rank == count ? 0 : 2;
no_parse:
	pfp_rule_free (r);
no_scan:
	return 1;
}

int main (int argc, char *argv[])
{
	if (argc > 1 && strcmp (argv[1], "-v") == 0) {
		--argc, ++argv;
		verbose = 1;
	}

	if (argc != 2)
		goto usage;

	if (strcmp (argv[1], "scan") == 0)
		return do_scan ();
	else if (strcmp (argv[1], "parse") == 0)
		return do_parse ();
	else if (strcmp (argv[1], "match") == 0)
		return do_match ();
usage:
	fprintf (stderr, "usage:\n"
			 "\tpfp [-v] scan > out\n"
			 "\tpfp [-v] parse < in\n"
			 "\tpfp [-v] match < in\n");
	return 1;
}
