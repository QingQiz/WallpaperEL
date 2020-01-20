include ../config.mk

CC = gcc
CFLAGS = -Wall -Wextra -c -DPACKAGE=\"$(EXEC_NAME)\" -DVERSION=\"$(VERSION)\"
LDLIBS = -lX11 -lXrandr -lImlib2

SRC = $(shell echo *.c)
INC = $(shell echo *.h)

TARGETS = $(SRC:.c=.o)

default : debug

debug : CFLAGS += -g
debug : $(EXEC_NAME)

release : CFLAGS += -DRELEASE
release : $(EXEC_NAME)

$(EXEC_NAME) : $(TARGETS)
	$(CC) $(TARGETS) $(LDLIBS) -o $@

options.o : options.c options.h debug.h help.inc
	$(CC) $(CFLAGS) $< -o $@

%.o : %.c %.h debug.h
	$(CC) $(CFLAGS) $< -o $@

%.inc : %.raw
	sed 's/^\(.*\)$$/"\1\\n"/' < $< > $@

clean:
	@rm -f *.o $(EXEC_NAME) *.inc


.PHONY : clean default debug release
