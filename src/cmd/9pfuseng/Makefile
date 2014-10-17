CC=9 9c
CFLAGS!= pkg-config --cflags fuse
CFLAGS+= -DFUSE_USE_VERSION=25
LD=9 9l
LDFLAGS!= pkg-config --libs fuse

9pfuse: 9pfuseng.o errstr.o
	${LD} -o 9pfuseng ${LDFLAGS} 9pfuseng.o errstr.o

clean:
	rm -f 9pfuseng *.o
