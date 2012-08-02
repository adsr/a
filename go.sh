gcc -g -Wall -o main `find . -maxdepth 1 -regex '.*\.[hc]$' -printf "%p "` ext/bstrlib/bstrlib.o ext/bstrlib/bstraux.o -Lext/lua-5.2.1/src/ -Iext/lua-5.2.1/src/ -lm -llua -lncurses && ./main
