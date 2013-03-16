all: atto

atto: atto.c bstrlib.o bstraux.o
	gcc -o atto bstrlib.o bstraux.o atto.c -lncursesw

bstrlib.o:
	gcc -c -o bstrlib.o ext/bstrlib/bstrlib.c

bstraux.o:
	gcc -c -o bstraux.o ext/bstrlib/bstraux.c

clean:
	rm -f *.o atto
