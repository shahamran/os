CC=g++
CFLAGS = -Wall -Wextra -g -std=c++11 $(INCS)
RANLIB=ranlib

LIBSRC=uthreads.cpp uthread.cpp
LIBOBJ=uthreads.o uthread.o

INCS=-I.
LOADLIBES = -L./

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

UTHREADSLIB = libuthreads.a
TARGETS = $(UTHREADSLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) Makefile README

all: $(TARGETS)

TEST_FILE=main.cpp
TEST_OBJ=test

$(TEST_OBJ): $(UTHREADSLIB)
	$(CC) $(CFLAGS) $(TEST_FILE) $(UTHREADSLIB) -o $@

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

RM=rm -fv

clean:
	$(RM) $(TARGETS) $(UTHREADSLIB) $(OBJ) $(LIBOBJ) *~ *core $(TEST_OBJ)

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: all clean tar
