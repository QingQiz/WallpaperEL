EXEC_NAME = we
VERSION   = v0.2.2


CFLAGS   = -Wall -Wextra -c \
	   -DPACKAGE=\"$(EXEC_NAME)\" \
	   -DVERSION=\"$(VERSION)\"

LDLIBS   = -lX11 -lXrandr -lImlib2


INSTALL_DIR = /usr/bin
