main.o:
	gcc main.c -o screenshot -s -lX11 -lpng -O3

run:
	./screenshot

install:
	ln -s "$(shell pwd)/screenshot" "/usr/bin/screenshot"
