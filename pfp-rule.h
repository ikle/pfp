/*
 *  PCI finger-print rule
 */

#ifndef _PFP_RULE_H
#define _PFP_RULE_H  1

#include <stdio.h>

struct pfp_bdf {
	int bus, device, function;
};

struct pfp_rule {
	struct pfp_rule *next;
	const struct pfp_rule *up;

	struct pfp_bdf parent, slot;
	int class, interface;
	int vendor, device;
	int svendor, sdevice;
};

struct pfp_rule *pfp_rule_alloc (void);
void pfp_rule_free (struct pfp_rule *o);

size_t pfp_rule_count (struct pfp_rule *o);

void pfp_rule_show (struct pfp_rule *o, FILE *to);

/* return number of matches */
size_t pfp_rule_match (struct pfp_rule *o, struct pfp_rule *pattern);

#endif  /* _PFP_RULE_H */
