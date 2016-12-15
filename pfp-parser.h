/*
 *  PCI finger-print parser
 */

#ifndef _PFP_PARSER_H
#define _PFP_PARSER_H  1

#include "pfp-rule.h"

struct pfp_parser *pfp_parser_alloc (void);
void pfp_parser_free (struct pfp_parser *o);

struct pfp_rule *pfp_parser_run (struct pfp_parser *o);

/* all in one */
struct pfp_rule *pfp_parse (void);

#endif  /* _PFP_PARSER_H */
