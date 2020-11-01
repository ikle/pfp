/*
 * PCI Finger-Print Bus Scanner
 *
 * Copyright (c) 2016-2020 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>

#include <pci/pci.h>

#include "pfp-scanner.h"

struct pci_state {
	struct pci_access *pacc;

	struct pci_state_bus {
		struct pci_dev *devices;
		struct pfp_sbdf parent;
		struct pfp_rule *link;     /* link to chain of rules to
					      accelerate parent search */
	} bus[256];
};

static void pci_state_bus_init (struct pci_state *s)
{
	int i;

	for (i = 0; i < 256; ++i) {
		s->bus[i].devices = NULL;
		s->bus[i].parent.segment = -1;\
		s->bus[i].link = NULL;
	}
}

static void pci_state_add (struct pci_state *s, struct pci_dev *p)
{
	int i;

	p->next = s->bus[p->bus].devices;
	s->bus[p->bus].devices = p;

	switch (pci_read_byte (p, PCI_HEADER_TYPE) & 0x7f) {
	case PCI_HEADER_TYPE_BRIDGE:
		i = pci_read_byte (p, PCI_SECONDARY_BUS);

		s->bus[i].parent.segment  = p->domain;
		s->bus[i].parent.bus      = p->bus;
		s->bus[i].parent.device   = p->dev;
		s->bus[i].parent.function = p->func;
		break;
	}
}

static int pci_state_init (struct pci_state *s)
{
	struct pci_dev *p;

	if ((s->pacc = pci_alloc ()) == NULL)
		return 0;

	pci_init (s->pacc);
	pci_scan_bus(s->pacc);

	pci_state_bus_init (s);

	for (p = s->pacc->devices; p != NULL; p = s->pacc->devices) {
		s->pacc->devices = p->next;  /* cut device */
		pci_state_add (s, p);
	}

	return 1;
}

static void pci_state_fini (struct pci_state *s)
{
	int i;
	struct pci_dev *p, *next;

	for (i = 0; i < 256; ++i)
		for (p = s->bus[i].devices; p != NULL; p = next) {
			next = p->next;
			pci_free_dev (p);
		}

	pci_cleanup (s->pacc);
}

static struct pfp_rule *pci_rule_alloc (struct pci_dev *dev)
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

	pfp_rule_fill (o);
	return o;
}

static const
struct pfp_rule *find_parent_rule (const struct pfp_rule *p, struct pfp_sbdf *o)
{
	for (; p != NULL && p->slot.bus == o->bus; p = p->next)
		if (p->slot.device   == o->device &&
		    p->slot.function == o->function)
			return p;

	return NULL;
}

static size_t write_segment (char *to, size_t avail, const struct pfp_rule *o)
{
	return snprintf (to, avail, "%x", o->slot.segment);
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

struct pfp_rule *pfp_scan (void)
{
	struct pci_state s;
	int i;
	struct pci_dev *p;

	struct pfp_rule *head = NULL, **tail = &head, *rule;

	if (!pci_state_init (&s))
		return NULL;

	for (i = 0; i < 256; ++i)
		for (p = s.bus[i].devices; p != NULL; p = p->next) {
			if ((rule = pci_rule_alloc (p)) == NULL)
				goto error;

			*tail = rule;
			tail = &rule->next;

			if (s.bus[rule->slot.bus].link == NULL)
				s.bus[rule->slot.bus].link = rule;

			rule->parent.segment  = s.bus[i].parent.segment;
			rule->parent.bus      = s.bus[i].parent.bus;
			rule->parent.device   = s.bus[i].parent.device;
			rule->parent.function = s.bus[i].parent.function;
		}

	for (rule = head; rule != NULL; rule = rule->next) {
		if ((i = rule->parent.segment) < 0)
			continue;

		if (i != 0) {
			fprintf (stderr, "oops\n");
			continue;
		}

		rule->up = find_parent_rule (
			s.bus[rule->parent.bus].link,
			&rule->parent
		);
	}

	for (rule = head; rule != NULL; rule = rule->next)
		rule->path = calc_path (rule);

	pci_state_fini (&s);
	return head;
error:
	pci_state_fini (&s);
	pfp_rule_free (head);
	return NULL;
}
