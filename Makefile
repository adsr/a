all: atto

atto: atto.c lapi.c bstrlib.o bstraux.o
	gcc -g -o atto bstrlib.o bstraux.o atto.c -lncurses -llua5.2

bstrlib.o:
	gcc -c -o bstrlib.o ext/bstrlib/bstrlib.c

bstraux.o:
	gcc -c -o bstraux.o ext/bstrlib/bstraux.c

lapi.c:
	php lapi_gen.php

clean:
	rm -f *.o atto lapi.c
