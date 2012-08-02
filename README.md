mace
====

mace is a console-based Lua-scriptable text editor written in C. It was originally written in PHP, but that project is now abandoned and archived in the [macephp](https://github.com/adsr/macephp/) repo.

mace is a work in progress and currently not usable.

**Required libraries**
* [ncurses](http://ftp.gnu.org/pub/gnu/ncurses/)
* [lua](http://www.lua.org/)
* [bstrlib](http://bstring.sourceforge.net/)
* [uthash](http://uthash.sourceforge.net/) + utarray
* [pcre](http://www.pcre.org/)

**How to compile and run**

There is no make file or build script yet.

1. Clone the mace repo
2. Download the required libraries and place them into a directory named `ext/` alongside main.c. (ncurses is most likely on your system already.)
3. Extract and compile each library such that you have the following directory structure:

```adam@acrux:~/mace$ pwd
/home/adam/mace
adam@acrux:~/mace$ find ext/ -maxdepth 2
ext/
ext/uthash-1.9.6.tar.bz2
ext/uthash
ext/uthash/src
ext/uthash/tests
ext/uthash/utarray.h
ext/uthash/doc
ext/uthash/uthash.h
...
ext/lua-5.2.1.tar.gz
ext/lua-5.2.1
ext/lua-5.2.1/src
ext/lua-5.2.1/doc
...
ext/pcre-8.30.tar.gz
ext/pcre-8.30
ext/pcre-8.30/pcre.h
...
ext/bstrlib-05122010.zip
ext/bstrlib
ext/bstrlib/bstrlib.h
ext/bstrlib/bstrlib.o
...
```

4. Execute `./go.sh`
