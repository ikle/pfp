/*
 * Make PCI finger-print from lspci -nm output
 */

#include <stdio.h>

static
int show (const char *slot, const char *class, const char *intf,
	  const char *vendor, const char *device,
	  const char *svendor, const char *sdevice)
{
	if (intf[0] == '0')
		++intf;

	printf ("slot\t= %s\n", slot);
	printf ("class\t= %s.%s\n", class, intf);

	if (vendor[0] != '\0') {
		printf ("vendor\t= %s\n", vendor);
		printf ("device\t= %s\n", device);
	}

	if (svendor[0] != '\0') {
		printf ("svendor\t= %s\n", svendor);
		printf ("sdevice\t= %s\n", sdevice);
	}

	return 1;
}

static int scan (const char *line)
{
	const char *f[] = {
		"%9s \"%4s\" \"%4s\" \"%4s\" -r%2s -p%2s \"%4s\" \"%4s\"",
		"%9s \"%4s\" \"%4s\" \"%4s\" -r%2s \"%4s\" \"%4s\"",
		"%9s \"%4s\" \"%4s\" \"%4s\" -p%2s \"%4s\" \"%4s\"",
		"%9s \"%4s\" \"%4s\" \"%4s\" \"%4s\" \"%4s\"",
	};
	char s[10], c[5], v[5], d[5], r[3], p[3], sv[5], sd[5];

	if (sscanf (line, f[0], &s, &c, &v, &d, &r, &p, &sv, &sd) == 8)
		return show (s, c, p, v, d, sv, sd);

	if (sscanf (line, f[1], &s, &c, &v, &d, &r, &sv, &sd) == 7)
		return show (s, c, "00", v, d, sv, sd);

	if (sscanf (line, f[2], &s, &c, &v, &d, &p, &sv, &sd) == 7)
		return show (s, c, p, v, d, sv, sd);

	if (sscanf (line, f[3], &s, &c, &v, &d, &sv, &sd) == 6)
		return show (s, c, "00", v, d, sv, sd);

	return 0;
}

int main (int argc, char *argv[])
{
	int first = 1;
	char line[64];

	while (fgets (line, sizeof (line), stdin) != NULL) {
		if (!first)
			printf ("\n");

		scan (line);
		first = 0;
	}

	return 0;
}
