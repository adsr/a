all: atto

CC=$(shell if test `which colorgcc`; then echo colorgcc; else echo gcc; fi)
SOURCES=editor.c buffer.c cursor.c mark.c keymap.c bview.c srule.c util.c
LIBS=-ltermbox -llua5.2 -lm -lpcre

atto: $(SOURCES)
	$(CC) -Wall -g -o atto $(SOURCES) $(LIBS)

stubs:
	./mkcstubs atto.h

clean:
	rm -f *.o atto core
