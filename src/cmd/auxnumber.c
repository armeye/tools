#include <lib9.h>
/*% 9 9c %; 9 9l -o # #.o 
see http://www.9atom.org/magic/man2html/8/number
FIXME: expression parser is missing!
*/
void usage()
{
	print("auxnumber -f fmt arg\n");
}

int main(int argc, char *argv[])
{
	char *fmt = "%lld";
	vlong number;

	ARGBEGIN {
		case 'f':
			fmt = EARGF(usage());
			break;
		default:
			print(" badflag(%c)\n", ARGC());
			return -1;
	} ARGEND

	number = atoll(argv[0]);

	print(fmt, number);
	print("\n");

	return 0;
}
