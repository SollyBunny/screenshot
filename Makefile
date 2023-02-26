build:
	gcc main.c -o screenshot -s -lX11 -lwebp -O4

install:
	rm -f "/usr/local/bin/screenshot"
	ln -s "`pwd`/screenshot" "/usr/local/bin/screenshot"

uninstall:
	-m -f "/usr/local/bin/screenshot"
