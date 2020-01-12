include ../config.mk

CC = gcc
CFLAGS = -Wall -Wextra -c -DPACKAGE=\"$(EXEC_NAME)\" -DVERSION=\"$(VERSION)\"
LDLIBS = -lX11 -lXrandr -lImlib2
DEBUG_FLAG = -g

SRC = $(shell echo *.c)
INC = $(shell echo *.h)

TARGETS = $(SRC:.c=.o)

default : debug

debug : $(EXEC_NAME)

release : CFLAGS += -DRELEASE
release : DEBUG_FLAG = 
release : $(EXEC_NAME)

$(EXEC_NAME) : $(TARGETS)
	$(CC) $(LDLIBS) $(TARGETS) -o $@ $(DEBUG_FLAG)

options.o : options.c options.h debug.h help.inc
	$(CC) $(CFLAGS) $< -o $@ $(DEBUG_FLAG)

%.o : %.c %.h debug.h
	$(CC) $(CFLAGS) $< -o $@ $(DEBUG_FLAG)

clean:
	@rm -f *.o $(EXEC_NAME)


.PHONY : clean default debug release
