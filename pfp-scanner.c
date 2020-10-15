/*
 * PCI finger-print bus scanner
 */

#include <stdlib.h>
#include <string.h>

#include <pci/pci.h>

#include "pfp-scanner.h"

struct pci_state {
	struct pci_access *pacc;

	struct pci_state_bus {
		struct pci_dev *devices;
		u8 bus, device, function;  /* parent BDF */
		struct pfp_rule *link;     /* link to chain of rules to
					      accelerate parent search */
	} bus[256];
};

static int pci_state_init (struct pci_state *s)
{
	struct pci_dev *p;
	int i;

	if ((s->pacc = pci_alloc ()) == NULL)
		return 0;

	pci_init (s->pacc);
	pci_scan_bus(s->pacc);

	memset (s->bus, 0, sizeof (s->bus));

	for (p = s->pacc->devices; p != NULL; p = s->pacc->devices) {
		s->pacc->devices = p->next;  /* cut device */

		p->next = s->bus[p->bus].devices;
		s->bus[p->bus].devices = p;

		switch (pci_read_byte (p, PCI_HEADER_TYPE) & 0x7f) {
		case PCI_HEADER_TYPE_BRIDGE:
			i = pci_read_byte (p, PCI_SECONDARY_BUS);

			s->bus[i].bus      = p->bus;
			s->bus[i].device   = p->dev;
			s->bus[i].function = p->func;
			break;
		}
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

	return o;
}

static const
struct pfp_rule *find_parent_rule (const struct pfp_rule *p,
				   int bus, int device, int function)
{
	for (; p != NULL && p->slot.bus == bus; p = p->next)
		if (p->slot.device   == device &&
		    p->slot.function == function)
			return p;

	return NULL;
}

static size_t write_path (char *to, size_t avail, const struct pfp_rule *o)
{
	size_t len;

	if (o == NULL || o == o->up)
		return 0;

	len = write_path (to, avail, o->up);

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

			rule->parent.bus      = s.bus[i].bus;
			rule->parent.device   = s.bus[i].device;
			rule->parent.function = s.bus[i].function;
		}

	for (rule = head; rule != NULL; rule = rule->next) {
		i = rule->parent.bus;

		rule->up = find_parent_rule (
			s.bus[i].link,
			i, rule->parent.device, rule->parent.function
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
