/*
 *  PCI finger-print parser
 */

%{
#include <assert.h>
#include <stdlib.h>

#include "pfp-parser.h"

#define YY_DECL  struct pfp_rule *pfplex (yyscan_t yyscanner)

static void fatal_error (const char *msg, yyscan_t yyscanner);

#define YY_FATAL_ERROR(msg)  fatal_error(msg, yyscanner)
%}

%option reentrant prefix="pfp"
%option yylineno never-interactive
%option nodefault noyywrap
%option noinput

%x COMMENT
%x SLOT SLOT_DEV SLOT_FN
%x CLASS CLASS_IF
%x ID
%x RULE

space	[ \t]+
xdigit	[0-9a-f]

eq	{space}={space}

any	.|\n

%%
	struct pfp_rule *head = NULL, **tail = &head, *rule = NULL;
	struct pfp_bdf *slot = NULL;
	int *id = NULL;

	BEGIN (INITIAL);

<COMMENT>{
	({space}\([^()]+\))?\n {
		BEGIN (RULE);
	}
	{any} {
		YY_FATAL_ERROR ("extra characters at end of line");
	}
}

<SLOT>{
	{xdigit}{1,2}: {
		assert (slot != NULL);
		slot->bus = strtol (yytext, NULL, 16);
		BEGIN (SLOT_DEV);
	}
	{any} {
		unput (yytext[0]);
		assert (slot != NULL);
		slot->bus = 0;
		BEGIN (SLOT_DEV);
	}
}

<SLOT_DEV>{
	[01]?{xdigit}\. {
		assert (slot != NULL);
		slot->device = strtol (yytext, NULL, 16);
		BEGIN (SLOT_FN);
	}
	{any} {
		YY_FATAL_ERROR ("PCI device number expected");
	}
}

<SLOT_FN>{
	[0-7] {
		assert (slot != NULL);
		slot->function = strtol (yytext, NULL, 8);
		BEGIN (COMMENT);
	}
	{any} {
		YY_FATAL_ERROR ("PCI device function expected");
	}
}

<CLASS>{
	{xdigit}{4} {
		rule->class = strtol (yytext, NULL, 16);
		BEGIN (CLASS_IF);
	}
	{any} {
		YY_FATAL_ERROR ("PCI class expected");
	}
}

<CLASS_IF>{
	\.{xdigit}{1,2} {
		rule->interface = strtol (yytext + 1, NULL, 16);
		BEGIN (COMMENT);
	}
	{any} {
		unput (yytext[0]);
		rule->interface = -1;
		BEGIN (COMMENT);
	}
}

<ID>{
	{xdigit}{4} {
		assert (id != NULL);
		*id = strtol (yytext, NULL, 16);
		BEGIN (COMMENT);
	}
	{any} {
		YY_FATAL_ERROR ("PCI identifier expected");
	}
}

<RULE>{
	#.+\n		/* line comment */
	parent{eq}	slot = &rule->parent; BEGIN (SLOT);
	slot{eq}	slot = &rule->slot;   BEGIN (SLOT);
	class{eq}	BEGIN (CLASS);
	vendor{eq}	id = &rule->vendor;  BEGIN (ID);
	device{eq}	id = &rule->device;  BEGIN (ID);
	svendor{eq}	id = &rule->svendor; BEGIN (ID);
	sdevice{eq}	id = &rule->sdevice; BEGIN (ID);

	<<EOF>>		return head;
	\n		BEGIN (INITIAL);

	{any}		YY_FATAL_ERROR ("unrecognized rule line");
}

<INITIAL>{
	#.+\n	/* line comment */
	\n	/* empty line */

	{any} {
		if ((rule = pfp_rule_alloc ()) == NULL) {
			pfp_rule_free (head);

			return NULL;
		}

		*tail = rule;
		tail = &rule->next;

		unput (yytext[0]);
		BEGIN (RULE);
	}
}

%%

static void fatal_error (const char *msg, yyscan_t yyscanner)
{
	struct yyguts_t * yyg = yyscanner;

	fprintf (stderr, "E:%d: %s\n", yylineno, msg);
	exit (1);
}

struct pfp_parser *pfp_parser_alloc (FILE *from)
{
	yyscan_t s;

	if (yylex_init (&s) != 0)
		return NULL;

	yyset_in (from, s);

	return s;
}

void pfp_parser_free (struct pfp_parser *o)
{
	if (o == NULL)
		return;

	yylex_destroy (o);
}

struct pfp_rule *pfp_parser_run (struct pfp_parser *o)
{
	return yylex (o);
}

void pfp_parser_reset (struct pfp_parser *o, FILE *from)
{
	yyrestart (from, o);
}

/* all in one */
struct pfp_rule *pfp_parse (FILE *from)
{
	struct pfp_parser *p;
	struct pfp_rule *r;

	if ((p = pfp_parser_alloc (from)) == NULL)
		return NULL;

	r = pfp_parser_run (p);

	pfp_parser_free (p);
	return r;
}
