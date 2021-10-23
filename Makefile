main.o:
	gcc main.c -o screenshot -s -lX11 -lpng -O3

run:
	./screenshot

install:
	-rm /usr/bin/screenshot
	ln -s "$(shell pwd)/screenshot" "/usr/bin/screenshot"

uninstall:
	rm /usr/bin/screenshot
