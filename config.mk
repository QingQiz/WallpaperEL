EXEC_NAME = we
VERSION   = $(shell git tag | tail -n1)


CFLAGS   = -Wall -Wextra -c -O2 \
	   -DPACKAGE=\"$(EXEC_NAME)\" \
	   -DVERSION=\"$(VERSION)\"

LDLIBS   = -lX11 -lXrandr -lImlib2


INSTALL_DIR = /usr/bin
