/*
 * PCI Finger-Print Bus Scanner
 *
 * Copyright (c) 2016-2020 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef PFP_SCANNER_H
#define PFP_SCANNER_H  1

#include "pfp-rule.h"

struct pfp_rule *pfp_scan (void);

#endif  /* PFP_SCANNER_H */
