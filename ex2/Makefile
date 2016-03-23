CFLAGS=-Wall -g -std=c++11 $(INCS)

# cpp to object files rule
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

# Library Stuff
INCS=-I.
LOADLIBES=-L./

LIBSRC=uthreads.cpp uthread.cpp
LIBH=uthread.h uthreads.h
LIBOBJ=uthreads.o uthread.o
ARFLAGS=rvs
RANLIB=ranlib

UTHREADSLIB=libuthreads.a
TARGETS=$(UTHREADSLIB)

all: $(TARGETS)

# library rule
$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@


# test rules
TEST_FILE=main.cpp
TEST_OBJ=test
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes

$(TEST_OBJ): $(UTHREADSLIB)
	$(CC) $(CFLAGS) $(TEST_FILE) $(UTHREADSLIB) -o $@

ValgrindTest: $(TEST_OBJ)
	valgrind $(VALGRIND_FLAGS) ./$<

# cleaning and such
RM=rm -fv
clean:
	$(RM) $(TARGETS) $(UTHREADSLIB) $(OBJ) $(LIBOBJ) *~ *core $(TEST_OBJ)

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

# tar rule
TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) $(LIBH) Makefile README

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: all clean tar ValgrindTest
