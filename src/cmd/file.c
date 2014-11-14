/*% 9 9c %; 9 9l -o # #.o
Simple file replacement. With capsicum support
*/
#include <u.h>
#include <sys/capability.h>
#include <libc.h>

typedef struct {
	char           *type;
	char		magic     [16];
}		magic;

magic		magicdb  [] = {
	{.type = "ELF file",.magic = {0x7f, 'E', 'L', 'F', 0}},
	{.type = "PostScript",.magic = "%!"},
	{.type = "PDF",.magic = "%PDF"},
	{.type = "UNIX script",.magic = "#!"},
	{.type = "Java bytecode",.magic = {0xca, 0xfe, 0xba, 0xbe, 0}},
	{.type = "ar archive",.magic = "!<arch>\0x0a"},
	{.type = "UTF-16 be",.magic = {0xfe, 0xff, 0}},
	{.type = "UTF-16 le",.magic = {0xff, 0xfe, 0}},
	{.type = nil,.magic = 0}
};

int
main(int argc, char *argv[])
{
	int		fd        , len, i, l;
	char		buf       [512];
	magic          *mp;
	cap_rights_t	rights;

	cap_rights_init(&rights, CAP_READ);

	if (argc != 2) {
		print("file <file>'n");
		return 1;
	}
	fd = p9open(argv[1], OREAD);

	cap_rights_limit(fd, &rights);
	if (cap_enter() == -1)
		print("foo!\n");

	if (fd < 0)
		sysfatal("file: %r\n");

	if ((len = read(fd, buf, 512)) < 0)
		sysfatal("file: %r\n");

	for (i = 0, mp = magicdb; mp->type != nil; i++, mp = &magicdb[i]) {
		l = strlen(mp->magic);
		if (l <= len) {
			if (memcmp(mp->magic, buf, l) == 0) {
				print("%s\n", mp->type);
				break;
			}
		}
	}

	return 0;
}
