/*
 *  PCI finger-print rule
 */

#include <stdlib.h>
#include "pfp-rule.h"

extern int verbose;

struct pfp_rule *pfp_rule_alloc (void)
{
	struct pfp_rule *o;

	if ((o = malloc (sizeof (*o))) == NULL)
		return NULL;

	o->next = NULL;
	o->up   = NULL;

	o->parent.bus = -1;
	o->slot.bus   = -1;

	o->interface = o->class = -1;

	o->device  = o->vendor  = -1;
	o->sdevice = o->svendor = -1;

	return o;
}

void pfp_rule_free (struct pfp_rule *o)
{
	struct pfp_rule *next;

	for (; o != NULL; o = next) {
		next = o->next;
		free (o);
	}
}

size_t pfp_rule_count (struct pfp_rule *o)
{
	size_t count;

	for (count = 0; o != NULL; ++count, o = o->next) {}

	return count;
}

static int rule_cmp (const void *a, const void *b)
{
	const struct pfp_rule *const *p = a, *const *q = b;

	if (p[0]->slot.bus != q[0]->slot.bus)
		return p[0]->slot.bus - q[0]->slot.bus;

	if (p[0]->slot.device != q[0]->slot.device)
		return p[0]->slot.device - q[0]->slot.device;

	return p[0]->slot.function - q[0]->slot.function;
}

struct pfp_rule *pfp_rule_sort (struct pfp_rule *o)
{
	struct pfp_rule **set;
	size_t count = pfp_rule_count (o), i;

	if ((set = malloc (sizeof (set[0]) * count)) == NULL) {
		pfp_rule_free (o);
		return NULL;
	}

	for (i = 0; i < count; ++i, o = o->next)
		set[i] = o;

	qsort (set, count, sizeof (set[0]), rule_cmp);

	for (i = 0; i < (count - 1); ++i)
		set[i]->next = set[i + 1];

	set[count - 1]->next = NULL;

	o = set[0];
	free (set);
	return o;
}

static void show_bdf (struct pfp_bdf *o, const char *prefix, FILE *to)
{
	if (o->bus < 0)
		return;

	fprintf (to, "%s\t= ", prefix);

	if (o->bus != 0)
		fprintf (to, "%x:", o->bus);

	fprintf (to, "%x.%x\n", o->device, o->function);
}

static void show_id (int id, const char *prefix, FILE *to)
{
	if (id >= 0)
		fprintf (to, "%s\t= %04x\n", prefix, id);
}

static void show_path_iter (const struct pfp_rule *o, FILE *to)
{
	if (o == NULL || o == o->up)
		return;

	show_path_iter (o->up, to);
	fprintf (to, "/%x.%x", o->slot.device, o->slot.function);
}

static void show_path (const struct pfp_rule *o, const char *prefix, FILE *to)
{
	fprintf (to, "%s\t= ", prefix);
	show_path_iter (o, to);
	fputc ('\n', to);
}

static void show_rule (struct pfp_rule *o, FILE *to)
{
	if (verbose > 0)
		show_path (o, "path", to);

	if (o->slot.bus != 0)
		show_bdf (&o->parent, "parent", to);

	show_bdf (&o->slot, "slot", to);

	if (o->interface >= 0)
		fprintf (to, "class\t= %04x.%x\n", o->class, o->interface);
	else
		show_id (o->class, "class", to);

	show_id (o->vendor, "vendor", to);
	show_id (o->device, "device", to);

	if ((o->svendor != 0 && o->svendor != 0xffff &&
	     o->svendor != o->vendor) ||
	    verbose > 1) {
		show_id (o->svendor, "svendor", to);
		show_id (o->sdevice, "sdevice", to);
	}
}

void pfp_rule_show (struct pfp_rule *o, FILE *to)
{
	for (; o != NULL; o = o->next) {
		show_rule (o, to);

		if (o->next != NULL)
			fprintf (to, "\n");
	}
}

static int slot_match (struct pfp_bdf *o, struct pfp_bdf *pattern)
{
	if (pattern->bus < 0)
		return 1;

	return o->bus      == pattern->bus    &&
	       o->device   == pattern->device &&
	       o->function == pattern->function;
}

static int id_match (int id, int pattern)
{
	if (pattern < 0)
		return 1;

	return id == pattern;
}

static int rule_match (struct pfp_rule *o, struct pfp_rule *pattern)
{
	return slot_match (&o->parent, &pattern->parent)	&&
	       slot_match (&o->slot,   &pattern->slot)		&&
	       id_match (o->class, pattern->class)		&&
	       id_match (o->interface, pattern->interface)	&&
	       id_match (o->vendor, pattern->vendor)		&&
	       id_match (o->device, pattern->device)		&&
	       id_match (o->svendor, pattern->svendor)		&&
	       id_match (o->sdevice, pattern->sdevice);
}

/* return number of matches */
size_t pfp_rule_match (struct pfp_rule *o, struct pfp_rule *pattern)
{
	struct pfp_rule *p;
	size_t count;

	for (count = 0; o != NULL; o = o->next)
		for (p = pattern; p != NULL; p = p->next)
			if (rule_match (o, p))
				++count;

	return count;
}
