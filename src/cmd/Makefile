all: com thash sup auxnumber

com:
	cc -o com ../../extern/com.c
	cp com ../../bin/

file: com
	./com file.c
	cp file ../../bin/mfile

thash: com
	make -C ../../extern/tiger lib
	./com thash.c
	cp thash ../../bin/

auxnumber: com
	./com auxnumber.c
	cp auxnumber ../../bin/aux/number

sup:
	make -C ../../extern/sup all
	cp ../../extern/sup/sup ../../bin/

clean:
	rm -f *.o
	make -C ../../extern/sup clean
	file * | sed 's/://' | grep ELF | awk '{ print $1; }' | xargs rm -f
