/*
 *  PCI finger-print detector
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfp-parser.h"
#include "pfp-scanner.h"

static void do_scan (void)
{
	struct pfp_rule *r;

	if ((r = pfp_scan ()) == NULL) {
		perror ("pfp scan");
		exit (1);
	}

	pfp_rule_show (r, stdout);
	pfp_rule_free (r);

	exit (0);
}

static void do_parse (void)
{
	struct pfp_rule *r;

	if ((r = pfp_parse ()) == NULL) {
		perror ("pfp parse");
		exit (1);
	}

	pfp_rule_show (r, stdout);
	pfp_rule_free (r);

	exit (0);
}

static void do_match (void)
{
	struct pfp_rule *r, *pattern;

	if ((r = pfp_scan ()) == NULL) {
		perror ("pfp scan");
		goto no_scan;
	}

	if ((pattern = pfp_parse ()) == NULL) {
		perror ("pfp parse");
		goto no_parse;
	}

	printf ("pattern size = %zd\n", pfp_rule_count (pattern));
	printf ("match rank = %zd\n", pfp_rule_match (r, pattern));

	pfp_rule_free (pattern);
	pfp_rule_free (r);
	exit (0);
no_parse:
	pfp_rule_free (r);
no_scan:
	exit (1);
}

int main (int argc, char *argv[])
{
	if (argc != 2)
		goto usage;

	if (strcmp (argv[1], "scan") == 0)
		do_scan ();
	else if (strcmp (argv[1], "parse") == 0)
		do_parse ();
	else if (strcmp (argv[1], "match") == 0)
		do_match ();
usage:
	fprintf (stderr, "usage:\n"
			 "\tpfp scan > out\n"
			 "\tpfp parse < in\n"
			 "\tpfp match < in\n");
	return 1;
}
