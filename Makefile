all: clean atto

atto: lapi.c bstrlib.o bstraux.o
	colorgcc -Wall -g -o atto bstrlib.o bstraux.o `ls *.c` -lncurses -llua5.2 -lm -lpcre

bstrlib.o:
	gcc -c -o bstrlib.o ext/bstrlib/bstrlib.c

bstraux.o:
	gcc -c -o bstraux.o ext/bstrlib/bstraux.c

lapi.c:
	php lapi_gen.php

clean:
	rm -f *.o atto lapi.c core

test: atto
	./atto -T
