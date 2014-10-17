#include <lib9.h>

char *strim(char *s)
{
	char *u0, *u1;
	char *out;

	u0 = s;
	u1 = strchr(s, '\0');

	while(*u0 == ' ' || *u0 == '\t')
		u0++;

	do {
		u1--;
	} while(*u1 == ' ' || *u1 == '\t');

	out = malloc(u1 - u0 + 2);
	strncpy(out, u0, u1 - u0 + 1);

	return out;
}