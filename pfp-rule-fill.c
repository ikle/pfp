/*
 * PCI Finger-Print Rule fill extra info helpers
 *
 * Copyright (c) 2016-2020 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glob.h>
#include <unistd.h>

#include "pfp-rule.h"

static char *add_name (char *list, const char *name)
{
	int len;
	char *p;

	if (list == NULL)
		return strdup (name);

	len = snprintf (NULL, 0, "%s, %s", list, name) + 1;

	if ((p = malloc (len)) == NULL)
		return list;

	snprintf (p, len, "%s, %s", list, name);
	free (list);
	return p;
}

static char *get_device_name (const char *device)
{
	char *name = NULL, *p;
	glob_t g;
	size_t i;
	char link[128];
	ssize_t len;

	if (glob ("/sys/class/*/*/device", 0, NULL, &g) == 0)
		for (i = 0; i < g.gl_pathc; ++i) {
			len = readlink (g.gl_pathv[i], link, sizeof (link));

			if (len <= 0 || len >= sizeof (link))
				continue;

			link[len] = '\0';

			p = strrchr (link, '/');
			p = (p != NULL) ? p + 1 : link;

			if (strcmp (p, device) != 0)
				continue;

			if ((p = strrchr (g.gl_pathv[i], '/')) != NULL)
				*p = '\0';

			p = g.gl_pathv[i] + 11;

			if ((p = strchr (p, '/')) != NULL)
				*p = ' ';

			name = add_name (name, g.gl_pathv[i] + 11);
		}

	globfree (&g);
	return name;
}

static void pfp_rule_fill_name (struct pfp_rule *o)
{
	char device[16];

	if (o->name != NULL || o->slot.function > 8)
		return;

	snprintf (device, sizeof (device), "%04x:%02x:%02x.%o",
		  o->slot.segment, o->slot.bus,
		  o->slot.device,  o->slot.function);

	o->name = get_device_name (device);
}

void pfp_rule_fill (struct pfp_rule *o)
{
	pfp_rule_fill_name (o);
}
