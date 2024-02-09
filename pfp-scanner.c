/*
 * PCI Finger-Print Bus Scanner
 *
 * Copyright (c) 2016-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>

#include <pci/pci.h>

#include "pfp-scanner.h"

struct pci_bus {
	struct pci_bus *next;
	struct pfp_sbdf root;
	int segment, bus;
	struct pci_dev *devices;
};

static struct pci_bus *pci_bus_alloc (int segment, int bus)
{
	struct pci_bus *o;

	if ((o = malloc (sizeof (*o))) == NULL)
		return NULL;

	o->next		= NULL;
	o->segment	= segment;
	o->bus		= bus;
	o->root.segment	= -1;
	o->devices	= NULL;
	return o;
}

static void pci_bus_free (struct pci_bus *o)
{
	struct pci_dev *p, *next;

	if (o == NULL)
		return;

	for (p = o->devices; p != NULL; p = next) {
		next = p->next;
		pci_free_dev (p);
	}

	free (o);
}

static void pci_bus_add (struct pci_bus *o, struct pci_dev *p)
{
	p->next = o->devices;
	o->devices = p;
}

struct pci_state {
	struct pci_access *pacc;
	struct pci_bus *list;
};

static struct pci_bus *
pci_state_find (struct pci_state *o, int segment, int bus, int alloc)
{
	struct pci_bus *p;

	for (p = o->list; p != NULL; p = p->next)
		if (p->segment == segment && p->bus == bus)
			return p;

	if (!alloc || (p = pci_bus_alloc (segment, bus)) == NULL)
		return NULL;

	p->next = o->list;
	o->list = p;
	return p;
}

static int pci_state_add (struct pci_state *o, struct pci_dev *p)
{
	struct pci_bus *bus;
	int i;

	if ((bus = pci_state_find (o, p->domain, p->bus, 1)) == NULL) {
		pci_free_dev (p);
		return 0;
	}

	pci_bus_add (bus, p);

	switch (pci_read_byte (p, PCI_HEADER_TYPE) & 0x7f) {
	case PCI_HEADER_TYPE_BRIDGE:
		i = pci_read_byte (p, PCI_SECONDARY_BUS);

		if ((bus = pci_state_find (o, p->domain, i, 1)) == NULL)
			return 0;

		bus->root.segment  = p->domain;
		bus->root.bus      = p->bus;
		bus->root.device   = p->dev;
		bus->root.function = p->func;
		break;
	}

	return 1;
}

static void recalc_segment (struct pci_bus *o)
{
	unsigned char s;

	if (o->root.segment >= 0)  /* not a root bridge */
		return;

	if (o->segment > 0 || o->bus == 0)  /* non-virtual segment */
		return;

	s = o->bus + (o->bus & 1);

	o->segment = (o->bus   & 0x01) |
		     ((s >> 6) & 0x02) |
		     ((s >> 4) & 0x04) |
		     ((s >> 2) & 0x08) |
		     ((s >> 0) & 0x10) |
		     ((s << 2) & 0x20) |
		     ((s << 4) & 0x40) |
		     ((s << 6) & 0x80);
}

static int pci_state_init (struct pci_state *s)
{
	struct pci_dev *p;
	struct pci_bus *bus;

	if ((s->pacc = pci_alloc ()) == NULL)
		return 0;

	pci_init (s->pacc);
	pci_scan_bus (s->pacc);

	s->list = NULL;

	for (p = s->pacc->devices; p != NULL; p = s->pacc->devices) {
		s->pacc->devices = p->next;  /* cut device */
		pci_state_add (s, p);
	}

	for (bus = s->list; bus != NULL; bus = bus->next)
		recalc_segment (bus);

	return 1;
}

static void pci_state_fini (struct pci_state *o)
{
	struct pci_bus *p, *next;

	for (p = o->list; p != NULL; p = next) {
		next = p->next;
		pci_bus_free (p);
	}

	pci_cleanup (o->pacc);
}

static struct pfp_rule *
pci_rule_alloc (struct pci_dev *dev, int verbose, const char *class)
{
	struct pfp_rule *o;
	int v;

	if ((o = pfp_rule_alloc ()) == NULL)
		return NULL;

	pci_fill_info (dev, PCI_FILL_IDENT | PCI_FILL_CLASS);

	o->parent.segment = -1;

	o->slot.segment  = dev->domain;
	o->slot.bus      = dev->bus;
	o->slot.device   = dev->dev;
	o->slot.function = dev->func;

	o->class     = dev->device_class;
	o->interface = pci_read_byte (dev, PCI_CLASS_PROG);

	o->vendor = dev->vendor_id;
	o->device = dev->device_id;

	v = pci_read_byte (dev, PCI_HEADER_TYPE) & 0x7f;

	if (v == PCI_HEADER_TYPE_NORMAL) {
		o->svendor = pci_read_word (dev, PCI_SUBSYSTEM_VENDOR_ID);
		o->sdevice = pci_read_word (dev, PCI_SUBSYSTEM_ID);
	}

	if (verbose)
		pfp_rule_fill (o, class);

	return o;
}

static const
struct pfp_rule *find_parent_rule (const struct pfp_rule *p, struct pfp_sbdf *o)
{
	for (; p != NULL; p = p->next)
		if (p->slot.segment  == o->segment	&&
		    p->slot.bus      == o->bus		&&
		    p->slot.device   == o->device	&&
		    p->slot.function == o->function)
			return p;

	return NULL;
}

static size_t write_segment (char *to, size_t avail, const struct pfp_rule *o)
{
	return snprintf (to, avail, "%x", o->segment);
}

static size_t write_path (char *to, size_t avail, const struct pfp_rule *o)
{
	size_t len;

	if (o == NULL || o == o->up)
		return snprintf (to, avail, "B");  /* buggy node */

	len = (o->up == NULL) ?
		write_segment (to, avail, o) :
		write_path (to, avail, o->up);

	to += len;
	avail = avail > len ? avail - len: 0;

	len += snprintf (to, avail, "/%x.%x", o->slot.device, o->slot.function);
	return len;
}

static char *calc_path (const struct pfp_rule *o)
{
	size_t len = write_path (NULL, 0, o);
	char *path;

	if ((path = malloc (len + 1)) == NULL)
		return path;

	write_path (path, len + 1, o);
	path[len] = '\0';
	return path;
}

struct pfp_rule *pfp_scan (int verbose, const char *class)
{
	struct pci_state s;
	struct pci_bus *bus;
	struct pci_dev *p;

	struct pfp_rule *head = NULL, **tail = &head, *rule;

	if (!pci_state_init (&s))
		return NULL;

	for (bus = s.list; bus != NULL; bus = bus->next)
		for (p = bus->devices; p != NULL; p = p->next) {
			if ((rule = pci_rule_alloc (p, verbose, class)) == NULL)
				goto error;

			*tail = rule;
			tail = &rule->next;

			if (bus->root.segment < 0) {
				rule->segment = bus->segment;
				continue;
			}

			rule->parent.segment  = bus->root.segment;
			rule->parent.bus      = bus->root.bus;
			rule->parent.device   = bus->root.device;
			rule->parent.function = bus->root.function;
		}

	for (rule = head; rule != NULL; rule = rule->next)
		if (rule->parent.segment >= 0)
			rule->up = find_parent_rule (head, &rule->parent);

	for (rule = head; rule != NULL; rule = rule->next)
		rule->path = calc_path (rule);

	pci_state_fini (&s);
	return head;
error:
	pci_state_fini (&s);
	pfp_rule_free (head);
	return NULL;
}
