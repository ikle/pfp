/*
 * PCI Finger-Print Rule
 *
 * Copyright (c) 2016-2020 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef PFP_RULE_H
#define PFP_RULE_H  1

#include <stdio.h>

struct pfp_bdf {
	int bus, device, function;
};

struct pfp_rule {
	struct pfp_rule *next;
	const struct pfp_rule *up;

	char *path;

	struct pfp_bdf parent, slot;
	int class, interface;
	int vendor, device;
	int svendor, sdevice;
};

struct pfp_rule *pfp_rule_alloc (void);
void pfp_rule_free (struct pfp_rule *o);

size_t pfp_rule_count (struct pfp_rule *o);
struct pfp_rule *pfp_rule_sort (struct pfp_rule *o);

void pfp_rule_show (struct pfp_rule *o, FILE *to);

const struct pfp_rule *
pfp_rule_search (const struct pfp_rule *o, const struct pfp_bdf *slot);

/* return number of matches */
size_t
pfp_rule_match (const struct pfp_rule *o, const struct pfp_rule *pattern);

#endif  /* PFP_RULE_H */
