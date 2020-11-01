/*
 * PCI Finger-Print Parser
 *
 * Copyright (c) 2016-2020 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef PFP_PARSER_H
#define PFP_PARSER_H  1

#include <stdio.h>
#include "pfp-rule.h"

struct pfp_parser *pfp_parser_alloc (FILE *from);
void pfp_parser_free (struct pfp_parser *o);

struct pfp_rule *pfp_parser_run (struct pfp_parser *o);
void pfp_parser_reset (struct pfp_parser *o, FILE *from);

/* all in one */
struct pfp_rule *pfp_parse (FILE *from);

#endif  /* PFP_PARSER_H */
