#include <u.h>
#include <stdint.h>
#include <libc.h>

/*% 9 9c %; 9 9l -o # #.o -L../../extern/tiger -ltiger
*/

void tiger(void *data, uint64_t length, uint64_t *state);

#define BUFSIZ 1024*1024

int main(int argc, char *argv[])
{
	int fd;
	char *buf;
	char *bucket;
	int belem;
	long n, nb;
	uint64_t state[3];

	nb = 0;
	belem = 512;
	
	if(argc==1){
		print("%s <file>\n", argv[0]);
		return -1;
	}


	if((fd = p9open(argv[1], OREAD)) < 0)
		sysfatal("Can't open file: %r");

	if((buf = malloc(BUFSIZ)) == nil)
		sysfatal("malloc %r");

	if((bucket = calloc(sizeof(state), belem)) == nil)
		sysfatal("malloc %r");

	while(1){
		n = read(fd, buf, BUFSIZ);
		tiger(buf,n,state);
		memcpy(bucket+nb, state, sizeof state);
		if(nb == sizeof(state)*belem){
			belem*=2;
			realloc(bucket, sizeof(state)*belem);
		}
		nb+=sizeof state;
		if(n < BUFSIZ)
			break;
	}
	free(buf);
	tiger(bucket, nb, state);
	print("%ullx%ullx%ullx\n", state[0], state[1], state[2]);
	return 0;
}
