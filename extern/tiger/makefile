#On all machines with gcc (including Alpha)
CC = cc -O3
#On 32-bit machines
#CC = cc -O4
#On Alpha:
#CC = cc -O -migrate

all : testtiger

lib : tiger.o sboxes.o
	ar -crs libtiger.a tiger.o sboxes.o

clean:
	rm -f *.o testtiger libtiger.a

testtiger: testtiger.o tiger.o sboxes.o
	$(CC) -o testtiger testtiger.o tiger.o sboxes.o

testtiger.o: testtiger.c
	$(CC) -c testtiger.c

tiger.o: tiger.c
	$(CC) -c tiger.c

sboxes.o: sboxes.c
	$(CC) -c sboxes.c
