all: com thash

com:
	cc -o com ../../extern/com.c
	cp com ../../bin/

thash: com
	make -C ../../extern/tiger lib
	./com thash.c
	cp thash ../../bin/

clean:
	rm -f *.o
	file * | sed 's/://' | grep ELF | awk '{ print $1; }' | xargs rm -f
