include ../config.mk

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

options.o : options.c options.h help.inc common.h
	$(CC) $(CFLAGS) $< -o $@

%.o : %.c %.h common.h
	$(CC) $(CFLAGS) $< -o $@

%.inc : %.raw
	sed 's/^\(.*\)$$/"\1\\n"/' < $< > $@

clean:
	@rm -f *.o $(EXEC_NAME) *.inc


.PHONY : clean default debug release
