main.o:
	gcc main.c -o screenshot -s -lX11 -lpng -O3

install:
	-rm "/usr/local/bin/screenshot"
	ln -s `pwd`/screenshot "/usr/local/bin/screenshot"

uninstall:
	-rm "/usr/local/bin/screenshot"
