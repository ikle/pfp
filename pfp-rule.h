/*
 * PCI Finger-Print Rule
 *
 * Copyright (c) 2016-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef PFP_RULE_H
#define PFP_RULE_H  1

#include <stdio.h>

struct pfp_sbdf {
	int segment;
	unsigned char bus, device, function;
};

struct pfp_rule {
	struct pfp_rule *next;
	const struct pfp_rule *up;

	char *path;

	int segment;  /* real segment */
	struct pfp_sbdf parent, slot;
	int class, interface;
	int vendor, device;
	int svendor, sdevice;

	char *name;
};

struct pfp_rule *pfp_rule_alloc (void);
void pfp_rule_free (struct pfp_rule *o);

/* load extra info */
void pfp_rule_fill (struct pfp_rule *o, const char *dev_class);

size_t pfp_rule_count (struct pfp_rule *o);
struct pfp_rule *pfp_rule_sort (struct pfp_rule *o);

void pfp_rule_show (struct pfp_rule *o, FILE *to);

const struct pfp_rule *
pfp_rule_search (const struct pfp_rule *o, const struct pfp_sbdf *slot);

/* return number of matches */
size_t
pfp_rule_match (const struct pfp_rule *o, const struct pfp_rule *pattern);

#endif  /* PFP_RULE_H */
