EXEC_NAME   = we
INSTALL_DIR = /usr/bin

SHELL       = /usr/bin/env bash
VERSION     = $(shell git tag | tail -n1)


CFLAGS   = -Wall -Wextra -c -O2 \
	   -DPACKAGE=\"$(EXEC_NAME)\" \
	   -DVERSION=\"$(VERSION)\"

LDLIBS   = -lX11 -lXrandr -lImlib2 -lasound -lpthread

little_endian = $(shell [[ `lscpu | grep "Byte Order" | grep "Little"` ]] && echo 1 || echo 0)

ifeq ($(little_endian), 1)
	CFLAGS += -DLITTLE_ENDIAN
endif
