/*
 * PCI Finger-Print Rule fill extra info helpers
 *
 * Copyright (c) 2016-2020 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include <dirent.h>

#include "pfp-rule.h"

static void pfp_rule_fill_name (struct pfp_rule *o)
{
	char path[128];
	int n;
	DIR *dir;
	struct dirent *de;

	if (o->name != NULL)
		return;

	n = snprintf (path, sizeof (path),
		      "/sys/bus/pci/devices/%04x:%02x:%02x.%o/net",
		      o->slot.segment, o->slot.bus,
		      o->slot.device, o->slot.function);

	if (n == sizeof (path))
		return;

	if ((dir = opendir (path)) == NULL)
		return;

	while ((de = readdir (dir)) != NULL) {
		if (de->d_name[0] == '.')
			continue;

		o->name = strdup (de->d_name);  /* use first device name */
		break;
	}

	closedir (dir);
}

void pfp_rule_fill (struct pfp_rule *o)
{
	pfp_rule_fill_name (o);
}
