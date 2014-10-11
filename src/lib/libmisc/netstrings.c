#include <lib9.h>

#pragma varargck type "N" char *

static
int Nfmt(Fmt *fmt)
{
	char *str;
	char *strstart;

	str = va_arg(fmt->args, char *);
	strstart = strchr(str, ':') + 1;

	return fmtprint(fmt, "%s", strstart ? strstart : str);
}

void netstringfmt()
{
	fmtinstall('N', Nfmt);
}

char *mknetstring(char *str)
{
	return smprint("%d:%s", strlen(str), str);
}

int main(int argc, char *argv[])
{
	char *s = mknetstring("foobar");
	
	if(s == nil)
		sysfatal("%r");
	netstringfmt();
	print("%N", s);
	return 0;
}